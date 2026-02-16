#pragma once
#include "PNG.h"
#include <Windows.h>
#include <d3d11.h>
#include <wrl.h>

class Displayer {
public:
	Displayer();
	Displayer(PNG& png);
	static LRESULT WINAPI Window_Procedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
	void Initialize_DirectX();
	void Initialize_Window();
	void Message_Loop();
	void Show(PNG& png);
private:
	unsigned m_x, m_y, m_width, m_height;
	HWND m_window;
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapchain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> ps;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vbuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> ibuffer;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputlayout;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
};

