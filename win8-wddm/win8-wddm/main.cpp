
#pragma comment (lib, "d3d11.lib") 
#pragma comment (lib, "Dxgi.lib") 


#include <fstream>
#include "wddm.h"

using namespace std;

int main(int argc, const char* argv[]) {
	WDDM wddm;

	wddm.wf_dxgi_init();

	byte* data;
	int pitch;
	RECT rect;
	rect.left=0;
	rect.top=0;
	rect.bottom=600;
	rect.right=600;
	while(true) {
		wddm.wf_dxgi_getPixelData(&data, &pitch, &rect);
	}

	system("pause");

	wddm.wf_dxgi_cleanup();
}