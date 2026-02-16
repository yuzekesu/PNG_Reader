#pragma once
#include "PNG.h"
#include <Windows.h>

class Displayer {
public:
	Displayer();
	void Show(PNG png);
	static LRESULT WINAPI Window_Procedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
	void Initialize_DirectX();
private:
	HWND m_window;
};

