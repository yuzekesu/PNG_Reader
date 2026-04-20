#pragma once
#include <fstream>
#include <memory>
#include <ostream>
#include <vector>
#include "BitReader.h"
#include "PNG.Chunk.h"

// YES, ALL HUFFMAN(usually MSB first) in DEFLATE are stored REVERSED in the bitsstream(LSB first)!
// MSB: where we traverse from the root of the Huffman tree.
// LSB: where we CANNOT traverse from the leafs of the huffman tree.
// 
// 
// The Order Of PNG Is: **************************************
// PNG::PNG(...)						load the bitsstream of the .png
//		PNG::Load_Chunk(...)			load the next chunk in the bitsstream
//			PNG::Process_Chunk(...)		load and save the next block in the chunk into "raw_block" and concatenate them into "compressed_data"
// PNG::Decompress_Blocks(...)			decompress the "compressed_data"
// PNG::Apply_Filter(...)				apply filters to the compressed data
// PNG::Load_RGBA(...)					remove the filter info from the "compressed_data"
// ***********************************************************
class PNG {
public:
	PNG(const wchar_t* file_path);
	PNG(const void* pSysMem);
	static void Converts_To_Little_Endian(unsigned int& big_endian);
	static void Converts_To_Little_Endian(uint16_t& big_endian);
private:
	bool Load_Chunk(std::ifstream& file, std::vector<uint8_t>& compressed_data);
	bool Process_Chunk(::PNG::Chunk& chunk, std::vector<uint8_t>& compressed_data);
	std::vector<uint8_t>Decompress_Blocks(std::vector<uint8_t>& compressed_data);
	void Apply_Filter(std::vector<uint8_t>& decompressed_data);
	void Compare_Signature(std::ifstream& file);
	void Load_RGBA(const std::vector<uint8_t>& decompressed_data);
public:
	std::vector<uint8_t> m_rgba;
	unsigned int m_height = 0u;
	unsigned int m_width = 0u;
};

