#include "Debug.h"
#include "PNG.h"
#include <algorithm>
#include <cinttypes>
#include <fstream>
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

void PNG::Chunk::Block::BitReader::Forward(size_t size) {
	m_offset += size;
}

PNG::Chunk::Block::Block(PNG::Chunk::Block::BitReader& bit_reader, std::vector<uint8_t>& output) {
	m_is_last_block = bit_reader.Read(1) & 0b00000001;
	uint16_t compression_type = bit_reader.Read(2);
	compression_type &= 0b0000011;
	bit_reader.Forward(5);
	switch (compression_type) {
	case 0b0000001:
		m_type = FIXED_HUFFMAN_CODES;
		decompress_block_fixed_huffman(bit_reader, output);
		break;
	case 0b0000010:
		m_type = DYNAMIC_HUFFMAN_CODE;
		break;
	default:
		m_type = UNCOMPRESSED;
	}
}

void PNG::Chunk::Block::decompress_block_fixed_huffman(PNG::Chunk::Block::BitReader& bit_reader, std::vector<uint8_t>& output) {
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
