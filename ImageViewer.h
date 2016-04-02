#pragma once

#include "Resource.h"

const float DEFAULT_DPI = 96.f;   // Default DPI that maps image resolution directly to screen resoltuion
const unsigned int MAX_LOADSTRING = 100;
const size_t MAX_FILENAME_LENGTH = 32768;

class ImageViewer
{
public:
	ImageViewer();
	~ImageViewer();

	HRESULT Initialize(HINSTANCE hInstance, int nCmdShow);

private:
	bool LocateImageFile();
	bool DragProc(WPARAM wParam);

	HRESULT CreateDeviceResources();
	HRESULT RenderImage();

	static LRESULT CALLBACK s_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnPaint();

private:
	WCHAR m_szTitle[MAX_LOADSTRING];
	WCHAR m_szWindowClassName[MAX_LOADSTRING];
	DWORD     m_dwExStyle;
	DWORD     m_dwStyle;

	std::wstring m_szFileName;

	HINSTANCE               m_hInst;
	HWND					m_hWnd;
	IWICImagingFactory     *m_pIWICFactory;

	ID2D1Factory           *m_pD2DFactory;
	ID2D1HwndRenderTarget  *m_pRT;
	ID2D1Bitmap            *m_pD2DBitmap;
	IWICFormatConverter    *m_pConvertedSourceBitmap;
};