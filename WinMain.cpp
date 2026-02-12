#include "MemoryTracker.h"
#include "PNG.h"
#include <cstdint>
#include <string>
#include <windows.h>

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	MemoryTracker tracker(1000);
	PNG png("red.png");
	if (std::string(png.m_chunk.m_type) != "IHDR") {
		Memory<unsigned> m1(png.m_chunk.m_length, "Chunk Length", " bytes");
		Memory<char[5]> m2(png.m_chunk.m_type, "Chunk Type");
		Memory<PNG::Chunk> m3(png.m_chunk, "Chunk Data");
		Memory<unsigned> m4(png.m_chunk.m_crc, "Chunk CRC");
		tracker.Add(m1, m2, m3, m4);
	}
	else {
		Memory<unsigned> m1(png.m_chunk.m_length, "Chunk Length", " bytes");
		Memory<char[5]> m2(png.m_chunk.m_type, "Chunk Type");
		Memory<unsigned> m3(png.m_ihdr.m_width, "Image Width");
		Memory<unsigned> m4(png.m_ihdr.m_height, "Image Height");
		Memory<uint8_t> m5(png.m_ihdr.m_bit_depth, "Image Bit Depth");
		Memory<uint8_t> m6(png.m_ihdr.m_color_type, "Image Color Type");
		Memory<uint8_t> m7(png.m_ihdr.m_compression_method, "Image Compression Method");
		Memory<uint8_t> m8(png.m_ihdr.m_filter_method, "Image Filter Method");
		Memory<uint8_t> m9(png.m_ihdr.m_interlace_method, "Image Interlace_method");
		Memory<unsigned> m10(png.m_chunk.m_crc, "Chunk CRC: ");
		tracker.Add(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10);
	}

	while (true);
	return 0;
}

