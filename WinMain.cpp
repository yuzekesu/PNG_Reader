#include "Displayer.h"
#include "PNG.h"
#include "resource.h"
#include <cstdint>
#include <sal.h>
#include <string>
#include <windows.h>

#ifdef NODEBUG
#define MODE_IS std::string(lpCmdLine) == "\0"
#else
#define MODE_IS 0
#endif



int WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE, _In_ LPSTR lpCmdLine, _In_ int) {
	if (MODE_IS) {
		HRSRC hrsrc = FindResource(hInst, MAKEINTRESOURCE(IDB_PNG1), RT_RCDATA);
		if (!hrsrc) throw std::runtime_error("FindResource failed");
		HGLOBAL hglob = LoadResource(hInst, hrsrc);
		if (!hglob) throw std::runtime_error("LoadResource failed");
		DWORD size = SizeofResource(hInst, hrsrc);
		if (!size) throw std::runtime_error("SizeofResource failed");
		void* ptr = LockResource(hglob);
		if (!ptr) throw std::runtime_error("LockResource failed");
		PNG png(ptr);
		Displayer displayer(png);
	}
	else {
		//PNG png("./image/1.png");
		//PNG png("./image/windows.png");
		//PNG png("./image/fox.png");
		PNG png("./image/vivado.png");
		Displayer displayer(png);

	}
	return 0;
}

