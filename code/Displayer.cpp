#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#include "Displayer.h"
#include "PNG.h"
#include "shader_in_char_star.h"
#include <Windows.h>
#include <d3d11.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <dxgiformat.h>
#include <dxgitype.h>
#include <libloaderapi.h>
#include <stdexcept>
#include <wrl/client.h>

Displayer::Displayer() {
	m_x = GetSystemMetrics(SM_CXSCREEN);
	m_y = GetSystemMetrics(SM_CYSCREEN);
	m_width = m_x / 2;
	m_height = m_y / 2;
	m_x = m_x * 1 / 4;
	m_y = m_y * 1 / 4;
	Initialize_Window();
	Initialize_DirectX();
	Message_Loop();
}

Displayer::Displayer(PNG& png) {
	Calculate_Window_Size(png);
	Initialize_Window();
	Initialize_DirectX();
	Show(png);
	Message_Loop();
}

LRESULT __stdcall Displayer::Window_Procedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			PostQuitMessage(0);
		}
		break;
	case WM_RBUTTONDOWN:
		if (MessageBox(NULL, L"Exit the PNG_Reader ?", L"Ask", MB_YESNO) == IDYES) {
			PostQuitMessage(0);
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Displayer::Calculate_Window_Size(PNG& png) {
	unsigned width_screen = GetSystemMetrics(SM_CXSCREEN);
	unsigned height_screen = GetSystemMetrics(SM_CYSCREEN);
	unsigned numerator = 1;
	unsigned denominator = 1;
	while (png.m_height * numerator < height_screen && png.m_width * numerator < width_screen) {
		numerator++;
	}
	while (png.m_width * numerator / denominator > width_screen || png.m_height * numerator / denominator > height_screen) {
		denominator++;
	}
	m_width = png.m_width * numerator / denominator;
	m_height = png.m_height * numerator / denominator;
	m_x = (width_screen - m_width) / 2;
	m_y = (height_screen - m_height) / 2;
}

void Displayer::Initialize_DirectX() {
	using namespace Microsoft::WRL;
	HRESULT hr;

	// swapchain
	DXGI_SWAP_CHAIN_DESC desc_swapchain{};
	desc_swapchain.BufferCount = 1;
	DXGI_MODE_DESC buffer_swapchain{};
	buffer_swapchain.Width = 0u;
	buffer_swapchain.Height = 0u;
	buffer_swapchain.RefreshRate.Denominator = 1u;
	buffer_swapchain.RefreshRate.Numerator = 1u;
	buffer_swapchain.Scaling = DXGI_MODE_SCALING_CENTERED;
	buffer_swapchain.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	buffer_swapchain.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc_swapchain.BufferDesc = buffer_swapchain;
	desc_swapchain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc_swapchain.Flags = 0;
	desc_swapchain.OutputWindow = m_window;
	desc_swapchain.SampleDesc.Count = 1;
	desc_swapchain.SampleDesc.Quality = 0;
	desc_swapchain.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	desc_swapchain.Windowed = true;
	hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_DEBUG, NULL, 0, D3D11_SDK_VERSION, &desc_swapchain, swapchain.ReleaseAndGetAddressOf(), device.ReleaseAndGetAddressOf(), NULL, context.ReleaseAndGetAddressOf());
	if (FAILED(hr)) { throw std::runtime_error("HRESULT problem"); }

	// render target view
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;
	ComPtr<ID3D11Texture2D> back_buffer;
	hr = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)back_buffer.GetAddressOf());
	if (FAILED(hr)) { throw std::runtime_error("HRESULT problem"); }
	hr = device->CreateRenderTargetView(back_buffer.Get(), NULL, rtv.ReleaseAndGetAddressOf());
	if (FAILED(hr)) { throw std::runtime_error("HRESULT problem"); }
	context->OMSetRenderTargets(1u, rtv.GetAddressOf(), NULL);

	// viewport
	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Height = static_cast<float>(m_height);
	viewport.Width = static_cast<float>(m_width);
	viewport.MaxDepth = 1.f;
	viewport.MinDepth = 0.f;
	context->RSSetViewports(1, &viewport);

	// vertex- || pixel- shader
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> ps;
	ComPtr<ID3DBlob> blob_vs, blob_ps, error;
	hr = D3DCompile(VertexShader, sizeof(VertexShader), NULL, NULL, NULL, "Vertex_Shader", "vs_5_0", 0, 0, blob_vs.ReleaseAndGetAddressOf(), error.ReleaseAndGetAddressOf());
	if (FAILED(hr)) { throw std::runtime_error("HRESULT problem"); }
	hr = device->CreateVertexShader(blob_vs->GetBufferPointer(), blob_vs->GetBufferSize(), NULL, vs.ReleaseAndGetAddressOf());
	if (FAILED(hr)) { throw std::runtime_error("HRESULT problem"); }
	context->VSSetShader(vs.Get(), NULL, 0);
	hr = D3DCompile(PixelShader, sizeof(PixelShader), NULL, NULL, NULL, "Pixel_Shader", "ps_5_0", 0, 0, blob_ps.ReleaseAndGetAddressOf(), error.ReleaseAndGetAddressOf());
	if (FAILED(hr)) { throw std::runtime_error("HRESULT problem"); }
	hr = device->CreatePixelShader(blob_ps->GetBufferPointer(), blob_ps->GetBufferSize(), NULL, ps.ReleaseAndGetAddressOf());
	if (FAILED(hr)) { throw std::runtime_error("HRESULT problem"); }
	context->PSSetShader(ps.Get(), NULL, 0);

	// input layout
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputlayout;
	D3D11_INPUT_ELEMENT_DESC input_desc_1{};
	input_desc_1.AlignedByteOffset = 0;
	input_desc_1.Format = DXGI_FORMAT_R32G32_FLOAT;
	input_desc_1.InputSlot = 0;
	input_desc_1.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	input_desc_1.InstanceDataStepRate = 0;
	input_desc_1.SemanticIndex = 0;
	input_desc_1.SemanticName = "POSITION";
	D3D11_INPUT_ELEMENT_DESC input_desc_2{};
	input_desc_2.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	input_desc_2.Format = DXGI_FORMAT_R32G32_FLOAT;
	input_desc_2.InputSlot = 0;
	input_desc_2.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	input_desc_2.InstanceDataStepRate = 0;
	input_desc_2.SemanticIndex = 0;
	input_desc_2.SemanticName = "TEXCOORD";
	D3D11_INPUT_ELEMENT_DESC input_descs[2] = { input_desc_1, input_desc_2 };
	hr = device->CreateInputLayout(input_descs, sizeof(input_descs) / sizeof(input_descs[0]), blob_vs->GetBufferPointer(), blob_vs->GetBufferSize(), inputlayout.ReleaseAndGetAddressOf());
	if (FAILED(hr)) { throw std::runtime_error("HRESULT problem"); }
	context->IASetInputLayout(inputlayout.Get());

	// vertex buffer
	Microsoft::WRL::ComPtr<ID3D11Buffer> vbuffer;
	struct Input {
		float pos[2]; // x, y
		float tex[2]; // u, v
	};
	Input input_0 = { {-1.f,1.f}, {0.f, 0.f} }; // left-top
	Input input_1 = { {-1.f,-1.f}, {0.f, 1.f} }; // left-bottom
	Input input_2 = { {1.f,-1.f}, {1.f, 1.f} }; // right-bottom
	Input input_3 = { {1.f,1.f}, {1.f, 0.f} }; // right-top
	Input inputs[4] = { input_0, input_1, input_2, input_3 };
	D3D11_BUFFER_DESC vbuffer_desc{};
	vbuffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbuffer_desc.ByteWidth = sizeof(inputs);
	vbuffer_desc.CPUAccessFlags = 0;
	vbuffer_desc.MiscFlags = 0;
	vbuffer_desc.StructureByteStride = 0;
	vbuffer_desc.Usage = D3D11_USAGE_DEFAULT;
	D3D11_SUBRESOURCE_DATA vdata{};
	vdata.pSysMem = inputs;
	hr = device->CreateBuffer(&vbuffer_desc, &vdata, vbuffer.ReleaseAndGetAddressOf());
	if (FAILED(hr)) { throw std::runtime_error("HRESULT problem"); }
	UINT stride = sizeof(inputs[0]);
	UINT offset = 0u;
	context->IASetVertexBuffers(0, 1, vbuffer.GetAddressOf(), &stride, &offset);

	// index buffer
	Microsoft::WRL::ComPtr<ID3D11Buffer> ibuffer;
	unsigned indices[6]{ 0u, 3u, 2u, 2u, 1u, 0u }; // clockwise
	D3D11_BUFFER_DESC ibuffer_desc{};
	ibuffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibuffer_desc.ByteWidth = sizeof(indices);
	ibuffer_desc.CPUAccessFlags = 0;
	ibuffer_desc.MiscFlags = 0;
	ibuffer_desc.StructureByteStride = 0;
	ibuffer_desc.Usage = D3D11_USAGE_DEFAULT;
	D3D11_SUBRESOURCE_DATA idata{};
	idata.pSysMem = indices;
	hr = device->CreateBuffer(&ibuffer_desc, &idata, ibuffer.ReleaseAndGetAddressOf());
	if (FAILED(hr)) { throw std::runtime_error("HRESULT problem"); }
	context->IASetIndexBuffer(ibuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// sample state
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
	D3D11_SAMPLER_DESC desc_samp{};
	desc_samp.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	desc_samp.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	desc_samp.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	desc_samp.BorderColor[0] = 0;
	desc_samp.BorderColor[1] = 0;
	desc_samp.BorderColor[2] = 0;
	desc_samp.BorderColor[3] = 0;
	desc_samp.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	//desc_samp.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	desc_samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	desc_samp.MaxAnisotropy = 1;
	desc_samp.MaxLOD = D3D11_MIP_LOD_BIAS_MAX;
	desc_samp.MinLOD = 0;
	desc_samp.MipLODBias = 0.0f;
	hr = device->CreateSamplerState(&desc_samp, sampler.ReleaseAndGetAddressOf());
	if (FAILED(hr)) { throw std::runtime_error("HRESULT problem"); }
	context->PSSetSamplers(0, 1, sampler.GetAddressOf());

}

void Displayer::Initialize_Window() {
	WNDCLASS wc{};
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpfnWndProc = Displayer::Window_Procedure;
	wc.lpszClassName = L"hehe";
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hCursor = LoadCursor(NULL, IDC_CROSS);
	RegisterClass(&wc);
	RECT r{};
	r.right = m_width;
	r.bottom = m_height;
	AdjustWindowRect(&r, WS_POPUPWINDOW, false);
	m_window = CreateWindowW(L"hehe", L"PNG_Reader", WS_POPUPWINDOW, m_x, m_y, r.right - r.left, r.bottom - r.top, NULL, NULL, wc.hInstance, NULL);
	DragAcceptFiles(m_window, true);
	ShowWindow(m_window, SW_SHOW);
}

void Displayer::Message_Loop() {
	MSG msg;
	while (GetMessage(&msg, NULL, NULL, NULL)) {
		DispatchMessage(&msg);
		if (msg.message == WM_DROPFILES) {
			wchar_t buffer[100];
			HDROP drop = reinterpret_cast<HDROP>(msg.wParam);
			DragQueryFile(drop, 0, buffer, 100);
			DragFinish(drop);
			PNG png(buffer);
			Calculate_Window_Size(png);
			Resize();
			Show(png);
		}
	}
}

void Displayer::Resize() {
	D3D11_VIEWPORT vp{};
	vp.Height = m_height;
	vp.MaxDepth = 1.f;
	vp.MinDepth = 0.f;
	vp.Width = m_width;
	context->RSSetViewports(1, &vp);

	// Resize swapchain buffers to match new window size
	HRESULT hr = swapchain->ResizeBuffers(0, m_width, m_height, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(hr)) { throw std::runtime_error("Failed to resize swapchain buffers"); }

	// Recreate render target view with new back buffer
	Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer;
	hr = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)back_buffer.GetAddressOf());
	if (FAILED(hr)) { throw std::runtime_error("Failed to get back buffer"); }

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;
	hr = device->CreateRenderTargetView(back_buffer.Get(), nullptr, rtv.ReleaseAndGetAddressOf());
	if (FAILED(hr)) { throw std::runtime_error("Failed to create RTV"); }

	context->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);

	RECT r{};
	r.right = m_width;
	r.bottom = m_height;
	AdjustWindowRect(&r, WS_POPUPWINDOW, false);
	SetWindowPos(m_window, NULL, m_x, m_y, r.right - r.left, r.bottom - r.top, SWP_SHOWWINDOW);
}

void Displayer::Show(PNG& png) {
	HRESULT hr;
	using namespace Microsoft::WRL;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
	D3D11_TEXTURE2D_DESC desc_tex{};
	desc_tex.ArraySize = 1;
	desc_tex.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc_tex.CPUAccessFlags = 0;
	desc_tex.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc_tex.Height = png.m_height;
	desc_tex.MipLevels = 0;
	desc_tex.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	desc_tex.SampleDesc.Count = 1;
	desc_tex.SampleDesc.Quality = 0;
	desc_tex.Usage = D3D11_USAGE_DEFAULT;
	desc_tex.Width = png.m_width;
	D3D11_SUBRESOURCE_DATA data{};
	data.pSysMem = png.m_rgba.data();
	data.SysMemPitch = png.m_width * 4u;
	hr = device->CreateTexture2D(&desc_tex, NULL, tex.ReleaseAndGetAddressOf());
	if (FAILED(hr)) { throw std::runtime_error("HRESULT problem"); }
	context->UpdateSubresource(tex.Get(), 0u, NULL, png.m_rgba.data(), png.m_width * 4, 0);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
	D3D11_SHADER_RESOURCE_VIEW_DESC desc_srv{};
	desc_srv.Format = desc_tex.Format;
	desc_srv.Texture2D.MipLevels = -1;
	desc_srv.Texture2D.MostDetailedMip = 0;
	desc_srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	hr = device->CreateShaderResourceView(tex.Get(), &desc_srv, srv.ReleaseAndGetAddressOf());
	if (FAILED(hr)) { throw std::runtime_error("HRESULT problem"); }
	context->GenerateMips(srv.Get());
	context->PSSetShaderResources(0, 1, srv.GetAddressOf());

	context->DrawIndexed(6, 0, 0);
	hr = swapchain->Present(1, 0);
	if (FAILED(hr)) { throw std::runtime_error("HRESULT problem"); }

}

