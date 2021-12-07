
#include "Canvas.h"
#include "Utils.h"

#include <cstring>
#include <iostream>

#include<windowsx.h>

#pragma comment(lib, "d2d1.lib")

extern "C" IMAGE_DOS_HEADER __ImageBase;

LRESULT gi::Canvas::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	auto* thiz = reinterpret_cast<Canvas*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	switch (uMsg)
	{
	case WM_CREATE: {
		LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
		::SetLastError(0);
		if (!::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pcs->lpCreateParams)) && ::GetLastError())
		{
			return -1;
		}
		return 0;
	}
	case WM_GETMINMAXINFO: {
		LPMINMAXINFO lpMMI = reinterpret_cast<LPMINMAXINFO>(lParam);
		lpMMI->ptMinTrackSize.x = 300;
		lpMMI->ptMinTrackSize.y = 300;
		return 0;
	}
	case WM_SIZE: {
		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);
		thiz->OnResizeWindow(width, height);
		return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
	case WM_MOUSEMOVE:
		thiz->mouseX = GET_X_LPARAM(lParam);
		thiz->mouseY = GET_Y_LPARAM(lParam);
		InvalidateRect(hWnd, &thiz->mouseTipRect, TRUE);
		UpdateWindow(hWnd);
		return 0;
	case WM_CLOSE:
		::DestroyWindow(hWnd);
		thiz->hWnd = NULL;
		return 0;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		if (thiz)
			return thiz->OnPaintWindow(hWnd);
		return -1;
	default:
		return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}

}

LRESULT gi::Canvas::OnPaintWindow(HWND hWnd)
{
	HRESULT hr = S_OK;
	PAINTSTRUCT ps;
	auto mouseString = JoinAsWideString(mouseX, L", ", mouseY);
	HDC hDC = BeginPaint(hWnd, &ps);
	FillRect(hDC, &ps.rcPaint, brushBackground.get());
	mouseTipRect.bottom = mouseTipRect.top + DrawTextW(hDC, mouseString.c_str(), -1, &mouseTipRect, DT_CALCRECT | DT_SINGLELINE | DT_LEFT);
	DrawTextW(hDC, mouseString.c_str(), -1, &mouseTipRect, DT_SINGLELINE | DT_LEFT);
	for (auto& point : points) {
		if (point.pointSize > 0) {
			SelectObject(hDC, colors[point.colorIndex].brushFill.get());
			SelectObject(hDC, colors[point.colorIndex].penBorder.get());
			Ellipse(
				hDC,
				static_cast<int>(point.x - point.pointSize),
				static_cast<int>(point.y - point.pointSize),
				static_cast<int>(point.x + point.pointSize + 1),
				static_cast<int>(point.y + point.pointSize + 1));
		}
		else
		{
			SetPixel(
				hDC,
				static_cast<int>(point.x),
				static_cast<int>(point.y),
				colors[point.colorIndex].pointColor);
		}
	}
	EndPaint(hWnd, &ps);
	return 0;
}

LRESULT gi::Canvas::OnResizeWindow(UINT width, UINT height)
{
	return 0;
}

void gi::Canvas::RegenerateTransformMatrix()
{
	double scale[3][3] = {
		{scaleFactorX,0.0,0.0},
		{0.0,scaleFactorY,0.0},
		{0.0,0.0,1.0}
	};
	double rotate[3][3] = {
		{cos(rotateAngle),-sin(rotateAngle),0.0},
		{sin(rotateAngle),cos(rotateAngle),0.0},
		{0.0,0.0,1.0}
	};
	double pan[3][3] = {
		{1.0,0.0,0.0},
		{0.0,1.0,0.0},
		{origin.x,origin.y,1.0}
	};
	double tmp[3][3] = {
		{1.0,0.0,0.0},
		{0.0,1.0,0.0},
		{0.0,0.0,1.0}
	};
	MultiplyMatrixMatrix(tmp, scale);
	MultiplyMatrixMatrix(tmp, rotate);
	MultiplyMatrixMatrix(tmp, pan);
	memcpy(&transformMatrix, &tmp, sizeof(tmp));
}

void gi::Canvas::MultiplyMatrixMatrix(double(&a)[3][3], const double(&b)[3][3])
{
	double tmp[3][3];
	for (size_t i = 0; i < 3; ++i)
	{
		for (size_t j = 0; j < 3; ++j)
		{
			tmp[i][j] = 0;
			for (size_t k = 0; k < 3; ++k)
			{
				tmp[i][j] += a[i][k] * b[k][j];
			}
		}
	}
	memcpy(&a, &tmp, sizeof(tmp));
}

void gi::Canvas::MultiplyVectorMatrix(double(&a)[3], const double(&b)[3][3])
{
	double tmp[3];
	for (size_t i = 0; i < 3; ++i)
	{
		tmp[i] = 0;
		for (size_t j = 0; j < 3; ++j)
		{
			tmp[i] += a[j] * b[j][i];
		}
	}
	memcpy(&a, &tmp, sizeof(tmp));
}

gi::Canvas::Canvas()
{
	colors.emplace_back(ColorInfo::MakeColor(RGB(0, 0, 0)));
}

gi::Canvas::~Canvas()
{
	if (hWnd)
		CloseWindow();
}

bool gi::Canvas::InitializeWindow()
{
	WNDCLASSEXW windowClass;
	memset(&windowClass, 0, sizeof(windowClass));
	windowClass.cbSize = sizeof(windowClass);
	windowClass.hInstance = reinterpret_cast<HINSTANCE>(&__ImageBase);
	windowClass.lpfnWndProc = &WindowProc;
	windowClass.lpszClassName = WindowClassName;
	windowClass.hCursor = LoadCursorW(nullptr, IDC_CROSS);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.cbWndExtra = sizeof(LONG_PTR);
	if (!::RegisterClassExW(&windowClass))
		return false;

	hWnd = ::CreateWindowExW(
		0,
		WindowClassName,
		L"Graph",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		reinterpret_cast<HINSTANCE>(&__ImageBase),
		this);
	if (hWnd == NULL)
		return false;

	return true;
}

void gi::Canvas::ShowWindow()
{
	::ShowWindow(hWnd, SW_NORMAL);
	MSG msg = { };
	while (::GetMessageW(&msg, NULL, 0, 0) > 0)
	{
		::TranslateMessage(&msg);
		::DispatchMessageW(&msg);
	}
}

void gi::Canvas::CloseWindow()
{
	::SendMessageW(hWnd, WM_CLOSE, 0, 0);
}

void gi::Canvas::SetDrawOrigin(double x, double y)
{
	origin.x = x;
	origin.y = y;
	RegenerateTransformMatrix();
}

void gi::Canvas::SetDrawRotation(double r)
{
	rotateAngle = r;
	RegenerateTransformMatrix();
}

void gi::Canvas::SetDrawScale(double x, double y)
{
	scaleFactorX = x;
	scaleFactorY = y;
	RegenerateTransformMatrix();
}

void gi::Canvas::SetDrawPointSize(int size)
{
	if (size >= 0)
		pointSize = size;
}

void gi::Canvas::SetDrawPointColor(uint8_t r, uint8_t g, uint8_t b)
{
	colors.push_back(ColorInfo::MakeColor(RGB(r, g, b)));
}

void gi::Canvas::DrawPoint(double x, double y)
{
	double coord[3] = { x,y,1.0 };
	MultiplyVectorMatrix(coord, transformMatrix);
	coord[0] /= coord[2];
	coord[1] /= coord[2];
	points.push_back({ coord[0], coord[1], pointSize, colors.size() - 1 });
}

void gi::Canvas::SetDrawBackgroundColor(uint8_t r, uint8_t g, uint8_t b)
{
	brushBackground.reset(::CreateSolidBrush(RGB(r, g, b)));
}

void gi::Canvas::Clear()
{
	points.clear();
}
