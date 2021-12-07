#pragma once

#include <cstdint>

namespace gi
{
	// Canvas World Coordinate System:
	// +------------> X
	// |
	// |
	// |
	// |
	// V
	// Y
	// By default, every integer coordinate is mapped to a screen pixel(scale factor = 1).
	class ICanvas
	{
	public:
		// Set origin of following draw operation
		virtual void SetDrawOrigin(double x, double y) = 0;
		// Set rotation of drawing coordinate system, counter-clockwise, in rads
		virtual void SetDrawRotation(double r) = 0;
		// Set scale factor for two axes
		virtual void SetDrawScale(double x, double y) = 0;
		// Set point size, in pixel
		virtual void SetDrawPointSize(int size) = 0;
		// Set point color
		virtual void SetDrawPointColor(uint8_t r, uint8_t g, uint8_t b) = 0;
		// Set background color
		virtual void SetDrawBackgroundColor(uint8_t r, uint8_t g, uint8_t b) = 0;
		// Draw a point
		virtual void DrawPoint(double x, double y) = 0;
		// Clear Canvas
		virtual void Clear() = 0;
	};
}