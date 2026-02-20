#include "Displayer.h"
#include "PNG.h"
#include <cstdint>
#include <sal.h>
#include <windows.h>

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	PNG png("./image/1.png");
	// PNG png("./image/1.png");
	// PNG png("./image/vivado.png");
	Displayer displayer(png);
	uint8_t temp = *(png.m_rgba.end() - 1);
	return 0;
}

