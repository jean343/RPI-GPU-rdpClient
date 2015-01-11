#pragma once

class Capture {
public:
	virtual void init(UINT monitorID, RECT screen) = 0;
	virtual int getNextFrame(RGBQUAD**) = 0;
	virtual void doneNextFrame() = 0;
};