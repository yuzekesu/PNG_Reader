#include "HuffmanTree.h"
#include "PNG.h"

/// <summary>
/// Constructs the block and concatenate the decompressed version to the "output".
/// </summary>
PNG::Chunk::Block::Block(BitReader<uint16_t>& bit_reader, std::vector<uint8_t>& output) {
	m_is_last_block = bit_reader.Read(1) & 0b00000001;
	uint16_t compression_type = bit_reader.Read(2);
	compression_type &= 0b0000011;
	switch (compression_type) {
	case 0b0000001:
		// FIXED_HUFFMAN_CODES;
		Decompress_Block_Fixed_Huffman(bit_reader, output);
		break;
	case 0b0000010:
		// DYNAMIC_HUFFMAN_CODE;
		Decompress_Block_Dynamic_Huffman(bit_reader, output);
		break;
	default:
		// UNCOMPRESSED;
		bit_reader.Align();
		Decompress_Block_That_Is_Uncompressed(bit_reader, output);
		break;
	}
}
/// <summary>
/// Decompress current block (inside the bit_reader) with dynamic huffman in mind.
/// </summary>
void PNG::Chunk::Block::Decompress_Block_Dynamic_Huffman(BitReader<uint16_t>& bit_reader, std::vector<uint8_t>& output) {
	struct Header_Processed {
		size_t HLIT; // 5 bits
		size_t HDIST;// 5 bits
		size_t HCLEN;// 4 bits
	};
	Header_Processed header{};
	header.HLIT = bit_reader.Read(5u) + 257u; // amount of symbols
	header.HDIST = bit_reader.Read(5u) + 1u;
	header.HCLEN = bit_reader.Read(4u) + 4u;

	// HCLEN
	// this constructs a Huffman tree where each symbol represents the bit-length of a code in the main huffman tree
	std::vector<uint8_t> code_indices = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
	std::vector<uint8_t> code_lengths(code_indices.size());
	for (int i = 0; i < header.HCLEN; i++) { // just reading the length
		code_lengths[code_indices[i]] = bit_reader.Read(3);
	}
	std::vector<uint8_t> code_symbols;
	for (uint8_t i = 0; i < code_lengths.size(); i++) {

		// the order doesnt follow the code_indices
		code_symbols.push_back(i);
	}
	HuffmanTree<uint8_t, uint8_t, uint8_t> code_length_tree(code_symbols, code_lengths);

	// HLIT - literal/length huffman tree
	// the MAIN canoncial huffman starts here
	// reading bits for "length of huffman codes" and construct the canoncial huffman
	// the previous section will acts as decoder since the "length of huffman codes" is stored as canoncial huffman code
	std::vector<uint8_t> literal_lengths(286u);
	for (size_t i = 0u; i < header.HLIT; i++) { // for so many literal
		for (uint8_t u = code_length_tree.Minimum_Length(); u <= 8u; u++) {

			// start getting "length of huffman codes" using the previous canoncial huffman codes
			// bit per bit reading
			uint16_t temp_bits = bit_reader.Peak(u);

			// why reversed: historical reason.
			// huffman codes stores with Least-Significant-Bit first in DEFLATE.
			// but canoncial huffman codes are usually described MSB-first arithmetically.
			// should make the BitReader return the reversed bits directly instead of doing it afterward
			temp_bits = code_length_tree.Reverse_Bits(temp_bits, u);
			std::optional<uint16_t> temp_symbol = code_length_tree.Decode(temp_bits, u);
			if (temp_symbol != std::nullopt) {

				// update the offset in the bit-reader when found the symbol using huffman
				bit_reader.Forward(u);

				// so what does the symbol say:
				// <16 : the bits represents the length
				// =16 : repeats the previous length k many time. where k is the next 2 bits
				// =17 : repeats zero as length, k many time. where k is the next 3 bits
				// =17 : repeats zero as length, k many time. where k is the next 7 bits
				if (*temp_symbol < 16u) {
					literal_lengths[i] = *temp_symbol;
				}
				else if (*temp_symbol == 16u) {
					uint8_t repeat_how_many_times = bit_reader.Read(2) + 3u;
					for (uint8_t k = 0u; k < repeat_how_many_times; k++) {
						literal_lengths[i + k] = literal_lengths[i - 1u];
					}
					i += repeat_how_many_times - 1;
				}
				else if (*temp_symbol == 17u) {
					uint8_t repeat_how_many_times = bit_reader.Read(3) + 3u;
					for (uint8_t k = 0u; k < repeat_how_many_times; k++) {
						literal_lengths[i + k] = 0u;
					}
					i += repeat_how_many_times - 1;
				}
				else if (*temp_symbol == 18u) {
					uint8_t repeat_how_many_times = bit_reader.Read(7) + 11u;
					for (uint8_t k = 0u; k < repeat_how_many_times; k++) {
						literal_lengths[i + k] = 0u;
					}
					i += repeat_how_many_times - 1;
				}
				break;
			}
		}
	}

	// make the huffman-tree for the literals, aka datas
	std::vector<uint16_t> literal_symbols(literal_lengths.size());
	for (uint16_t i = 0u; i < literal_lengths.size(); i++) {
		literal_symbols[i] = i;
	}
	HuffmanTree<uint16_t, uint16_t, uint8_t> literal_tree(literal_symbols, literal_lengths);

	// HDIST
	// the literals from the previous section are actuelly not only datas, it also contain "length"
	// the "length" represents the length of the data, so where are the datas ?
	// we retrieve data from the previous decompressed bitsstream using "distance" value.
	struct Distance_Code {
		uint16_t base_length;
		uint8_t extra_bits;
	};
	const Distance_Code distance_table[30u] = { // 0 - 29
		{1, 0}, {2, 0}, {3, 0}, {4, 0},            // codes 0-3
		{5, 1}, {7, 1},                            // codes 4-5 (1 extra bit)
		{9, 2}, {13, 2},                           // codes 6-7 (2 extra bits)
		{17, 3}, {25, 3},                          // codes 8-9 (3 extra bits)
		{33, 4}, {49, 4},                          // codes 10-11
		{65, 5}, {97, 5},                          // codes 12-13
		{129, 6}, {193, 6},                        // codes 14-15
		{257, 7}, {385, 7},                        // codes 16-17
		{513, 8}, {769, 8},                        // codes 18-19
		{1025, 9}, {1537, 9},                      // codes 20-21
		{2049, 10}, {3073, 10},                    // codes 22-23
		{4097, 11}, {6145, 11},                    // codes 24-25
		{8193, 12}, {12289, 12},                   // codes 26-27
		{16385, 13}, {24577, 13}                   // codes 28-29
	};
	std::vector<uint8_t> distance_lengths(30u);
	for (uint8_t i = 0u; i < header.HDIST; i++) {

		// reads bit by bit from the bit reader
		for (uint8_t u = code_length_tree.Minimum_Length(); u <= 8; u++) {
			uint8_t temp_bits = bit_reader.Peak(u);
			temp_bits = code_length_tree.Reverse_Bits(temp_bits, u);
			std::optional<uint8_t> temp_symbol = code_length_tree.Decode(temp_bits, u);
			if (temp_symbol != std::nullopt) {

				// found matching huffman and update the offset
				bit_reader.Forward(u);

				// so what does the symbol say:
				// <16 : the bits represents the length
				// =16 : repeats the previous length k many time. where k is the next 2 bits
				// =17 : repeats zero as length, k many time. where k is the next 3 bits
				// =17 : repeats zero as length, k many time. where k is the next 7 bits
				if (*temp_symbol < 16u) {
					distance_lengths[i] = *temp_symbol;
				}
				else if (*temp_symbol == 16u) {
					uint8_t repeat_how_many_times = bit_reader.Read(2) + 3u;
					for (uint8_t k = 0u; k < repeat_how_many_times; k++) {
						distance_lengths[i + k] = distance_lengths[i - 1u];
					}
					i += repeat_how_many_times - 1;
				}
				else if (*temp_symbol == 17u) {
					uint8_t repeat_how_many_times = bit_reader.Read(3) + 3u;
					for (uint8_t k = 0u; k < repeat_how_many_times; k++) {
						distance_lengths[i + k] = 0u;
					}
					i += repeat_how_many_times - 1;
				}
				else if (*temp_symbol == 18u) {
					uint8_t repeat_how_many_times = bit_reader.Read(7) + 11u;
					for (uint8_t k = 0u; k < repeat_how_many_times; k++) {
						distance_lengths[i + k] = 0u;
					}
					i += repeat_how_many_times - 1;
				}
				break;
			}
		}
	}

	// make the huffman-tree for the distances
	std::vector<Distance_Code> distance_symbols(distance_lengths.size());
	for (uint8_t i = 0u; i < distance_lengths.size(); i++) {
		distance_symbols[i] = distance_table[i];
	}
	HuffmanTree<uint16_t, Distance_Code, uint8_t> distance_tree(distance_symbols, distance_lengths);

	// decoding the block
	// regular huffman decompression, nothing special
	uint16_t current_symbol = 0u;

	// keep decompressing untill reach the symbol that indicates the last symbol
	do {

		// still. reads bit by bit
		for (size_t i = literal_tree.Minimum_Length(); i <= 16; i++) {
			uint16_t temp_bits = bit_reader.Peak(i);
			temp_bits = literal_tree.Reverse_Bits(temp_bits, i);
			std::optional<uint16_t> temp_symbol = literal_tree.Decode(temp_bits, i);
			if (temp_symbol != std::nullopt) {

				// found matching huffman and update the offset
				bit_reader.Forward(i);
				current_symbol = *temp_symbol;

				// the "length" is apart of the symbol
				// and the real data has size of uint8_t (255u)

				// what does the symbol say:
				// <256 the bits represents the data itself
				// =256 indicate this is the last symbol of the block
				// >256 this data shall be copied from elsewhere (length & distance)
				if (*temp_symbol < 256u) {
					output.push_back(*temp_symbol);
				}
				else if (*temp_symbol > 256u) {
					struct Length_Code {
						uint16_t base_length;
						uint8_t extra_bits;
					};

					// {length, reads so many extra bits and adds it to the length}
					const Length_Code LENGTH_TABLE[29] = {
						{3,   0}, {4,   0}, {5,   0}, {6,   0},		// 257-260
						{7,   0}, {8,   0}, {9,   0}, {10,  0},		// 261-264
						{11,  1}, {13,  1}, {15,  1}, {17,  1},		// 265-268
						{19,  2}, {23,  2}, {27,  2}, {31,  2},		// 269-272
						{35,  3}, {43,  3}, {51,  3}, {59,  3},		// 273-276
						{67,  4}, {83,  4}, {99,  4}, {115, 4},		// 277-280
						{131, 5}, {163, 5}, {195, 5}, {227, 5},		// 281-284
						{258, 0}									// 285
					};

					// length starts at symbol 257u, thus minus 257u
					Length_Code temp_code = LENGTH_TABLE[*temp_symbol - 257u];
					uint16_t length = temp_code.base_length + bit_reader.Read(temp_code.extra_bits);

					// reads bit by bit for retrieving the "distance" value
					// since the "distance" symbol is always directly after the "length" symbol
					for (uint16_t u = distance_tree.Minimum_Length(); u <= 16u; u++) {
						uint16_t distance_bits = bit_reader.Peak(u);
						distance_bits = distance_tree.Reverse_Bits(distance_bits, u);
						std::optional<Distance_Code> distance_code = distance_tree.Decode(distance_bits, u);
						if (distance_code != std::nullopt) {

							// founds the symbol and updats the offset
							bit_reader.Forward(u);
							uint16_t distance = (*distance_code).base_length + bit_reader.Read((*distance_code).extra_bits);
							size_t anchor = output.size();

							// copys "length"st bits from "distance"st bits away
							for (uint16_t k = 0u; k < length; k++) {
								output.push_back(output[anchor - distance + k]);
							}
							break;
						}
					}
				}
				break;
			}
		}
	} while (current_symbol != 256u);
}
/// <summary>
/// do people ever use this ?
/// </summary>
void PNG::Chunk::Block::Decompress_Block_Fixed_Huffman(BitReader<uint16_t>& bit_reader, std::vector<uint8_t>& output) {
	uint8_t result = 0u;
	const uint16_t length_base[] = {
		3, 4, 5, 6, 7, 8, 9, 10,	// 257-264
		11, 13, 15, 17,				// 265-268
		19, 23, 27, 31,				// 269-272
		35, 43, 51, 59,				// 273-276
		67, 83, 99, 115,			// 277-280
		131, 163, 195, 227,			// 281-284
		258							// 285
	};
	const uint16_t length_extra_bits[] = {
		0, 0, 0, 0, 0, 0, 0, 0,		// 257-264
		1, 1, 1, 1,					// 265-268
		2, 2, 2, 2,					// 269-272
		3, 3, 3, 3,					// 273-276
		4, 4, 4, 4,					// 277-280
		5, 5, 5, 5,					// 281-284
		0							// 285
	};
	const uint16_t distance_base[] = {
		1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
		257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
		8193, 12289, 16385, 24577
	};
	const uint8_t distance_extra_bits[] = {
		0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
		7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
	};
	while (true) {
		bool is_length_code = false;
		uint16_t length = 0u;
		uint16_t offset = 0u;
		uint16_t bits = bit_reader.Peak(7);
		if (bits == 0u) { // range 256 <7>
			bit_reader.Forward(7);
			break;
		}
		else {
			if (0b0000001 <= bits && bits <= 0b0010111) { // range 257 - 279 <7>
				is_length_code = true;
				offset = bits - 0b0000001;
				bit_reader.Forward(7);
			}
			else {
				bits = bit_reader.Peak(8);
				if (0b00110000 <= bits && bits <= 0b10111111) { // range 0 - 143 <8>
					result = bits - 0b00110000 + 0;
					bit_reader.Forward(8);
				}
				else if (0b11000000 <= bits && bits <= 0b11000101) { // range 280 - 285 <8>
					is_length_code = true;
					offset = bits - 0b11000000 + 23;
					bit_reader.Forward(8);
				}
				else {
					bits = bit_reader.Peak(9);
					if (0b110010000 <= bits && bits <= 0b111111111) { // range 144 - 255 <9>
						result = bits - 0b110010000 + 144u;
						bit_reader.Forward(9);
					}
				}
			}
		}
		if (is_length_code) {
			length = length_base[offset] + bit_reader.Read(length_extra_bits[offset]);
			offset = bit_reader.Read(5);
			uint16_t distance = distance_base[offset] + bit_reader.Read(distance_extra_bits[offset]);
			size_t anchor = output.size();
			for (uint16_t i = 0u; i < length; i++) {
				output.push_back(output[anchor - distance + i]);
			}
		}
		else {
			output.push_back(result);
		}
	}
}
/// <summary>
/// Just concatenate the uncompressed block onto the output (std:vector&lt;uint8_t&gt;).
/// </summary>
void PNG::Chunk::Block::Decompress_Block_That_Is_Uncompressed(BitReader<uint16_t>& bit_reader, std::vector<uint8_t>& output) {
	struct Header_Processed {
		uint16_t LEN;
		uint16_t NLEN;
	};
	Header_Processed header;
	header.LEN = bit_reader.Read(16u);
	header.NLEN = bit_reader.Read(16u);
	if (header.LEN + header.NLEN != 0xFFFF) {
		throw std::runtime_error("Uncompressed block LEN/NLEN mismatch.");
	}
	for (uint16_t i = 0u; i < header.LEN; i++) {
		output.push_back(bit_reader.Read(8u));
	}
}