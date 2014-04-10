#pragma once

#include "Capture.h"

class GDICapture : public Capture {
public:
	void init(RECT screen)
	{
		this->screen = screen;
		hdc = GetDC(NULL); // get the desktop device context
		hDest = CreateCompatibleDC(hdc); // create a device context to use yourself

		// get the height and width of the screen
		height = screen.bottom - screen.top;
		width = screen.right - screen.left;

		int virtualScreenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
		int virtualScreenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);

		// create a bitmap
		hbDesktop = CreateCompatibleBitmap( hdc, virtualScreenWidth, virtualScreenHeight);

		// use the previously created device context with the bitmap
		SelectObject(hDest, hbDesktop);
		
		bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
		bmi.bmiHeader.biWidth = width;
		bmi.bmiHeader.biHeight = -height;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;

		pPixels = new RGBQUAD[width * height];

	}
	int getNextFrame(RGBQUAD** pPixels)
	{
		// copy from the desktop device context to the bitmap device context
		BitBlt( hDest, 0,0, width, height, hdc, screen.left, screen.top, SRCCOPY);

		GetDIBits(
			hDest,
			hbDesktop,
			0,
			height,
			*pPixels,
			&bmi,
			DIB_RGB_COLORS
		);

		return 0;
	}
	void doneNextFrame()
	{
	}
private:
	HDC hdc, hDest;
	int width, height;
	RECT screen;
	RGBQUAD *pPixels;
	HBITMAP hbDesktop;
	BITMAPINFO bmi;
};