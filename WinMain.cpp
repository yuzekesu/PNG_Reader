#include "./code/Displayer.h"
#include "./code/PNG.h"
#include "resource.h"
#include <exception>
#include <sal.h>
#include <string>
#include <windows.h>

int WINAPI wWinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE, _In_ LPWSTR lpCmdLine, _In_ int) {
	try {
		if (std::wstring(lpCmdLine) == L"\0") {
			HRSRC hrsrc = FindResource(hInst, MAKEINTRESOURCE(IDB_PNG1), RT_RCDATA);
			if (!hrsrc) throw std::runtime_error("FindResource failed");
			HGLOBAL hglob = LoadResource(hInst, hrsrc);
			if (!hglob) throw std::runtime_error("LoadResource failed");
			void* ptr = LockResource(hglob);
			if (!ptr) throw std::runtime_error("LockResource failed");
			PNG png(ptr);
			Displayer displayer(png);
		}
		else {
			std::vector<wchar_t> buffer;
			for (int i = 0; lpCmdLine[i] != '\0'; i++) {
				wchar_t c = lpCmdLine[i];
				if (c != '"') {
					buffer.push_back(c);
				}
			}
			buffer.push_back('\0');
			PNG png(buffer.data());
			Displayer displayer(png);
		}
	}
	catch (std::exception e) {
		MessageBoxA(NULL, e.what(), NULL, NULL);
	};
	return 0;
}

