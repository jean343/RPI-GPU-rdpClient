#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "color_conversion.h"

#include "stdio.h"

__host__ __device__ __forceinline__ int divUp(int total, int grain)
{
	return (total + grain - 1) / grain;
}

__global__ void RGB_to_jp(uchar4 *input, unsigned char *yuv_luma, unsigned char *yuv_cb, unsigned char *yuv_cr, int width, int height)
{
    const int x = blockIdx.x * blockDim.x + threadIdx.x;
    const int y = blockIdx.y * blockDim.y + threadIdx.y;
	
	if (x >= width || y>=height) return;

	uchar4 px = input[y * width + x];
	int Y = ( (  66 * px.x + 129 * px.y +  25 * px.z + 128) >> 8) +  16;
	int U = ( ( -38 * px.x -  74 * px.y + 112 * px.z + 128) >> 8) + 128;
	int V = ( ( 112 * px.x -  94 * px.y -  18 * px.z + 128) >> 8) + 128;
	
	yuv_luma[y * width + x] = Y;

	int pos = (y >> 1) * (width >> 1) + (x >> 1);
	yuv_cr[pos] = U;
	yuv_cb[pos] = V;
}

bool RGB_to_YV12(int width, int height, void *pPixels, void* yuv_luma, void* yuv_cb, void* yuv_cr)
{
	cudaError_t cudaStatus;

	const dim3 block(32, 8);
	const dim3 grid(divUp(width, block.x), divUp(height, block.y));

	unsigned char *yuv_luma_device;
	cudaMalloc(&yuv_luma_device, width *height * sizeof(unsigned char));

	unsigned char *yuv_cb_device;
	cudaMalloc(&yuv_cb_device, width *height * sizeof(unsigned char) / 4);

	unsigned char *yuv_cr_device;
	cudaMalloc(&yuv_cr_device, width *height * sizeof(unsigned char) / 4);
   

	// Copy input vectors from host memory to GPU buffers.
	uchar4 *dev_pPixels;
	cudaStatus = cudaMalloc((void**)&dev_pPixels, width *height * sizeof(uchar4));
	if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMalloc failed!");
        return false;
    }

	cudaStatus = cudaMemcpy(dev_pPixels, pPixels, width *height * sizeof(uchar4), cudaMemcpyHostToDevice);
	if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMemcpy 1 failed!");
        return false;
    }

	RGB_to_jp<<< grid, block >>>(dev_pPixels, yuv_luma_device, yuv_cb_device, yuv_cr_device, width, height);
   
	cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "RGB_to_jp failed!");
        return false;
    }
	
	cudaStatus = cudaDeviceSynchronize();
	if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaDeviceSynchronize failed!");
        return false;
    }

	cudaStatus = cudaMemcpy(yuv_luma, yuv_luma_device, width *height * sizeof(unsigned char), cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMemcpy 2 failed!");
        return false;
    }
	cudaStatus = cudaMemcpy(yuv_cb, yuv_cb_device, width *height * sizeof(unsigned char) / 4, cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMemcpy 2 failed!");
        return false;
    }
	cudaStatus = cudaMemcpy(yuv_cr, yuv_cr_device, width *height * sizeof(unsigned char) / 4, cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMemcpy 2 failed!");
        return false;
    }

	cudaFree(yuv_luma_device);
	cudaFree(yuv_cb_device);
	cudaFree(yuv_cr_device);
	cudaFree(dev_pPixels);

    return true;
}