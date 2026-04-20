#pragma once
#include <cstdint>

/// <summary>
/// Reads bits from a bitsstream (uint8_t*) in LSB first order.
/// </summary>
/// <typeparam name="output_t"></typeparam>
template<typename output_t>
class BitReader {
public:
	BitReader() = delete;
	BitReader(const uint8_t* data);
	output_t Read(size_t size);
	output_t Peak(size_t size);
	size_t Has_Read();
	void Align();
	void Forward(size_t size);
private:
	size_t m_offset = 0u;
	const uint8_t* m_data;
	const size_t m_maxSize = sizeof(output_t) * 8u;
};

