#pragma once
#include <fstream>
#include <memory>
#include <ostream>
class PNG {
public:
	struct Chunk {
		unsigned int m_length;
		char m_type[5]{}; // null-terminated character
		std::unique_ptr<uint8_t[]> m_data;
		unsigned int m_crc;
	public:
		friend std::ostream& operator<<(std::ostream& os, const PNG::Chunk& chunk);
	};
	struct IHDR {
		unsigned int m_width;
		unsigned int m_height;
		uint8_t m_bit_depth;
		uint8_t m_color_type;
		uint8_t m_compression_method;
		uint8_t m_filter_method;
		uint8_t m_interlace_method;
	};
public:
	PNG(const char* file_path);
private:
	void Compare_Signature(std::ifstream& file);
	void Converts_To_Little_Endian(unsigned int& big_endian);
	void Load_Next_Chunk(std::ifstream& file);
public:
	Chunk m_chunk;
	IHDR m_ihdr;
};

