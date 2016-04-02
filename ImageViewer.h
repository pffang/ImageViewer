#pragma once

#include "Resource.h"

const float DEFAULT_DPI = 96.f;   // Default DPI that maps image resolution directly to screen resoltuion
const unsigned int MAX_LOADSTRING = 100;

class ImageViewer
{
public:
	ImageViewer();
	~ImageViewer();

	HRESULT Initialize(HINSTANCE hInstance, int nCmdShow);

private:
	HRESULT CreateD2DBitmapFromFile(HWND hWnd);
	bool LocateImageFile(HWND hWnd, LPWSTR pszFileName, DWORD cbFileName);
	HRESULT CreateDeviceResources(HWND hWnd);

	
	LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnPaint(HWND hWnd);

	static LRESULT CALLBACK s_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	WCHAR m_szTitle[MAX_LOADSTRING];
	WCHAR m_szWindowClassName[MAX_LOADSTRING];
	DWORD     m_dwExStyle;
	DWORD     m_dwStyle;

	HINSTANCE               m_hInst;
	IWICImagingFactory     *m_pIWICFactory;

	ID2D1Factory           *m_pD2DFactory;
	ID2D1HwndRenderTarget  *m_pRT;
	ID2D1Bitmap            *m_pD2DBitmap;
	IWICFormatConverter    *m_pConvertedSourceBitmap;
};