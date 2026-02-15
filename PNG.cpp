#include "Debug.h"
#include "HuffmanTree.h"
#include "PNG.h"
#include <algorithm>
#include <cinttypes>
#include <fstream>
#include <iostream>
#include <string>

PNG::PNG(const char* file_path) {
	std::ifstream file(file_path, std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Could not open file");
	}
	else {
		Compare_Signature(file);
	}
	while (Load_Next_Chunk(file));
}

void PNG::Converts_To_Little_Endian(unsigned int& big_endian) {
	unsigned int result = 0u;
	result |= (big_endian & 0xFF000000) >> 24;
	result |= (big_endian & 0x00FF0000) >> 8;
	result |= (big_endian & 0x0000FF00) << 8;
	result |= (big_endian & 0x000000FF) << 24;
	big_endian = result;
}

void PNG::Converts_To_Little_Endian(uint16_t& big_endian) {
	uint16_t result = 0u;
	result |= (big_endian & 0xFF00) >> 8;
	result |= (big_endian & 0x00FF) << 8;
	big_endian = result;
}

bool PNG::Load_Next_Chunk(std::ifstream& file) {
	Chunk chunk;
	Chunk::IHDR ihdr;
	file.read((char*)&chunk.m_length, sizeof(chunk.m_length));
	Converts_To_Little_Endian(chunk.m_length);
	file.read(chunk.m_type, 4i64);
	chunk.m_raw_blocks = std::make_unique<uint8_t[]>(chunk.m_length);
	file.read((char*)chunk.m_raw_blocks.get(), chunk.m_length);
	file.read((char*)&chunk.m_crc, sizeof(chunk.m_crc));
	Converts_To_Little_Endian(chunk.m_crc);
	return Process_Chunk(chunk);
}

bool PNG::Process_Chunk(PNG::Chunk& chunk) {
	bool we_still_have_more_chunk_to_be_processed = true;
	std::string this_chunk_is(chunk.m_type);
	if (this_chunk_is == "IHDR") {
		PNG::Chunk::IHDR ihdr = *reinterpret_cast<PNG::Chunk::IHDR*>(chunk.m_raw_blocks.get());
		PNG::Converts_To_Little_Endian(ihdr.m_width);
		PNG::Converts_To_Little_Endian(ihdr.m_height);
		m_width = ihdr.m_width;
		m_height = ihdr.m_height;
	}
	else if (this_chunk_is == "IDAT") {
		bool is_all_the_decompressed_data_retrieved_from_this_chunk = false;
		PNG::Chunk::Block::BitReader bit_reader(chunk.m_raw_blocks.get());
		bit_reader.Forward(16u);
		do {
			size_t bits_processed = 0u;
			PNG::Chunk::Block block(bit_reader, m_decompressed_data);
			is_all_the_decompressed_data_retrieved_from_this_chunk = block.m_is_last_block;
		} while (!is_all_the_decompressed_data_retrieved_from_this_chunk);
	}
	else if (this_chunk_is == "IEND") {
		we_still_have_more_chunk_to_be_processed = false;
	}
	return we_still_have_more_chunk_to_be_processed;
}

void PNG::Compare_Signature(std::ifstream& file) {
	uint8_t signature[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
	uint8_t file_signature[8];
	file.read((char*)file_signature, sizeof(file_signature));
	if (not std::equal(std::begin(signature), std::end(signature), std::begin(file_signature))) {
		throw std::runtime_error("Not PNG-file");
	}
}

std::ostream& operator<<(std::ostream& os, const PNG::Chunk& chunk) {
	const size_t MAX_OUTPUT_LENGTH = 10;
	size_t length = chunk.m_length < MAX_OUTPUT_LENGTH ? chunk.m_length : MAX_OUTPUT_LENGTH;
	for (int i = 0; i < length; i++) {
		os << std::to_string(chunk.m_raw_blocks[i]) << " ";
	}
	return os;
}

PNG::Chunk::Block::BitReader::BitReader(const uint8_t* data) { m_data = data; }

uint16_t PNG::Chunk::Block::BitReader::Read(size_t size) {
	uint16_t result = Peak(size);
	Forward(size);
	return result;
}

uint16_t PNG::Chunk::Block::BitReader::Peak(size_t size) {
	if (size > 16) {
		throw std::runtime_error("Too big reading size for BitReader.");
	}
	uint16_t result = 0u;
	for (size_t i = 0; i < size; i++) {
		const size_t byte_offset = (m_offset + i) / 8u;
		const size_t bit_offset_in_that_byte = (m_offset + i) % 8u;
		const uint16_t current_bit = 1u << i;
		const uint8_t current_bit_in_that_byte = 1u << bit_offset_in_that_byte;
		if (m_data[byte_offset] & current_bit_in_that_byte) {
			result |= current_bit;
		}
	}
	return result;
}

size_t PNG::Chunk::Block::BitReader::Has_Read() {
	return m_offset;
}

void PNG::Chunk::Block::BitReader::Align() {
	size_t rest = m_offset % 8u;
	if (rest) {
		m_offset += 8u - rest;
	}
}

void PNG::Chunk::Block::BitReader::Forward(size_t size) {
	m_offset += size;
}

PNG::Chunk::Block::Block(PNG::Chunk::Block::BitReader& bit_reader, std::vector<uint8_t>& output) {
	m_is_last_block = bit_reader.Read(1) & 0b00000001;
	uint16_t compression_type = bit_reader.Read(2);
	compression_type &= 0b0000011;
	switch (compression_type) {
	case 0b0000001:
		m_type = FIXED_HUFFMAN_CODES;
		Decompress_Block_Fixed_Huffman(bit_reader, output);
		break;
	case 0b0000010:
		m_type = DYNAMIC_HUFFMAN_CODE;
		Decompress_Block_Dynamic_Huffman(bit_reader, output);
		break;
	default:
		m_type = UNCOMPRESSED;
		bit_reader.Align();
		Decompress_Block_That_Is_Uncompressed(bit_reader, output);
		break;
	}
}

void PNG::Chunk::Block::Decompress_Block_Dynamic_Huffman(PNG::Chunk::Block::BitReader& bit_reader, std::vector<uint8_t>& output) {
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
	std::vector<uint8_t> code_indices = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
	std::vector<uint8_t> code_lengths(code_indices.size());
	for (int i = 0; i < header.HCLEN; i++) {
		code_lengths[code_indices[i]] = bit_reader.Read(3);
	}
	std::vector<uint8_t> code_symbols;
	for (uint8_t i = 0; i < code_lengths.size(); i++) {
		code_symbols.push_back(i);
	}
	HuffmanTree<uint8_t, uint8_t, uint8_t> code_tree(code_symbols, code_lengths);

	// HLIT
	std::vector<uint8_t> literal_lengths(286u);
	for (size_t i = 0u; i < header.HLIT; i++) {
		for (uint8_t u = code_tree.Minimum_Length(); u <= 8u; u++) {
			uint16_t temp_bits = bit_reader.Peak(u);
			std::optional<uint16_t> temp_symbol = code_tree.Decode(temp_bits, u);
			if (temp_symbol != std::nullopt) {
				bit_reader.Forward(u);
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

	std::vector<uint16_t> literal_symbols(literal_lengths.size());
	for (uint16_t i = 0u; i < literal_lengths.size(); i++) {
		literal_symbols[i] = i;
	}
	HuffmanTree<uint16_t, uint16_t, uint8_t> literal_tree(literal_symbols, literal_lengths);

	// HDIST
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
		for (uint8_t u = code_tree.Minimum_Length(); u <= 8; u++) {
			uint8_t temp_bits = bit_reader.Peak(u);
			std::optional<uint8_t> temp_symbol = code_tree.Decode(temp_bits, u);
			if (temp_symbol != std::nullopt) {
				bit_reader.Forward(u);
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
	std::vector<Distance_Code> distance_symbols(distance_lengths.size());
	for (uint8_t i = 0u; i < distance_lengths.size(); i++) {
		distance_symbols[i] = distance_table[i];
	}
	HuffmanTree<uint16_t, Distance_Code, uint8_t> distance_tree(distance_symbols, distance_lengths);

	// decoding the block
	uint16_t current_symbol = 0u;
	do {
		for (size_t i = literal_tree.Minimum_Length(); i <= 16; i++) {
			uint16_t temp_bits = bit_reader.Peak(i);
			std::optional<uint16_t> temp_symbol = literal_tree.Decode(temp_bits, i);
			if (temp_symbol != std::nullopt) {
				bit_reader.Forward(i);
				current_symbol = *temp_symbol;
				if (*temp_symbol < 256u) {
					output.push_back(*temp_symbol);
				}
				else if (*temp_symbol > 256u) {
					struct Length_Code {
						uint16_t base_length;
						uint8_t extra_bits;
					};
					const Length_Code LENGTH_TABLE[29] = {
						{3,   0}, {4,   0}, {5,   0}, {6,   0},   // 257-260
						{7,   0}, {8,   0}, {9,   0}, {10,  0},   // 261-264
						{11,  1}, {13,  1}, {15,  1}, {17,  1},   // 265-268
						{19,  2}, {23,  2}, {27,  2}, {31,  2},   // 269-272
						{35,  3}, {43,  3}, {51,  3}, {59,  3},   // 273-276
						{67,  4}, {83,  4}, {99,  4}, {115, 4},   // 277-280
						{131, 5}, {163, 5}, {195, 5}, {227, 5},   // 281-284
						{258, 0}                                   // 285
					};
					Length_Code temp_code = LENGTH_TABLE[*temp_symbol - 257u];
					uint16_t length = temp_code.base_length + bit_reader.Read(temp_code.extra_bits);
					for (uint16_t u = distance_tree.Minimum_Length(); u <= 16u; u++) {
						uint16_t distance_bits = bit_reader.Peak(u);
						std::optional<Distance_Code> distance_code = distance_tree.Decode(distance_bits, u);
						if (distance_code != std::nullopt) {
							bit_reader.Forward(u);
							uint16_t distance = (*distance_code).base_length + bit_reader.Read((*distance_code).extra_bits);
							size_t anchor = output.size();
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

void PNG::Chunk::Block::Decompress_Block_Fixed_Huffman(PNG::Chunk::Block::BitReader& bit_reader, std::vector<uint8_t>& output) {
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

void PNG::Chunk::Block::Decompress_Block_That_Is_Uncompressed(PNG::Chunk::Block::BitReader& bit_reader, std::vector<uint8_t>& output) {
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
