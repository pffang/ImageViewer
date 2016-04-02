// ImageViewer.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "ImageViewer.h"

template <typename Type>
inline void SafeRelease(Type *&p)
{
	if (nullptr != p)
	{
		p->Release();
		p = nullptr;
	}
}

INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	/*UNREFERENCED_PARAMETER(nCmdShow);*/

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_IMAGEVIEWER));
	HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);

	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		ImageViewer oViewer;
		HRESULT hr = oViewer.Initialize(hInstance, nCmdShow);

		if (SUCCEEDED(hr))
		{
			BOOL fRet;
			MSG msg;

			// Main message loop:
			while ((fRet = GetMessage(&msg, nullptr, 0, 0)) != 0)
			{
				if (fRet == -1)
				{
					break;
				}
				else
				{
					if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
			}
		}
		CoUninitialize();
	}
	else
	{
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

ImageViewer::ImageViewer()
{
	m_hInst = nullptr;
	m_hWnd = nullptr;

	m_dwExStyle = WS_EX_ACCEPTFILES;
	m_dwStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	m_pD2DBitmap = nullptr;
	m_pConvertedSourceBitmap = nullptr;
	m_pIWICFactory = nullptr;
	m_pD2DFactory = nullptr;
	m_pRT = nullptr;
}

ImageViewer::~ImageViewer()
{
	SafeRelease(m_pD2DBitmap);
	SafeRelease(m_pConvertedSourceBitmap);
	SafeRelease(m_pIWICFactory);
	SafeRelease(m_pD2DFactory);
	SafeRelease(m_pRT);
}

HRESULT ImageViewer::Initialize(HINSTANCE hInstance, int nCmdShow)
{
	HRESULT hr = S_OK;

	// Create WIC factory
	hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&m_pIWICFactory)
	);

	if (SUCCEEDED(hr))
	{
		// Create D2D factory
		hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
	}

	if (SUCCEEDED(hr))
	{
		LoadStringW(hInstance, IDS_APP_TITLE, m_szTitle, MAX_LOADSTRING);
		LoadStringW(hInstance, IDC_IMAGEVIEWER, m_szWindowClassName, MAX_LOADSTRING);

		WNDCLASSEX wcex;

		// Register window class
		wcex.cbSize = sizeof(WNDCLASSEX);

		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = s_WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_IMAGEVIEWER));
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_IMAGEVIEWER);
		wcex.lpszClassName = m_szWindowClassName;
		wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

		m_hInst = hInstance;

		hr = RegisterClassEx(&wcex) ? S_OK : E_FAIL;
	}

	if (SUCCEEDED(hr))
	{
		// Create window
		HWND hWnd = CreateWindowExW(m_dwExStyle, m_szWindowClassName, m_szTitle, m_dwStyle,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, hInstance, this);
		if (hWnd)
		{
			hr = S_OK;
			m_hWnd = hWnd;
			ShowWindow(hWnd, nCmdShow);
			UpdateWindow(hWnd);
		}
		else
		{
			hr = E_FAIL;
		}
	}

	return hr;
}

LRESULT ImageViewer::s_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ImageViewer *pThis;
	LRESULT lRet = 0;

	if (uMsg == WM_NCCREATE)
	{
		auto pcs = reinterpret_cast<LPCREATESTRUCT> (lParam);
		pThis = reinterpret_cast<ImageViewer *> (pcs->lpCreateParams);

		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR> (pThis));
		lRet = DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	else
	{
		pThis = reinterpret_cast<ImageViewer *> (GetWindowLongPtr(hWnd, GWLP_USERDATA));
		if (pThis)
		{
			lRet = pThis->WndProc(uMsg, wParam, lParam);
		}
		else
		{
			lRet = DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
	}
	return lRet;
}

LRESULT ImageViewer::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		switch (wmId)
		{
		case IDM_FILE:
		{
			if (LocateImageFile() && SUCCEEDED(RenderImage()))
			{
				InvalidateRect(m_hWnd, nullptr, TRUE);
			}
			else
			{
				MessageBox(m_hWnd, L"Failed to load image, select a new one.", L"Application Error", MB_ICONEXCLAMATION | MB_OK);
			}
		}
		break;
		case IDM_ABOUT:
			DialogBox(m_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), m_hWnd, About);
			break;
		case IDM_EXIT:
			PostMessage(m_hWnd, WM_CLOSE, 0, 0);
			break;
		default:
			return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
		}
	}
	break;
	case WM_DROPFILES:
	{
		if (DragProc(wParam) && SUCCEEDED(RenderImage()))
		{
			InvalidateRect(m_hWnd, nullptr, TRUE);
		}
		else
		{
			MessageBox(m_hWnd, L"Failed to load image, select a new one.", L"Application Error", MB_ICONEXCLAMATION | MB_OK);
		}
	}
	break;
	case WM_SIZE:
	{
		auto size = D2D1::SizeU(LOWORD(lParam), HIWORD(lParam));

		if (m_pRT)
		{
			// If we couldn't resize, release the device and we'll recreate it
			// during the next render pass.
			if (FAILED(m_pRT->Resize(size)))
			{
				SafeRelease(m_pRT);
				SafeRelease(m_pD2DBitmap);
			}
		}
	}
	break;
	case WM_PAINT:
	{
		return OnPaint();
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

HRESULT ImageViewer::RenderImage()
{
	HRESULT hr = S_OK;
	// Create a decoder
	IWICBitmapDecoder *pDecoder = nullptr;

	hr = m_pIWICFactory->CreateDecoderFromFilename(
		m_szFileName.c_str(),                      // Image to be decoded
		nullptr,                         // Do not prefer a particular vendor
		GENERIC_READ,                    // Desired read access to the file
		WICDecodeMetadataCacheOnDemand,  // Cache metadata when needed
		&pDecoder                        // Pointer to the decoder
	);

	// Retrieve the first frame of the image from the decoder
	IWICBitmapFrameDecode *pFrame = nullptr;

	if (SUCCEEDED(hr))
	{
		hr = pDecoder->GetFrame(0, &pFrame);
	}

	if (SUCCEEDED(hr))
	{
		RECT rc = { 0 };
		BOOL ret = TRUE;

		unsigned int nImageWidth, nImageHeight;
		pFrame->GetSize(&nImageWidth, &nImageHeight);

		rc.right = rc.left + nImageWidth;
		rc.bottom = rc.top + nImageHeight;
		ret = AdjustWindowRectEx(&rc, m_dwStyle, TRUE, m_dwExStyle);
		assert(ret);

		int nWindowWidth = rc.right - rc.left;
		int nWindowHeight = rc.bottom - rc.top;
		int nBorderWidth = nWindowWidth - nImageWidth;
		int nBorderHeight = nWindowHeight - nImageHeight;

		RECT rcWorkArea = { 0 };
		SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);

		const int nWorkAreaWidth = rcWorkArea.right - rcWorkArea.left;
		const int nWorkAreaHeight = rcWorkArea.bottom - rcWorkArea.top;

		ret = GetWindowRect(m_hWnd, &rc);
		assert(ret);
		if (nWindowHeight < nWorkAreaHeight && nWindowWidth < nWorkAreaWidth)
		{
			if (nWindowHeight < nWorkAreaHeight)
				rc.top = rcWorkArea.top;
			if (nWindowWidth < nWorkAreaWidth)
				rc.left = rcWorkArea.left;
		}
		else
		{
			rc.top = rcWorkArea.top;
			rc.left = rcWorkArea.left;
			if (nWindowHeight > nWorkAreaHeight)
			{
				nWindowHeight = nWorkAreaHeight;
				double fWindowWidth = (double)nImageWidth / (double)nImageHeight * ((double)nWindowHeight - (double)nBorderHeight) + (double)nBorderWidth;
				nWindowWidth = (int)fWindowWidth;
			}
			if (nWindowWidth > nWorkAreaWidth)
			{
				nWindowWidth = nWorkAreaWidth;
				double fWindowHeight = (double)nImageHeight / (double)nImageWidth * ((double)nWindowWidth - (double)nBorderWidth) + (double)nBorderHeight;
				nWindowHeight = (int)fWindowHeight;
			}
		}

		ret = MoveWindow(m_hWnd, rc.left, rc.top, nWindowWidth, nWindowHeight, TRUE);
		assert(ret);
	}

	//Format convert the frame to 32bppPBGRA
	if (SUCCEEDED(hr))
	{
		SafeRelease(m_pConvertedSourceBitmap);
		hr = m_pIWICFactory->CreateFormatConverter(&m_pConvertedSourceBitmap);
	}

	if (SUCCEEDED(hr))
	{
		hr = m_pConvertedSourceBitmap->Initialize(
			pFrame,                          // Input bitmap to convert
			GUID_WICPixelFormat32bppPBGRA,   // Destination pixel format
			WICBitmapDitherTypeNone,         // Specified dither pattern
			nullptr,                         // Specify a particular palette 
			0.f,                             // Alpha threshold
			WICBitmapPaletteTypeCustom       // Palette translation type
		);
	}

	//Create render target and D2D bitmap from IWICBitmapSource
	if (SUCCEEDED(hr))
	{
		hr = CreateDeviceResources();
	}

	if (SUCCEEDED(hr))
	{
		// Need to release the previous D2DBitmap if there is one
		SafeRelease(m_pD2DBitmap);
		hr = m_pRT->CreateBitmapFromWicBitmap(m_pConvertedSourceBitmap, nullptr, &m_pD2DBitmap);
	}

	SafeRelease(pDecoder);
	SafeRelease(pFrame);

	return hr;
}

bool ImageViewer::LocateImageFile()
{
	m_szFileName = L"\\\\?\\";
	WCHAR* szFileName = nullptr;
	szFileName = new WCHAR[MAX_FILENAME_LENGTH];
	ZeroMemory(szFileName, MAX_FILENAME_LENGTH);

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrFilter = L"All Image Files\0"              L"*.bmp;*.dib;*.wdp;*.mdp;*.hdp;*.gif;*.png;*.jpg;*.jpeg;*.tif;*.ico\0"
		L"Windows Bitmap\0"               L"*.bmp;*.dib\0"
		L"High Definition Photo\0"        L"*.wdp;*.mdp;*.hdp\0"
		L"Graphics Interchange Format\0"  L"*.gif\0"
		L"Portable Network Graphics\0"    L"*.png\0"
		L"JPEG File Interchange Format\0" L"*.jpg;*.jpeg\0"
		L"Tiff File\0"                    L"*.tif\0"
		L"Icon\0"                         L"*.ico\0"
		L"All Files\0"                    L"*.*\0"
		L"\0";
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = MAX_FILENAME_LENGTH;
	ofn.lpstrTitle = L"Open Image";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

	// Display the Open dialog box.
	BOOL ret = GetOpenFileName(&ofn);

	if (ret == TRUE)
	{
		m_szFileName += szFileName;
	}
	delete[] szFileName;

	return ret == TRUE ? true : false;
}

bool ImageViewer::DragProc(WPARAM wParam)
{
	HDROP hDrop = (HDROP)wParam;
	m_szFileName = L"\\\\?\\";
	bool ret = false;

	UINT nCount = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);

	// 	for (UINT i = 0; i < nCount; i++)
	// 	{
	// 		DragQueryFileW(hDrop, i, szFileName, MAX_FILENAME_LENGTH);
	// 	}
	UINT nSize = DragQueryFileW(hDrop, 0, nullptr, 0);
	if (nSize > 0)
	{
		WCHAR* szFileName = nullptr;
		szFileName = new WCHAR[nSize + 1];
		ZeroMemory(szFileName, nSize + 1);
		if (DragQueryFileW(hDrop, 0, szFileName, nSize + 1) > 0)
		{
			m_szFileName += szFileName;
			ret = true;
		}
		delete[] szFileName;
	}

	DragFinish(hDrop);
	return ret;
}

HRESULT ImageViewer::CreateDeviceResources()
{
	HRESULT hr = S_OK;

	if (!m_pRT)
	{
		RECT rc;
		hr = GetClientRect(m_hWnd, &rc) ? S_OK : E_FAIL;

		if (SUCCEEDED(hr))
		{
			auto renderTargetProperties = D2D1::RenderTargetProperties();

			// Set the DPI to be the default system DPI to allow direct mapping
			// between image pixels and desktop pixels in different system DPI settings
			renderTargetProperties.dpiX = DEFAULT_DPI;
			renderTargetProperties.dpiY = DEFAULT_DPI;

			auto size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

			hr = m_pD2DFactory->CreateHwndRenderTarget(
				renderTargetProperties,
				D2D1::HwndRenderTargetProperties(m_hWnd, size),
				&m_pRT
			);
		}
	}

	return hr;
}

LRESULT ImageViewer::OnPaint()
{
	HRESULT hr = S_OK;
	PAINTSTRUCT ps;

	if (BeginPaint(m_hWnd, &ps))
	{
		// Create render target if not yet created
		hr = CreateDeviceResources();

		if (SUCCEEDED(hr) && !(m_pRT->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
		{
			m_pRT->BeginDraw();

			m_pRT->SetTransform(D2D1::Matrix3x2F::Identity());

			// Clear the background
			m_pRT->Clear(D2D1::ColorF(D2D1::ColorF::White));

			auto rtSize = m_pRT->GetSize();

			// Create a rectangle with size of current window
			auto rectangle = D2D1::RectF(0.0f, 0.0f, rtSize.width, rtSize.height);

			// D2DBitmap may have been released due to device loss. 
			// If so, re-create it from the source bitmap
			if (m_pConvertedSourceBitmap && !m_pD2DBitmap)
			{
				m_pRT->CreateBitmapFromWicBitmap(m_pConvertedSourceBitmap, nullptr, &m_pD2DBitmap);
			}

			// Draws an image and scales it to the current window size
			if (m_pD2DBitmap)
			{
				m_pRT->DrawBitmap(m_pD2DBitmap, rectangle);
			}

			hr = m_pRT->EndDraw();

			// In case of device loss, discard D2D render target and D2DBitmap
			// They will be re-created in the next rendering pass
			if (hr == D2DERR_RECREATE_TARGET)
			{
				SafeRelease(m_pD2DBitmap);
				SafeRelease(m_pRT);
				// Force a re-render
				hr = InvalidateRect(m_hWnd, nullptr, TRUE) ? S_OK : E_FAIL;
			}
		}

		EndPaint(m_hWnd, &ps);
	}

	return SUCCEEDED(hr) ? 0 : 1;
}