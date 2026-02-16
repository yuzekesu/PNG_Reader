#include "Displayer.h"

Displayer::Displayer() {
	WNDCLASS wc{};
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpfnWndProc = Displayer::Window_Procedure;
	wc.lpszClassName = L"hehe";
	wc.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&wc);
	int x = GetSystemMetrics(SM_CXSCREEN);
	int y = GetSystemMetrics(SM_CYSCREEN);
	int width = x / 2;
	int height = y / 2;
	x = x * 1 / 4;
	y = y * 1 / 4;
	m_window = CreateWindowW(L"hehe", L"PNG_Reader", WS_POPUPWINDOW, x, y, width, height, NULL, NULL, wc.hInstance, NULL);
	ShowWindow(m_window, SW_SHOW);
}

void Displayer::Show(PNG png) {
}

LRESULT __stdcall Displayer::Window_Procedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Displayer::Initialize_DirectX() {
}
