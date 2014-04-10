#pragma once

class Capture {
public:
	virtual void init(RECT screen) = 0;
	virtual int getNextFrame(RGBQUAD**) = 0;
	virtual void doneNextFrame() = 0;
};