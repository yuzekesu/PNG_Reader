#include "HuffmanTree.h"
#include "PNG.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>


/// <summary>
/// Read .png bitsstream through file path.
/// </summary>
PNG::PNG(const wchar_t* file_path) {
	std::ifstream file(file_path, std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Could not open file");
	}
	else {
		Compare_Signature(file);
	}
	std::vector<uint8_t> compressed_data;
	while (Load_Chunk(file, compressed_data));
	std::vector<uint8_t> decompressed_data = Decompress_Blocks(compressed_data);
	Apply_Filter(decompressed_data);
	Load_RGBA(decompressed_data);
}
/// <summary>
/// Read .png bitsstream through win32 resource, aka embedded into the .exe file.
/// </summary>
PNG::PNG(const void* pSysMem) {
	const uint8_t* bytes = reinterpret_cast<const uint8_t*>(pSysMem);
	size_t offset = 8u;
	std::vector<uint8_t> compressed_data;
	bool is_not_the_last_chunk = true;
	while (is_not_the_last_chunk) {
		::PNG::Chunk chunk;

		// reads meta data of the chunk
		chunk.m_length = *(reinterpret_cast<const unsigned int*>(bytes + offset));
		offset += sizeof(chunk.m_length);
		Converts_To_Little_Endian(chunk.m_length);
		std::copy_n(reinterpret_cast<const char*>(bytes + offset), 4u, chunk.m_type);
		offset += sizeof(4u);
		chunk.m_raw_blocks = std::make_unique<uint8_t[]>(chunk.m_length);
		std::copy_n(reinterpret_cast<const char*>(bytes + offset), chunk.m_length, chunk.m_raw_blocks.get());
		offset += chunk.m_length;
		chunk.m_crc = *(reinterpret_cast<const unsigned int*>(bytes + offset));
		offset += sizeof(chunk.m_crc);
		Converts_To_Little_Endian(chunk.m_crc);

		// reads the chunks until we reaches the end
		is_not_the_last_chunk = Process_Chunk(chunk, compressed_data);
	}
	std::vector<uint8_t> decompressed_data = Decompress_Blocks(compressed_data);
	Apply_Filter(decompressed_data);
	Load_RGBA(decompressed_data);
}
/// <summary>
/// Reverse the bits in the bitsstream
/// </summary>
void PNG::Converts_To_Little_Endian(unsigned int& big_endian) {
	/// .png stores all the bits reversed for systems that uses little endian
	/// Little-endian is a computer architecture byte-ordering format where the least significant byte (LSB)—the "little end" or smallest value—is stored at the lowest memory address.
	unsigned int result = 0u;
	result |= (big_endian & 0xFF000000) >> 24;
	result |= (big_endian & 0x00FF0000) >> 8;
	result |= (big_endian & 0x0000FF00) << 8;
	result |= (big_endian & 0x000000FF) << 24;
	big_endian = result;
}
/// <summary>
/// Reverse the bits in the bitsstream
/// </summary>
void PNG::Converts_To_Little_Endian(uint16_t& big_endian) {
	uint16_t result = 0u;
	result |= (big_endian & 0xFF00) >> 8;
	result |= (big_endian & 0x00FF) << 8;
	big_endian = result;
}
/// <summary>
/// Load the chunk and starts processing the chunk.
/// </summary>
bool PNG::Load_Chunk(std::ifstream& file, std::vector<uint8_t>& compressed_data) {
	/// not usable when the .png was embedded into the .exe file.
	Chunk chunk;

	// meta data 
	file.read((char*)&chunk.m_length, sizeof(chunk.m_length));
	Converts_To_Little_Endian(chunk.m_length);
	file.read(chunk.m_type, 4i64);
	chunk.m_raw_blocks = std::make_unique<uint8_t[]>(chunk.m_length);
	file.read((char*)chunk.m_raw_blocks.get(), chunk.m_length);
	file.read((char*)&chunk.m_crc, sizeof(chunk.m_crc));
	Converts_To_Little_Endian(chunk.m_crc);
	return Process_Chunk(chunk, compressed_data);
}
/// <summary>
/// Process the chunk.
/// </summary>
bool PNG::Process_Chunk(PNG::Chunk& chunk, std::vector<uint8_t>& compressed_data) {
	/// perform differently depending on what type the current chunk is
	struct IHDR {
		unsigned int m_width;
		unsigned int m_height;
		uint8_t m_bit_depth;
		uint8_t m_color_type;
		uint8_t m_compression_method;
		uint8_t m_filter_method;
		uint8_t m_interlace_method;
	};
	bool we_still_have_more_chunk_to_be_processed = true;
	std::string this_chunk_is(chunk.m_type);
	if (this_chunk_is == "IHDR") {
		IHDR ihdr = *reinterpret_cast<IHDR*>(chunk.m_raw_blocks.get());
		PNG::Converts_To_Little_Endian(ihdr.m_width);
		PNG::Converts_To_Little_Endian(ihdr.m_height);
		m_width = ihdr.m_width;
		m_height = ihdr.m_height;
		// ADD VALIDATION
		if (ihdr.m_color_type != 6) { // 6 = RGBA
			throw std::runtime_error("Unsupported color type. Only RGBA (type 6) is supported.");
		}
		if (ihdr.m_bit_depth != 8) { // yes, only 8 bits per channel
			throw std::runtime_error("Unsupported bit depth. Only 8-bit is supported.");
		}
	}
	else if (this_chunk_is == "IDAT") {

		// concatenate onto the existing std::vector
		compressed_data.insert(compressed_data.end(), chunk.m_raw_blocks.get(), chunk.m_raw_blocks.get() + chunk.m_length);
	}
	else if (this_chunk_is == "IEND") {
		we_still_have_more_chunk_to_be_processed = false;
	}
	return we_still_have_more_chunk_to_be_processed;
}
/// <summary>
/// Decompress ALL the blocks (std::vector&lt;uint8_t&gt;).
/// </summary>
std::vector<uint8_t> PNG::Decompress_Blocks(std::vector<uint8_t>& compressed_data) {
	std::vector<uint8_t> decompressed_data;
	bool is_all_the_decompressed_data_retrieved_from_this_chunk = false;
	BitReader<uint16_t> bit_reader(compressed_data.data());
	bit_reader.Forward(16u);
	do {
		size_t bits_processed = 0u;

		// since we dont know the size of each block beforehand
		// thus we reads bit per bit
		// and at the same time decompress it

		// i know this part sucks
		// we are not utilizing the OOP here
		PNG::Chunk::Block block(bit_reader, decompressed_data);
		is_all_the_decompressed_data_retrieved_from_this_chunk = block.m_is_last_block;
	} while (!is_all_the_decompressed_data_retrieved_from_this_chunk);
	return decompressed_data;
}
/// <summary>
/// Apply the filter to each channel.
/// </summary>
void PNG::Apply_Filter(std::vector<uint8_t>& decompressed_data) {
	enum Filter {
		None = 0,
		Sub = 1,
		Up = 2,
		Average = 3,
		Paeth = 4
	};
	const uint8_t BPP = 4u; // RGBA = 4 bytes
	for (unsigned i = 0u; i < m_height; i++) {

		// each row
		auto begin = decompressed_data.begin() + i * (m_width * 4 + 1) + 1;
		auto end = begin + (m_width * 4);
		uint8_t filter = *(begin - 1);
		for (auto u = begin; u != end; ++u) {
			switch (filter) {
			case None:
				break;
			case Sub:
			{
				if (u != begin && u != begin + 1 && u != begin + 2 && u != begin + 3) {
					auto left = u - BPP;
					*u = ((int)*u + (int)*left) & 0xFF;
				}
				break;
			}
			case Up:
			{
				auto up = u - (m_width * BPP + 1);
				*u = ((int)*u + (int)*up) & 0xFF;
				break;
			}
			case Average:
			{
				if (u != begin && u != begin + 1 && u != begin + 2 && u != begin + 3) {
					int left = *(u - 4);
					int up = *(u - (m_width * BPP + 1));
					*u = static_cast<int>(static_cast<int>(*u) + ((left + up) / 2));
					int a = 1;
				}
				else {

					// if we are on the first row
					int up = *(u - (m_width * BPP + 1));
					*u = static_cast<int>(static_cast<int>(*u) + up / 2);
				}
				break;
			}
			case Paeth: // compare nearest three channel: left, up & left-up
			{
				int left = 0;
				int left_up = 0;
				if (u != begin && u != begin + 1 && u != begin + 2 && u != begin + 3) {
					left = *(u - BPP);
					left_up = *(u - (m_width * BPP + 1) - BPP);
				}
				int up = *(u - (m_width * BPP + 1));
				int p = left + up - left_up;
				int p_left = std::abs(p - left);
				int p_up = std::abs(p - up);
				int p_left_up = std::abs(p - left_up);

				if (p_left <= p_up && p_left <= p_left_up) p = left;
				else if (p_up <= p_left_up) p = up;
				else p = left_up;
				*u = (int)(*u + p) & 0xFF;
				break;
			}
			default:
				throw std::runtime_error("Reading invalid filter.");
				break;
			}
		}
	}
}
/// <summary>
/// Validates the signature of .png.
/// </summary>
/// <param name="file"></param>
void PNG::Compare_Signature(std::ifstream& file) {
	uint8_t signature[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
	uint8_t file_signature[8];
	file.read((char*)file_signature, sizeof(file_signature));
	if (not std::equal(std::begin(signature), std::end(signature), std::begin(file_signature))) {
		throw std::runtime_error("Not PNG-file");
	}
}
/// <summary>
/// Create new vector in order to remove the filter information from the old bitsstream (std::vector&lt;uint8_t&gt;).
/// </summary>
void PNG::Load_RGBA(const std::vector<uint8_t>& decompressed_data) {
	m_rgba.resize(decompressed_data.size() - m_height);
	for (unsigned i = 0u; i < m_height; i++) {
		auto begin = decompressed_data.begin() + i * (m_width * 4 + 1) + 1;
		auto end = begin + (m_width * 4);
		auto destination = m_rgba.begin() + i * m_width * 4;
		std::copy(begin, end, destination);
	}
}
