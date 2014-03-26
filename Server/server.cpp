//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <fstream>
#include <iostream>
#include "OpenCVLib.h"
#include "fps.h"

using namespace cv;
using namespace cuda;
using namespace cudacodec;
using namespace std;
using namespace boost::asio;
using ip::tcp;

const int max_length = 1024;

typedef boost::shared_ptr<tcp::socket> socket_ptr;

class JPEncoderCallBack : public EncoderCallBack
{
public:
	JPEncoderCallBack(socket_ptr sock, Size frameSize, double fps){
        int buf_size = 10 * 1024;
        buf_.resize(buf_size);
		this->sock = sock;
	}

    ~JPEncoderCallBack() {
	}

    //! callback function to signal the start of bitstream that is to be encoded
    //! callback must allocate host buffer for CUDA encoder and return pointer to it and it's size
    uchar* acquireBitStream(int* bufferSize) {
        *bufferSize = static_cast<int>(buf_.size());
        return &buf_[0];
	}

    //! callback function to signal that the encoded bitstream is ready to be written to file
    void releaseBitStream(unsigned char* data, int size) {
		//printf("start Send packet: %d\n", size);
		write(*sock, buffer((char*)data, size));
		//printf("done Send packet: %d\n", size);
        //fs.write((char*)data, size);
        //fs.close();
		//fps.newFrame();
	}

    //! callback function to signal that the encoding operation on the frame has started
    void onBeginFrame(int frameNumber, PicType picType) {
	}

    //! callback function signals that the encoding operation on the frame has finished
    void onEndFrame(int frameNumber, PicType picType) {
	}

private:
    std::vector<uchar> buf_;
	std::ofstream fs;
	socket_ptr sock;
	FPS fps;
};

void sessionVideo(socket_ptr sock)
{
	HDC hdc = GetDC(NULL); // get the desktop device context
	HDC hDest = CreateCompatibleDC(hdc); // create a device context to use yourself

	// get the height and width of the screen
	int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);

	// create a bitmap
	HBITMAP hbDesktop = CreateCompatibleBitmap( hdc, width, height);

	// use the previously created device context with the bitmap
	SelectObject(hDest, hbDesktop);

	Ptr<JPEncoderCallBack> callback(new JPEncoderCallBack(sock, Size(width, height), 10));

	//string video = "C:\\Users\\Gateway\\Desktop\\CBSA\\RemoteDesktop\\vid.avi";
	Ptr<cudacodec::VideoWriter> writer = createVideoWriter(callback, Size(width, height), 10);



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
		BitBlt( hDest, 0,0, width, height, hdc, 0, 0, SRCCOPY);

		GetDIBits(
			hDest, 
			hbDesktop, 
			0,  
			height,  
			pPixels, 
			&bmi,  
			DIB_RGB_COLORS
		);


		Mat image(Size(width, height), CV_8UC4, pPixels);
		GpuMat gpuImage(image);
		writer->write(gpuImage);

		fps.newFrame();
	}
}

struct SendStruct {
    int type;
    int x;
    int y;
    int button;
    int keycode;
};
void sessionKeystroke(socket_ptr sock)
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
				SetCursorPos(s->x, s->y);
				break;

			case 1:
				input.type      = INPUT_MOUSE;
				input.mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;
				::SendInput(1,&input,sizeof(INPUT));
				break;
			case 2:
				input.type      = INPUT_MOUSE;
				input.mi.dwFlags  = MOUSEEVENTF_LEFTUP;
				::SendInput(1,&input,sizeof(INPUT));
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
void session(socket_ptr sock)
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
			sessionVideo(sock);
		} else if (data[0] == 'b'){
			sessionKeystroke(sock);
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception in thread: " << e.what() << "\n";
	}
}

void server(io_service& io_service, short port)
{
	tcp::acceptor a(io_service, tcp::endpoint(tcp::v4(), port));
	for (;;)
	{
		socket_ptr sock(new tcp::socket(io_service));
		a.accept(*sock);
		boost::thread t(boost::bind(session, sock));
	}
}

int main(int argc, char* argv[])
{
	try
	{
		io_service io_service;

		server(io_service, 8080);
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
	return 0;
}