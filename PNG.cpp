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

	for (int i = 0; i < 1; i++) {
		Load_Next_Chunk(file);
	}

	/*
	do {
		Load_Next_Chunk(file);
	} while (m_chunk.m_type != "IEND");
	*/
}

void PNG::Compare_Signature(std::ifstream& file) {
	uint8_t signature[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
	uint8_t file_signature[8];
	file.read((char*)file_signature, sizeof(file_signature));
	if (not std::equal(std::begin(signature), std::end(signature), std::begin(file_signature))) {
		throw std::runtime_error("Not PNG-file");
	}
}

void PNG::Converts_To_Little_Endian(unsigned int& big_endian) {
	unsigned int result = 0u;
	result |= (big_endian & 0xFF000000) >> 24;
	result |= (big_endian & 0x00FF0000) >> 8;
	result |= (big_endian & 0x0000FF00) << 8;
	result |= (big_endian & 0x000000FF) << 24;
	big_endian = result;
}

void PNG::Load_Next_Chunk(std::ifstream& file) {
	file.read((char*)&m_chunk.m_length, sizeof(m_chunk.m_length));
	Converts_To_Little_Endian(m_chunk.m_length);
	file.read(m_chunk.m_type, 4i64);
	m_chunk.m_data = std::make_unique<uint8_t[]>(m_chunk.m_length);
	file.read((char*)m_chunk.m_data.get(), m_chunk.m_length);
	if (std::string(m_chunk.m_type) == "IHDR") {
		m_ihdr = *reinterpret_cast<PNG::IHDR*>(m_chunk.m_data.get());
		Converts_To_Little_Endian(m_ihdr.m_width);
		Converts_To_Little_Endian(m_ihdr.m_height);
	}
	file.read((char*)&m_chunk.m_crc, sizeof(m_chunk.m_crc));
	Converts_To_Little_Endian(m_chunk.m_crc);
}

std::ostream& operator<<(std::ostream& os, const PNG::Chunk& chunk) {
	const size_t MAX_OUTPUT_LENGTH = 10;
	size_t length = chunk.m_length < MAX_OUTPUT_LENGTH ? chunk.m_length : MAX_OUTPUT_LENGTH;
	for (int i = 0; i < length; i++) {
		os << std::to_string(chunk.m_data[i]) << " ";
	}
	return os;
}
