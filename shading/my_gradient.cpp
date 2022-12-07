#include "GShader.h"
#include "GMatrix.h"
#include "GMath.h"

#include <vector>


int float2Pixel(float value) {
	return floor(value * 255 + 0.5);
}

GPixel color2Pixel(const GColor& c) {
	int a = float2Pixel(c.a);
	int r = float2Pixel(c.r * c.a);
	int g = float2Pixel(c.g * c.a);
	int b = float2Pixel(c.b * c.a);
	return GPixel_PackARGB(a, r, g, b);
}

class my_gradient: public GShader {
public:
	my_gradient(GPoint p0, GPoint p1, const GColor colors[], int count) : fcount(count) {
		for (int i = 0; i < count; i++){
			colorsArr.push_back(colors[i]);
		}

		fCTM = GMatrix();

		deltaX = p1.x() - p0.x();
		float deltaY = p1.y() - p0.y();

		fLocalMatrix = GMatrix(deltaX, -deltaY, p0.x(), deltaY, deltaX, p0.y());
	}

	bool isOpaque() {
		for (int i = 0; i < fcount; i++) {
			if (colorsArr[i].a != 1.0) {
				return false;
			}
		}
		return true;
	}

	bool setContext(const GMatrix& ctm) override {
		return (ctm * fLocalMatrix).invert(&fCTM);
	}

	float clamp(GPoint point) {
		float px = point.x();
		if (point.x() < 0.0) {
			px = 0;
		}
		if (point.x() > 1.0) {
			px = 1;
		}
		return px;
	}

	void shadeRow(int x, int y, int count, GPixel row[]) {
		for (int i = 0; i < count; i++) {
			GPoint point = { x + 0.5f + i, y + 0.5f };
			GPoint lm = fCTM * point;
			GColor color;

			float px = clamp(lm);
			float x = px * (fcount - 1);
			int index = GFloorToInt(x);
			float w = x - index;
			if (w == 0) {
				assert(index <= fcount - 1);
				color = colorsArr.at(index);
			}else{
				color = colorsArr.at(index) + (colorsArr.at(index + 1) - colorsArr.at(index)) * w;
			}
			row[i] = color2Pixel(color);
		}
	}

private:
	int fcount;
	float deltaX;
	GMatrix fLocalMatrix;
	GMatrix fCTM;
	std::vector<GColor> colorsArr;
};


std::unique_ptr<GShader> GCreateLinearGradient(GPoint p0, GPoint p1, const GColor colors[], int count) {
	if (count < 1) {
		return nullptr;
	}
	return std::unique_ptr<GShader>(new my_gradient(p0, p1, colors, count));
}