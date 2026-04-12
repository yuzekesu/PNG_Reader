#include "BitReader.h"

template class BitReader<uint64_t>;
template class BitReader<uint32_t>;
template class BitReader<uint16_t>;
template class BitReader<uint8_t>;

/// <summary>
/// Constructs BitReader with existing bitsstream (uint8_t*).
/// </summary>
template<typename output_t>
BitReader<output_t>::BitReader(const uint8_t* data) { m_data = data; }

/// <summary>
/// Reads bits from right to left.
/// </summary>
template<typename output_t>
output_t BitReader<output_t>::Read(size_t size) {
	output_t result = Peak(size);
	Forward(size);
	return result;
}

/// <summary>
/// Peaks bits from LSB to MSB.
/// </summary>
template<typename output_t>
output_t BitReader<output_t>::Peak(size_t size) {
	if (size > m_maxSize) {
		throw std::runtime_error("Too big reading size for BitReader.");
	}
	uint16_t result = 0u;
	for (size_t i = 0; i < size; i++) {
		const size_t byte_offset = (m_offset + i) / 8u;
		const size_t bit_offset_in_that_byte = (m_offset + i) % 8u;
		const output_t current_bit = 1u << i;
		const uint8_t current_bit_in_that_byte = 1u << bit_offset_in_that_byte;
		if (m_data[byte_offset] & current_bit_in_that_byte) {
			result |= current_bit;
		}
	}
	return result;
}

/// <summary>
/// Returns the current offset.
/// </summary>
template<typename output_t>
size_t BitReader<output_t>::Has_Read() {
	return m_offset;
}

/// <summary>
/// Aligns the offset to the byte using cielling.
/// </summary>
template<typename output_t>
void BitReader<output_t>::Align() {
	size_t rest = m_offset % 8u;
	if (rest) {
		m_offset += 8u - rest;
	}
}

/// <summary>
/// Increases the current offset.
/// </summary>
template<typename output_t>
void BitReader<output_t>::Forward(size_t size) {
	m_offset += size;
}
