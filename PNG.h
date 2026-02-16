#pragma once
#include <fstream>
#include <memory>
#include <ostream>
#include <queue>
#include <vector>
class PNG {
public:
	struct Chunk {
		struct IHDR {
			unsigned int m_width;
			unsigned int m_height;
			uint8_t m_bit_depth;
			uint8_t m_color_type;
			uint8_t m_compression_method;
			uint8_t m_filter_method;
			uint8_t m_interlace_method;
		};
		struct Block {
			class BitReader {
			public:
				BitReader() = delete;
				BitReader(const uint8_t* data);;
				uint16_t Read(size_t size);
				uint16_t Peak(size_t size);
				size_t Has_Read();
				void Align();
				void Forward(size_t size);
			private:
				size_t m_offset = 0u;
				const uint8_t* m_data;
			};
			enum CompressionType {
				UNCOMPRESSED,
				FIXED_HUFFMAN_CODES,
				DYNAMIC_HUFFMAN_CODE
			};
			bool m_is_last_block;
			CompressionType m_type;
		public:
			Block() = delete;
			Block(PNG::Chunk::Block::BitReader& bit_reader, std::vector<uint8_t>& output);
			void Decompress_Block_Dynamic_Huffman(PNG::Chunk::Block::BitReader& bit_reader, std::vector<uint8_t>& output);
			void Decompress_Block_Fixed_Huffman(PNG::Chunk::Block::BitReader& bit_reader, std::vector<uint8_t>& output);
			void Decompress_Block_That_Is_Uncompressed(PNG::Chunk::Block::BitReader& bit_reader, std::vector<uint8_t>& output);
		};
		unsigned int m_length;
		char m_type[5]{}; // null-terminated character
		std::unique_ptr<uint8_t[]> m_raw_blocks;
		unsigned int m_crc;
	public:
		friend std::ostream& operator<<(std::ostream& os, const PNG::Chunk& chunk);
	};
public:
	PNG(const char* file_path);
	static void Converts_To_Little_Endian(unsigned int& big_endian);
	static void Converts_To_Little_Endian(uint16_t& big_endian);
private:
	bool Load_Next_Chunk(std::ifstream& file);
	bool Process_Chunk(PNG::Chunk& chunk);
	void Apply_Filter();
	void Compare_Signature(std::ifstream& file);
public:
	std::vector<uint8_t> m_decompressed_data;
	std::vector<uint8_t> m_rgba;
	unsigned int m_height = 0u;
	unsigned int m_width = 0u;
};

