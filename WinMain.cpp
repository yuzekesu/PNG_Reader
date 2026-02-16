#include "Displayer.h"
#include "MemoryTracker.h"
#include "PNG.h"
#include <windows.h>

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	PNG png("red.png");
	Displayer displayer;
	while (true);
	return 0;
}

int main() {
	PNG png("red.png");
	return 0;
}
