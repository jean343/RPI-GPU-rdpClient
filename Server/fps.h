#pragma once
#include <time.h>

class FPS {
public:
	FPS() {
		fps = 0;
		numFrame = 0;
		lastSec = 0;
		lastShouldRefresh = 0;
	}
	void newFrame() {
		numFrame++;
		double newTime = (double)clock() / CLOCKS_PER_SEC;

		if (newTime >= lastSec + 1) {
			fps = numFrame;
			numFrame = 0;
			lastSec = newTime;
			printf("FPS: %d\n", getFps());
		}
	}
	int getFps() {
		return fps;
	}

	/* Returns true only 30 times per second */
	bool shouldRefresh() {
		double newTime = (double)clock() / CLOCKS_PER_SEC;
		if (newTime >= lastShouldRefresh + 1.0/30) {
			lastShouldRefresh = newTime;
			return true;
		} else {
			return false;
		}
	}
private:
	int fps;
	int numFrame;
	double lastSec;

	double lastShouldRefresh;
};