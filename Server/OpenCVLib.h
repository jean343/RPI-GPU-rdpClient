#pragma once

#include <opencv2/core.hpp>
#include <opencv2/cudacodec.hpp>

#ifdef _DEBUG
	#pragma comment(lib, "opencv_core300d.lib")
	#pragma comment(lib, "opencv_highgui300d.lib")
	#pragma comment(lib, "opencv_video300d.lib")
	#pragma comment(lib, "opencv_imgproc300d.lib")
	#pragma comment(lib, "opencv_features2d300d.lib")
	#pragma comment(lib, "opencv_nonfree300d.lib")
	#pragma comment(lib, "opencv_flann300d.lib")

	#pragma comment(lib, "opencv_cuda300d.lib")
	#pragma comment(lib, "opencv_cudacodec300d.lib")
#else
	#pragma comment(lib, "opencv_core300.lib")
	#pragma comment(lib, "opencv_highgui300.lib")
	#pragma comment(lib, "opencv_video300.lib")
	#pragma comment(lib, "opencv_imgproc300.lib")
	#pragma comment(lib, "opencv_features2d300.lib")
	#pragma comment(lib, "opencv_nonfree300.lib")
	#pragma comment(lib, "opencv_flann300.lib")

	#pragma comment(lib, "opencv_cuda300.lib")
	#pragma comment(lib, "opencv_cudafeatures2d300.lib")
	#pragma comment(lib, "opencv_cudacodec300.lib")

#endif