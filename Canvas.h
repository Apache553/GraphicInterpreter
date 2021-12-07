#pragma once

#include "ICanvas.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d2d1.h>
#include <d3d11.h>

#include <wil/resource.h>
#include <wil/com.h>

#include <vector>

namespace gi
{
	class Canvas : public ICanvas
	{
	private:
		HWND hWnd = NULL;

		UINT mouseX = 0;
		UINT mouseY = 0;
		RECT mouseTipRect = { 10,10,10,20 };
	private:
		static constexpr auto WindowClassName = L"GIGraphWindow";

		static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		LRESULT OnPaintWindow(HWND hWnd);
		LRESULT OnResizeWindow(UINT width, UINT height);
	private:
		struct PointInfo
		{
			double x;
			double y;
			int pointSize;
			size_t colorIndex;
		};

		struct ColorInfo
		{
			COLORREF pointColor;
			wil::unique_hbrush brushFill;
			wil::unique_hpen penBorder;
			static ColorInfo MakeColor(COLORREF color) {
				return { color,
					wil::unique_hbrush(::CreateSolidBrush(color)),
					wil::unique_hpen(::CreatePen(PS_SOLID, 0, color))
				};
			}
		};
	private:
		wil::unique_hbrush brushBackground{ reinterpret_cast<HBRUSH>(COLOR_WINDOWTEXT + 1) };
		std::vector<ColorInfo> colors;
		int pointSize = 4;

		std::vector<PointInfo> points;

		PointInfo origin{ 0,0 };
		double scaleFactorX = 1.0;
		double scaleFactorY = 1.0;
		double rotateAngle = 0.0;

		double transformMatrix[3][3] = {
			{1.0,0.0,0.0},
			{0.0,1.0,0.0},
			{0.0,0.0,1.0} };
		void RegenerateTransformMatrix();

		static void MultiplyMatrixMatrix(double(&a)[3][3], const double(&b)[3][3]);
		static void MultiplyVectorMatrix(double(&a)[3], const double(&b)[3][3]);
	public:
		Canvas();
		Canvas(const Canvas&) = delete;
		Canvas(Canvas&&) = delete;

		Canvas& operator=(const Canvas&) = delete;
		Canvas& operator=(Canvas&&) = delete;

		~Canvas();

		bool InitializeWindow();
		void ShowWindow();

		void CloseWindow();

		void SetDrawOrigin(double x, double y) override;
		void SetDrawRotation(double r) override;
		void SetDrawScale(double x, double y) override;
		void SetDrawPointSize(int size) override;
		void SetDrawPointColor(uint8_t r, uint8_t g, uint8_t b) override;
		void SetDrawBackgroundColor(uint8_t r, uint8_t g, uint8_t b) override;
		void DrawPoint(double x, double y) override;
		void Clear() override;
	};


}
