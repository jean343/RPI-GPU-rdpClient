//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <fstream>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "fps.h"
#include "monitor.h"
#include "params.h"
#include "FFMPEG_encoding.hpp"
#include "config.h"

#ifdef OpenCV_FOUND
	#include <opencv2/opencv.hpp>
	#include <opencv2/cudacodec.hpp>
	#include <opencv2/highgui.hpp>
	using namespace cv;
	using namespace cuda;
	using namespace cudacodec;
#endif

using namespace std;
using namespace boost::asio;
using ip::tcp;

const int max_length = 1024;

typedef boost::shared_ptr<tcp::socket> socket_ptr;

#ifdef OpenCV_FOUND
class StreamEncoder : public EncoderCallBack
{
public:
	StreamEncoder(socket_ptr sock){
        int buf_size = 10 * 1024; // TODO, set the right buffer size. Needs to be small enough to have at least one buffer per frame.
        buf_.resize(buf_size);
		this->sock = sock;
	}

    ~StreamEncoder() {
	}

    //! callback function to signal the start of bitstream that is to be encoded
    //! callback must allocate host buffer for CUDA encoder and return pointer to it and it's size
    uchar* acquireBitStream(int* bufferSize) {
        *bufferSize = static_cast<int>(buf_.size());
        return &buf_[0];
	}

    //! callback function to signal that the encoded bitstream is ready to be written to file
    void releaseBitStream(unsigned char* data, int size) {
		write(*sock, buffer((char*)data, size));
	}

    //! callback function to signal that the encoding operation on the frame has started
    void onBeginFrame(int frameNumber, PicType picType) {
	}

    //! callback function signals that the encoding operation on the frame has finished
    void onEndFrame(int frameNumber, PicType picType) {
	}

private:
    vector<uchar> buf_;
	socket_ptr sock;
	FPS fps;
};
#endif

void sessionVideo(socket_ptr sock, RECT screen)
{
	HDC hdc = GetDC(NULL); // get the desktop device context
	HDC hDest = CreateCompatibleDC(hdc); // create a device context to use yourself

	// get the height and width of the screen
	int height = screen.bottom - screen.top;
	int width = screen.right - screen.left;

	int virtualScreenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	int virtualScreenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);

	// create a bitmap
	HBITMAP hbDesktop = CreateCompatibleBitmap( hdc, virtualScreenWidth, virtualScreenHeight);

	// use the previously created device context with the bitmap
	SelectObject(hDest, hbDesktop);

#ifdef OpenCV_FOUND
	Ptr<StreamEncoder> callback(new StreamEncoder(sock));
	Ptr<cudacodec::VideoWriter> writer = createVideoWriter(callback, Size(width, height), 30); // TODO, find the right FPS
#else
	FFMPEG_encoding ffmpeg;
	ffmpeg.load(width, height, sock);
#endif


	BITMAPINFO bmi = {0};
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	RGBQUAD *pPixels = new RGBQUAD[width * height]; 

	FPS fps;
	while(true){

		// copy from the desktop device context to the bitmap device context
		BitBlt( hDest, 0,0, width, height, hdc, screen.left, screen.top, SRCCOPY);

		GetDIBits(
			hDest,
			hbDesktop,
			0,
			height,
			pPixels,
			&bmi,
			DIB_RGB_COLORS
		);

#ifdef OpenCV_FOUND
		Mat image(Size(width, height), CV_8UC4, pPixels);
		GpuMat gpuImage(image);
		writer->write(gpuImage);
#else
		ffmpeg.write(width, height, pPixels);
#endif

		fps.newFrame();
	}
#ifdef OpenCV_FOUND
#else
	ffmpeg.close();
#endif
}

struct SendStruct {
    int type;
    int x;
    int y;
    int button;
    int keycode;
};
void sessionKeystroke(socket_ptr sock, RECT screen)
{
	char data[sizeof(SendStruct)];
	boost::system::error_code error;

	SendStruct* s;
	INPUT input = {0};
	while(true) {
		size_t length = sock->read_some(buffer(data), error);
		if (error == error::eof)
			return; // Connection closed cleanly by peer.
		else if (error)
			throw boost::system::system_error(error); // Some other error.

		s = (SendStruct*)data;

		::ZeroMemory(&input,sizeof(INPUT));
		switch(s->type){
			case 0: // MotionNotify
				SetCursorPos(s->x + screen.left, s->y + screen.top);
				break;

			case 1:
				switch (s->button) {
				case 1: // left button
					input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
					break;
				case 2: // middle button
					input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
					break;
				case 3: // third button
					input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
					break;
				case 4: // scroll up
					input.mi.dwFlags = MOUSEEVENTF_WHEEL;
					input.mi.mouseData = 100;
					break;
				case 5: // scroll down
					input.mi.dwFlags = MOUSEEVENTF_WHEEL;
					input.mi.mouseData = -100;
					break;
				}
				input.type      = INPUT_MOUSE;
				::SendInput(1,&input,sizeof(INPUT));
				break;
			case 2:
				switch (s->button) {
				case 1: // left button
					input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
					break;
				case 2: // middle button
					input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
					break;
				case 3: // third button
					input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
					break;
				}
				if (input.mi.dwFlags) {
					input.type      = INPUT_MOUSE;
					::SendInput(1,&input,sizeof(INPUT));
				}
				break;

			case 3:
				input.type      = INPUT_KEYBOARD;
				input.ki.wScan = s->keycode;
				input.ki.wVk=0;
				input.ki.dwFlags  = KEYEVENTF_UNICODE;
				::SendInput(1,&input,sizeof(INPUT));
				break;
			case 4:
				input.type      = INPUT_KEYBOARD;
				input.ki.wScan = s->keycode;
				input.ki.wVk=0;
				input.ki.dwFlags  = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
				::SendInput(1,&input,sizeof(INPUT));
				break;
		}
	}
}
void session(socket_ptr sock, RECT screenCoordinates)
{
	try
	{
		sock->set_option(tcp::no_delay(true));
		char data[max_length];

		boost::system::error_code error;
		size_t length = sock->read_some(buffer(data), error);
		if (error == error::eof)
			return; // Connection closed cleanly by peer.
		else if (error)
			throw boost::system::system_error(error); // Some other error.

		if (data[0] == 'a'){
			sessionVideo(sock, screenCoordinates);
		} else if (data[0] == 'b'){
			sessionKeystroke(sock, screenCoordinates);
		} else {
			cout << "Received a connection with a wrong identification buffer " << string(data, length) << endl;
		}
	}
	catch (exception& e)
	{
		cerr << "Exception in thread: " << e.what() << "\n";
	}
}

void server(io_service& io_service, short port, RECT screenCoordinates)
{
	tcp::acceptor a(io_service, tcp::endpoint(tcp::v4(), port));
	for (;;)
	{
		socket_ptr sock(new tcp::socket(io_service));
		a.accept(*sock);
		boost::thread t(boost::bind(session, sock, screenCoordinates));
	}
}

int main(int argc, const char* argv[])
{
    cout << "Version 0.9" << endl;
	Params params(argc, argv);
    if (params.port == -1)
    {
		cerr << "Usage: ./server [options] port <#>" << endl;
		cerr << "monitor <n>\n";
		cerr << "Sample: ./server monitor 1 port 8080" << endl;
		return 1;
    }

	Monitor monitor;
	RECT screenCoordinates;
	int monitorCount = GetSystemMetrics(SM_CMONITORS);
	if (monitorCount > 1 && params.monitor == -1) {
		cerr << "There are more than one monitor available, select which monitor to use with\n./server -monitor <n> <port>" << endl;
		return 1;
	} else {
		if (params.monitor < 0 || params.monitor >= monitor.monitors.size()) {
			cerr << "The chosen monitor " << params.monitor << " is invalid, select from the following:\n";
			for (int i=0;i<monitor.monitors.size();i++) {
				RECT r = monitor.monitors[i];
				cerr << "Monitor " << i << ":" << "["<<r.left<<" "<<r.top<<","<<r.bottom<<" "<<r.right<<"]" << endl;
			}
			return 1;
		}
		screenCoordinates = monitor.monitors[params.monitor];
	}
	
	//socket_ptr sock;
	//sessionVideo(sock, screenCoordinates); // TODO test

	try
	{
		io_service io_service;

		server(io_service, params.port, screenCoordinates);
	}
	catch (exception& e)
	{
		cerr << "Exception: " << e.what() << "\n";
	}
	return 0;
}