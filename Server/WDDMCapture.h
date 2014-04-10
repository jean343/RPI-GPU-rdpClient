#pragma once

#include "wddm.h"

#include "Capture.h"

class WDDMCapture : public Capture {
public:
	void init(RECT screen)
	{
		this->screen = screen;

		wddm.wf_dxgi_init();

	}
	int getNextFrame(RGBQUAD** pPixels)
	{
		int rc;
		rc = wddm.wf_dxgi_nextFrame(3000);
		if (rc != 0) {
			return rc;
		}

		int pitch;
		rc = wddm.wf_dxgi_getPixelData((byte**)pPixels, &pitch, &screen);
		if (rc != 0) {
			return rc;
		}

		return 0;
	}
	void doneNextFrame()
	{
		int rc = wddm.wf_dxgi_releasePixelData();
	}

private:
	RECT screen;
	WDDM wddm;
};