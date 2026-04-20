#include "PNG.Chunk.h"
#include <string>

/// <summary>
/// Debugg only.
/// </summary>
std::ostream& PNG::operator<<(std::ostream& os, const PNG::Chunk& chunk) {
	const size_t MAX_OUTPUT_LENGTH = 10;
	size_t length = chunk.m_length < MAX_OUTPUT_LENGTH ? chunk.m_length : MAX_OUTPUT_LENGTH;
	for (int i = 0; i < length; i++) {
		os << std::to_string(chunk.m_raw_blocks[i]) << " ";
	}
	return os;
}
