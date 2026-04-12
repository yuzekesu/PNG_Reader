#include "Debug.h"
#include "MemoryTracker.h"

void Debug::Chunk(PNG::Chunk& chunk) {
	MemoryTracker tracker(1000);
	if (std::string(chunk.m_type) != "IHDR") {
		Memory<unsigned> m1(chunk.m_length, "Chunk Length", " bytes");
		Memory<char[5]> m2(chunk.m_type, "Chunk Type");
		Memory<PNG::Chunk> m3(chunk, "Chunk Data");
		Memory<unsigned> m4(chunk.m_crc, "Chunk CRC");
		tracker.Add(m1, m2, m3, m4);
	}
	else {
		PNG::Chunk::IHDR ihdr = *reinterpret_cast<PNG::Chunk::IHDR*>(chunk.m_raw_blocks.get());
		PNG::Converts_To_Little_Endian(ihdr.m_width);
		PNG::Converts_To_Little_Endian(ihdr.m_height);
		Memory<unsigned> m1(chunk.m_length, "Chunk Length", " bytes");
		Memory<char[5]> m2(chunk.m_type, "Chunk Type");
		Memory<unsigned> m3(ihdr.m_width, "Image Width");
		Memory<unsigned> m4(ihdr.m_height, "Image Height");
		Memory<uint8_t> m5(ihdr.m_bit_depth, "Image Bit Depth");
		Memory<uint8_t> m6(ihdr.m_color_type, "Image Color Type");
		Memory<uint8_t> m7(ihdr.m_compression_method, "Image Compression Method");
		Memory<uint8_t> m8(ihdr.m_filter_method, "Image Filter Method");
		Memory<uint8_t> m9(ihdr.m_interlace_method, "Image Interlace Method");
		Memory<unsigned> m10(chunk.m_crc, "Chunk CRC");
		tracker.Add(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10);
	}
	Sleep(5000);
}
