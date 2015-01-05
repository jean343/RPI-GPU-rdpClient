using namespace boost::asio;
using ip::tcp;

typedef boost::shared_ptr<tcp::socket> socket_ptr;

#include "NvEncoder/NvEncoder.h"
#include "color_conversion.h"

class NV_encoding {
public:
	void load(int width, int height, socket_ptr sock) {
		NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

		this->sock = sock;
		this->width = width;
		this->height = height;
		cNvEncoder = new CNvEncoder();
		cNvEncoder->InitCuda();
		nvStatus = cNvEncoder->Initialize(NV_ENC_DEVICE_TYPE_CUDA);
		nvStatus = cNvEncoder->CreateEncoder(width, height);
		nvStatus = cNvEncoder->AllocateIOBuffers(width, height, false);

		dataPacket = new DataPacket();
		dataPacket->data = new uint8_t[width*height];

		yuv[0] = new uint8_t[width*height];
        yuv[1] = new uint8_t[width*height / 4];
        yuv[2] = new uint8_t[width*height / 4];
	}
	void write(int width, int height, RGBQUAD *pPixels) {
		
		bool rc = RGB_to_YV12(width, height, pPixels, yuv[0], yuv[1], yuv[2]);

		if (!rc){
			// The Cuda function RGB_to_YV12 failed, do CPU conversion
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
				
					RGBQUAD px = pPixels[y*width+x];
					int Y = ( (  66 * px.rgbRed + 129 * px.rgbGreen +  25 * px.rgbBlue + 128) >> 8) +  16;
					int U = ( ( -38 * px.rgbRed -  74 * px.rgbGreen + 112 * px.rgbBlue + 128) >> 8) + 128;
					int V = ( ( 112 * px.rgbRed -  94 * px.rgbGreen -  18 * px.rgbBlue + 128) >> 8) + 128;

					yuv[0][y * width + x] = Y;
					yuv[1][(y >> 1) * (width >> 1) + (x >> 1)] = U;
					yuv[2][(y >> 1) * (width >> 1) + (x >> 1)] = V;
				}
			}
		}

		EncodeFrameConfig stEncodeFrame;
        memset(&stEncodeFrame, 0, sizeof(stEncodeFrame));

        stEncodeFrame.yuv[0] = yuv[0];
        stEncodeFrame.yuv[1] = yuv[1];
        stEncodeFrame.yuv[2] = yuv[2];

        stEncodeFrame.stride[0] = width;
        stEncodeFrame.stride[1] = width/2;
        stEncodeFrame.stride[2] = width/2;
        stEncodeFrame.width = width;
        stEncodeFrame.height = height;

        cNvEncoder->EncodeFrame(&stEncodeFrame, dataPacket, false, width, height);
		if (dataPacket->size > 0) {
			printf("Write frame (size=%5d)\n", dataPacket->size);

			boost::asio::write(*sock, buffer((char*)dataPacket->data, dataPacket->size));
		}
	}
	void close () {
		delete cNvEncoder;
		delete dataPacket->data;
		delete dataPacket;
		for (int i = 0; i < 3; i ++)
		{
			if (yuv[i])
			{
				delete [] yuv[i];
			}
		}
	}
private:
	int width;
	int height;
	socket_ptr sock;
    uint8_t *yuv[3];
	CNvEncoder* cNvEncoder;
	DataPacket* dataPacket;
};