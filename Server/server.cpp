//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <fstream>
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <bounded_buffer.h>

#include "fps.h"
#include "monitor.h"
#include "params.h"
#include "config.h"

#ifdef HAS_WDDM
	#include "WDDMCapture.h"
#else
	#include "GDICapture.h"
#endif

#ifdef HAS_FFMPEG
	#include "FFMPEG_encoding.hpp"
#endif

#ifdef HAS_NVENC
	#include "NV_encoding.hpp"
#endif

using namespace std;
using namespace boost::asio;
using ip::tcp;

const int max_length = 1024;

typedef boost::shared_ptr<tcp::socket> socket_ptr;

bounded_buffer<RGBQUAD*> screenToSendQueue(2);

void threadScreenCapture(UINT monitorID, RECT screen){
	int height = screen.bottom - screen.top;
	int width = screen.right - screen.left;

#ifdef HAS_WDDM
	WDDMCapture capture;
#else
	GDICapture capture;
#endif

	capture.init(monitorID, screen);

	RGBQUAD* pPixels;
	FPS fps;
	while(true){
		int rc = capture.getNextFrame(&pPixels);
		if (rc == 0) {
			RGBQUAD* pixCopy = new RGBQUAD[width * height];
			memcpy(pixCopy, pPixels, width * height * sizeof(RGBQUAD));
			screenToSendQueue.push_front(pixCopy);

			capture.doneNextFrame();
			fps.newFrame();
		}
	}
}

void sessionVideo(socket_ptr sock, UINT monitorID, RECT screen)
{
	
	// get the height and width of the screen
	int height = screen.bottom - screen.top;
	int width = screen.right - screen.left;

#ifdef HAS_NVENC
	NV_encoding nv_encoding;
	nv_encoding.load(width, height, sock, monitorID);
#elif defined(HAS_FFMPEG)
	FFMPEG_encoding ffmpeg;
	ffmpeg.load(width, height, sock);
#endif

	boost::thread t(boost::bind(threadScreenCapture, monitorID, screen));

	FPS fps;
	RGBQUAD* pPixels;
	while(true){
		screenToSendQueue.pop_back(&pPixels);

#ifdef HAS_NVENC
		nv_encoding.write(width, height, pPixels);
#elif defined(HAS_FFMPEG)
		ffmpeg.write(width, height, pPixels);
#endif
		//fps.newFrame();

		free(pPixels);
	}
#ifdef HAS_NVENC
	nv_encoding.close();
#elif defined(HAS_FFMPEG)
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
void session(socket_ptr sock, UINT monitorID, RECT screenCoordinates)
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
			sessionVideo(sock, monitorID, screenCoordinates);
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

void server(io_service& io_service, short port, UINT monitorID, RECT screenCoordinates)
{
	tcp::acceptor a(io_service, tcp::endpoint(tcp::v4(), port));
	for (;;)
	{
		socket_ptr sock(new tcp::socket(io_service));
		a.accept(*sock);
		boost::thread t(boost::bind(session, sock, monitorID, screenCoordinates));
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
	//sessionVideo(sock, params.monitor, screenCoordinates); // TODO test

	try
	{
		io_service io_service;

		server(io_service, params.port, params.monitor, screenCoordinates);
	}
	catch (exception& e)
	{
		cerr << "Exception: " << e.what() << "\n";
	}
	return 0;
}