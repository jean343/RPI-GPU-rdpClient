////////////////////////////////////////////////////////////////////////////
//
// Copyright 1993-2014 NVIDIA Corporation.  All rights reserved.
//
// Please refer to the NVIDIA end user license agreement (EULA) associated
// with this source code for terms and conditions that govern your use of
// this software. Any use, reproduction, disclosure, or distribution of
// this software and related documentation outside the terms of the EULA
// is strictly prohibited.
//
////////////////////////////////////////////////////////////////////////////

#if defined(NV_WINDOWS)
    #include <d3d9.h>
    #include <d3d10_1.h>
    #include <d3d11.h>
#pragma warning(disable : 4996)
#endif

//#pragma comment (lib, "cuda.lib")
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3d10.lib")
#pragma comment (lib, "d3d11.lib")

#include "NvHWEncoder.h"
#include "nvEncodeAPI.h"
#include "nvUtils.h"

#define MAX_ENCODE_QUEUE 32
#define BITSTREAM_BUFFER_SIZE 2 * 1024 * 1024

#define SET_VER(configStruct, type) {configStruct.version = type##_VER;}

template<class T>
class CNvQueue {
    T** m_pBuffer;
    unsigned int m_uSize;
    unsigned int m_uPendingCount;
    unsigned int m_uAvailableIdx;
    unsigned int m_uPendingndex;
public:
    CNvQueue(): m_pBuffer(NULL), m_uSize(0), m_uPendingCount(0), m_uAvailableIdx(0),
                m_uPendingndex(0)
    {
    }

    ~CNvQueue()
    {
        delete[] m_pBuffer;
    }

    bool Initialize(T *pItems, unsigned int uSize)
    {
        m_uSize = uSize;
        m_uPendingCount = 0;
        m_uAvailableIdx = 0;
        m_uPendingndex = 0;
        m_pBuffer = new T *[m_uSize];
        for (unsigned int i = 0; i < m_uSize; i++)
        {
            m_pBuffer[i] = &pItems[i];
        }
        return true;
    }


    T * GetAvailable()
    {
        T *pItem = NULL;
        if (m_uPendingCount == m_uSize)
        {
            return NULL;
        }
        pItem = m_pBuffer[m_uAvailableIdx];
        m_uAvailableIdx = (m_uAvailableIdx+1)%m_uSize;
        m_uPendingCount += 1;
        return pItem;
    }

    T* GetPending()
    {
        if (m_uPendingCount == 0) 
        {
            return NULL;
        }

        T *pItem = m_pBuffer[m_uPendingndex];
        m_uPendingndex = (m_uPendingndex+1)%m_uSize;
        m_uPendingCount -= 1;
        return pItem;
    }
};

typedef struct _EncodeFrameConfig
{
    uint8_t  *yuv[3];
    uint32_t stride[3];
    uint32_t width;
    uint32_t height;
}EncodeFrameConfig;

typedef enum 
{
    NV_ENC_DX9 = 0,
    NV_ENC_DX11 = 1,
    NV_ENC_CUDA = 2,
    NV_ENC_DX10 = 3,
} NvEncodeDeviceType;

class CNvEncoder
{
public:
	CNvEncoder()
	{
		m_pNvHWEncoder = new CNvHWEncoder;
		m_pDevice = NULL;
#if defined (NV_WINDOWS)
		m_pD3D = NULL;
#endif
		m_cuContext = NULL;

		m_uEncodeBufferCount = 0;
		memset(&m_stEncoderInput, 0, sizeof(m_stEncoderInput));
		memset(&m_stEOSOutputBfr, 0, sizeof(m_stEOSOutputBfr));

		memset(&m_stEncodeBuffer, 0, sizeof(m_stEncodeBuffer));
	}

	~CNvEncoder()
	{
		if (m_pNvHWEncoder)
		{
			delete m_pNvHWEncoder;
			m_pNvHWEncoder = NULL;
		}
	}

	NVENCSTATUS InitCuda(uint32_t deviceID = 0)
	{
		CUresult cuResult;
		CUdevice device;
		CUcontext cuContextCurr;
		int  deviceCount = 0;
		int  SMminor = 0, SMmajor = 0;

		cuResult = cuInit(0);
		if (cuResult != CUDA_SUCCESS)
		{
			PRINTERR("cuInit error:0x%x\n", cuResult);
			assert(0);
			return NV_ENC_ERR_NO_ENCODE_DEVICE;
		}

		cuResult = cuDeviceGetCount(&deviceCount);
		if (cuResult != CUDA_SUCCESS)
		{
			PRINTERR("cuDeviceGetCount error:0x%x\n", cuResult);
			assert(0);
			return NV_ENC_ERR_NO_ENCODE_DEVICE;
		}

		// If dev is negative value, we clamp to 0
		if ((int)deviceID < 0)
			deviceID = 0;

		if (deviceID >(unsigned int)deviceCount - 1)
		{
			PRINTERR("Invalid Device Id = %d\n", deviceID);
			return NV_ENC_ERR_INVALID_ENCODERDEVICE;
		}

		cuResult = cuDeviceGet(&device, deviceID);
		if (cuResult != CUDA_SUCCESS)
		{
			PRINTERR("cuDeviceGet error:0x%x\n", cuResult);
			return NV_ENC_ERR_NO_ENCODE_DEVICE;
		}

		cuResult = cuDeviceComputeCapability(&SMmajor, &SMminor, deviceID);
		if (cuResult != CUDA_SUCCESS)
		{
			PRINTERR("cuDeviceComputeCapability error:0x%x\n", cuResult);
			return NV_ENC_ERR_NO_ENCODE_DEVICE;
		}

		if (((SMmajor << 4) + SMminor) < 0x30)
		{
			PRINTERR("GPU %d does not have NVENC capabilities exiting\n", deviceID);
			return NV_ENC_ERR_NO_ENCODE_DEVICE;
		}

		cuResult = cuCtxCreate((CUcontext*)(&m_pDevice), 0, device);
		if (cuResult != CUDA_SUCCESS)
		{
			PRINTERR("cuCtxCreate error:0x%x\n", cuResult);
			assert(0);
			return NV_ENC_ERR_NO_ENCODE_DEVICE;
		}

		cuResult = cuCtxPopCurrent(&cuContextCurr);
		if (cuResult != CUDA_SUCCESS)
		{
			PRINTERR("cuCtxPopCurrent error:0x%x\n", cuResult);
			assert(0);
			return NV_ENC_ERR_NO_ENCODE_DEVICE;
		}
		return NV_ENC_SUCCESS;
	}
	NVENCSTATUS Initialize(NV_ENC_DEVICE_TYPE deviceType) {
		NVENCSTATUS nvStatus = m_pNvHWEncoder->Initialize(m_pDevice, deviceType);
		return nvStatus;
	}

	NVENCSTATUS CreateEncoder(int width, int height){
		EncodeConfig encodeConfig;

		memset(&encodeConfig, 0, sizeof(EncodeConfig));
		
		encodeConfig.width = width;
		encodeConfig.height = height;

		// B = Encoding bitrate
		int B = 1000 * 1024; // kbps
		int fps = 20;
		uint32_t maxFrameSize = B / fps; // bandwidth / frame rate
		
		encodeConfig.vbvSize = maxFrameSize;

		encodeConfig.endFrameIdx = INT_MAX;
		encodeConfig.bitrate = encodeConfig.vbvSize * fps;
		encodeConfig.vbvMaxBitrate = encodeConfig.vbvSize * fps;


		encodeConfig.rcMode = NV_ENC_PARAMS_RC_VBR;//NV_ENC_PARAMS_RC_CONSTQP;
		encodeConfig.gopLength = 200;//NVENC_INFINITE_GOPLENGTH;
		encodeConfig.deviceType = NV_ENC_CUDA;
		encodeConfig.codec = NV_ENC_H264;
		encodeConfig.fps = fps;
		encodeConfig.qp = 28;
		encodeConfig.presetGUID = NV_ENC_PRESET_LOW_LATENCY_HQ_GUID;//NV_ENC_PRESET_LOW_LATENCY_HQ_GUID;//NV_ENC_PRESET_DEFAULT_GUID;
		encodeConfig.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
		encodeConfig.isYuv444 = 0;

		encodeConfig.presetGUID = m_pNvHWEncoder->GetPresetGUID(encodeConfig.encoderPreset, encodeConfig.codec);

		printf("Encoding input           : \"%s\"\n", encodeConfig.inputFileName);
		printf("         output          : \"%s\"\n", encodeConfig.outputFileName);
		printf("         codec           : \"%s\"\n", encodeConfig.codec == NV_ENC_HEVC ? "HEVC" : "H264");
		printf("         size            : %dx%d\n", encodeConfig.width, encodeConfig.height);
		printf("         bitrate         : %d bits/sec\n", encodeConfig.bitrate);
		printf("         vbvMaxBitrate   : %d bits/sec\n", encodeConfig.vbvMaxBitrate);
		printf("         vbvSize         : %d bits\n", encodeConfig.vbvSize);
		printf("         fps             : %d frames/sec\n", encodeConfig.fps);
		printf("         rcMode          : %s\n", encodeConfig.rcMode == NV_ENC_PARAMS_RC_CONSTQP ? "CONSTQP" :
												  encodeConfig.rcMode == NV_ENC_PARAMS_RC_VBR ? "VBR" :
												  encodeConfig.rcMode == NV_ENC_PARAMS_RC_CBR ? "CBR" :
												  encodeConfig.rcMode == NV_ENC_PARAMS_RC_VBR_MINQP ? "VBR MINQP" :
												  encodeConfig.rcMode == NV_ENC_PARAMS_RC_2_PASS_QUALITY ? "TWO_PASS_QUALITY" :
												  encodeConfig.rcMode == NV_ENC_PARAMS_RC_2_PASS_FRAMESIZE_CAP ? "TWO_PASS_FRAMESIZE_CAP" :
												  encodeConfig.rcMode == NV_ENC_PARAMS_RC_2_PASS_VBR ? "TWO_PASS_VBR" : "UNKNOWN");
		if (encodeConfig.gopLength == NVENC_INFINITE_GOPLENGTH)
			printf("         goplength       : INFINITE GOP \n");
		else
			printf("         goplength       : %d \n", encodeConfig.gopLength);
		printf("         B frames        : %d \n", encodeConfig.numB);
		printf("         QP              : %d \n", encodeConfig.qp);
		printf("       Input Format      : %s\n", encodeConfig.isYuv444 ? "YUV 444" : "YUV 420");
		printf("         preset          : %s\n", (encodeConfig.presetGUID == NV_ENC_PRESET_LOW_LATENCY_HQ_GUID) ? "LOW_LATENCY_HQ" :
											(encodeConfig.presetGUID == NV_ENC_PRESET_LOW_LATENCY_HP_GUID) ? "LOW_LATENCY_HP" :
											(encodeConfig.presetGUID == NV_ENC_PRESET_HQ_GUID) ? "HQ_PRESET" :
											(encodeConfig.presetGUID == NV_ENC_PRESET_HP_GUID) ? "HP_PRESET" :
											(encodeConfig.presetGUID == NV_ENC_PRESET_LOSSLESS_HP_GUID) ? "LOSSLESS_HP" : "LOW_LATENCY_DEFAULT");
		printf("  Picture Structure      : %s\n", (encodeConfig.pictureStruct == NV_ENC_PIC_STRUCT_FRAME) ? "Frame Mode" :
												  (encodeConfig.pictureStruct == NV_ENC_PIC_STRUCT_FIELD_TOP_BOTTOM) ? "Top Field first" :
												  (encodeConfig.pictureStruct == NV_ENC_PIC_STRUCT_FIELD_BOTTOM_TOP) ? "Bottom Field first" : "INVALID");
		printf("         devicetype      : %s\n",   encodeConfig.deviceType == NV_ENC_DX9 ? "DX9" :
													encodeConfig.deviceType == NV_ENC_DX10 ? "DX10" :
													encodeConfig.deviceType == NV_ENC_DX11 ? "DX11" :
													encodeConfig.deviceType == NV_ENC_CUDA ? "CUDA" : "INVALID");

		printf("\n");

		NVENCSTATUS nvStatus = m_pNvHWEncoder->CreateEncoder(&encodeConfig);

	    m_uEncodeBufferCount = encodeConfig.numB + 1; // min buffers is numb + 1 + 3 pipelining

		m_uPicStruct = encodeConfig.pictureStruct;

		return nvStatus;
	}

    NVENCSTATUS EncodeFrame(EncodeFrameConfig *pEncodeFrame, DataPacket* dataPacket, bool bFlush=false, uint32_t width=0, uint32_t height=0) {
		NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
		uint32_t lockedPitch = 0;
		EncodeBuffer *pEncodeBuffer = NULL;

		if (bFlush)
		{
			FlushEncoder(dataPacket);
			return NV_ENC_SUCCESS;
		}

		if (!pEncodeFrame)
		{
			return NV_ENC_ERR_INVALID_PARAM;
		}

		pEncodeBuffer = m_EncodeBufferQueue.GetAvailable();
		if(!pEncodeBuffer)
		{
			m_pNvHWEncoder->ProcessOutput(m_EncodeBufferQueue.GetPending(), dataPacket);
			pEncodeBuffer = m_EncodeBufferQueue.GetAvailable();
		}

		unsigned char *pInputSurface;
    
		nvStatus = m_pNvHWEncoder->NvEncLockInputBuffer(pEncodeBuffer->stInputBfr.hInputSurface, (void**)&pInputSurface, &lockedPitch);
		if (nvStatus != NV_ENC_SUCCESS)
			return nvStatus;

		if (pEncodeBuffer->stInputBfr.bufferFmt == NV_ENC_BUFFER_FORMAT_NV12_PL)
		{
			unsigned char *pInputSurfaceCh = pInputSurface + (pEncodeBuffer->stInputBfr.dwHeight*lockedPitch);
			convertYUVpitchtoNV12(pEncodeFrame->yuv[0], pEncodeFrame->yuv[1], pEncodeFrame->yuv[2], pInputSurface, pInputSurfaceCh, width, height, width, lockedPitch);
		}
		else
		{
			unsigned char *pInputSurfaceCb = pInputSurface + (pEncodeBuffer->stInputBfr.dwHeight * lockedPitch);
			unsigned char *pInputSurfaceCr = pInputSurfaceCb + (pEncodeBuffer->stInputBfr.dwHeight * lockedPitch);
			convertYUVpitchtoYUV444(pEncodeFrame->yuv[0], pEncodeFrame->yuv[1], pEncodeFrame->yuv[2], pInputSurface, pInputSurfaceCb, pInputSurfaceCr, width, height, width, lockedPitch);
		}
		nvStatus = m_pNvHWEncoder->NvEncUnlockInputBuffer(pEncodeBuffer->stInputBfr.hInputSurface);
		if (nvStatus != NV_ENC_SUCCESS)
			return nvStatus;

		nvStatus = m_pNvHWEncoder->NvEncEncodeFrame(pEncodeBuffer, NULL, width, height, (NV_ENC_PIC_STRUCT)m_uPicStruct);
		return nvStatus;
	}
	
    NVENCSTATUS AllocateIOBuffers(uint32_t uInputWidth, uint32_t uInputHeight, uint32_t isYuv444)
	{
		NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

		m_EncodeBufferQueue.Initialize(m_stEncodeBuffer, m_uEncodeBufferCount);
		for (uint32_t i = 0; i < m_uEncodeBufferCount; i++)
		{
			nvStatus = m_pNvHWEncoder->NvEncCreateInputBuffer(uInputWidth, uInputHeight, &m_stEncodeBuffer[i].stInputBfr.hInputSurface, isYuv444);
			if (nvStatus != NV_ENC_SUCCESS)
				return nvStatus;

			m_stEncodeBuffer[i].stInputBfr.bufferFmt = isYuv444 ? NV_ENC_BUFFER_FORMAT_YUV444_PL : NV_ENC_BUFFER_FORMAT_NV12_PL;
			m_stEncodeBuffer[i].stInputBfr.dwWidth = uInputWidth;
			m_stEncodeBuffer[i].stInputBfr.dwHeight = uInputHeight;

			nvStatus = m_pNvHWEncoder->NvEncCreateBitstreamBuffer(BITSTREAM_BUFFER_SIZE, &m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
			if (nvStatus != NV_ENC_SUCCESS)
				return nvStatus;
			m_stEncodeBuffer[i].stOutputBfr.dwBitstreamBufferSize = BITSTREAM_BUFFER_SIZE;

	#if defined (NV_WINDOWS)
			nvStatus = m_pNvHWEncoder->NvEncRegisterAsyncEvent(&m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
			if (nvStatus != NV_ENC_SUCCESS)
				return nvStatus;
			m_stEncodeBuffer[i].stOutputBfr.bWaitOnEvent = true;
	#else
			m_stEncodeBuffer[i].stOutputBfr.hOutputEvent = NULL;
	#endif
		}

		m_stEOSOutputBfr.bEOSFlag = TRUE;

	#if defined (NV_WINDOWS)
		nvStatus = m_pNvHWEncoder->NvEncRegisterAsyncEvent(&m_stEOSOutputBfr.hOutputEvent);
		if (nvStatus != NV_ENC_SUCCESS)
			return nvStatus; 
	#else
		m_stEOSOutputBfr.hOutputEvent = NULL;
	#endif

		return NV_ENC_SUCCESS;
	}

    NVENCSTATUS ReleaseIOBuffers()
	{
		for (uint32_t i = 0; i < m_uEncodeBufferCount; i++)
		{
			m_pNvHWEncoder->NvEncDestroyInputBuffer(m_stEncodeBuffer[i].stInputBfr.hInputSurface);
			m_stEncodeBuffer[i].stInputBfr.hInputSurface = NULL;

			m_pNvHWEncoder->NvEncDestroyBitstreamBuffer(m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
			m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer = NULL;

	#if defined(NV_WINDOWS)
			m_pNvHWEncoder->NvEncUnregisterAsyncEvent(m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
			nvCloseFile(m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
			m_stEncodeBuffer[i].stOutputBfr.hOutputEvent = NULL;
	#endif
		}

		if (m_stEOSOutputBfr.hOutputEvent)
		{
	#if defined(NV_WINDOWS)
			m_pNvHWEncoder->NvEncUnregisterAsyncEvent(m_stEOSOutputBfr.hOutputEvent);
			nvCloseFile(m_stEOSOutputBfr.hOutputEvent);
			m_stEOSOutputBfr.hOutputEvent = NULL;
	#endif
		}

		return NV_ENC_SUCCESS;
	}

protected:
    CNvHWEncoder                                        *m_pNvHWEncoder;
    uint32_t                                             m_uEncodeBufferCount;
    uint32_t                                             m_uPicStruct;
    void*                                                m_pDevice;
#if defined(NV_WINDOWS)
    IDirect3D9                                          *m_pD3D;
#endif

    CUcontext                                            m_cuContext;
    EncodeConfig                                         m_stEncoderInput;
    EncodeBuffer                                         m_stEncodeBuffer[MAX_ENCODE_QUEUE];
    CNvQueue<EncodeBuffer>                               m_EncodeBufferQueue;
    EncodeOutputBuffer                                   m_stEOSOutputBfr; 
	
	void convertYUVpitchtoNV12( unsigned char *yuv_luma, unsigned char *yuv_cb, unsigned char *yuv_cr,
								unsigned char *nv12_luma, unsigned char *nv12_chroma,
								int width, int height , int srcStride, int dstStride)
	{
		int y;
		int x;
		if (srcStride == 0)
			srcStride = width;
		if (dstStride == 0)
			dstStride = width;

		for ( y = 0 ; y < height ; y++)
		{
			memcpy( nv12_luma + (dstStride*y), yuv_luma + (srcStride*y) , width );
		}

		for ( y = 0 ; y < height/2 ; y++)
		{
			for ( x= 0 ; x < width; x=x+2)
			{
				nv12_chroma[(y*dstStride) + x] =    yuv_cb[((srcStride/2)*y) + (x >>1)];
				nv12_chroma[(y*dstStride) +(x+1)] = yuv_cr[((srcStride/2)*y) + (x >>1)];
			}
		}
	}

	void convertYUVpitchtoYUV444(unsigned char *yuv_luma, unsigned char *yuv_cb, unsigned char *yuv_cr,
		unsigned char *surf_luma, unsigned char *surf_cb, unsigned char *surf_cr, int width, int height, int srcStride, int dstStride)
	{
		int h;

		for (h = 0; h < height; h++)
		{
			memcpy(surf_luma + dstStride * h, yuv_luma + srcStride * h, width);
			memcpy(surf_cb + dstStride * h, yuv_cb + srcStride * h, width);
			memcpy(surf_cr + dstStride * h, yuv_cr + srcStride * h, width);
		}
	}
protected:
    NVENCSTATUS                                          Deinitialize(uint32_t devicetype);
    NVENCSTATUS                                          InitD3D9(uint32_t deviceID = 0);
    NVENCSTATUS                                          InitD3D11(uint32_t deviceID = 0);
    NVENCSTATUS                                          InitD3D10(uint32_t deviceID = 0);



    unsigned char*                                       LockInputBuffer(void * hInputSurface, uint32_t *pLockedPitch);

    NVENCSTATUS                                          FlushEncoder(DataPacket* dataPacket) {
		NVENCSTATUS nvStatus = m_pNvHWEncoder->NvEncFlushEncoderQueue(m_stEOSOutputBfr.hOutputEvent);
		if (nvStatus != NV_ENC_SUCCESS)
		{
			assert(0);
			return nvStatus;
		}

		EncodeBuffer *pEncodeBufer = m_EncodeBufferQueue.GetPending();
		while (pEncodeBufer)
		{
			m_pNvHWEncoder->ProcessOutput(pEncodeBufer, dataPacket);
			pEncodeBufer = m_EncodeBufferQueue.GetPending();
		}

#if defined(NV_WINDOWS)
		if (WaitForSingleObject(m_stEOSOutputBfr.hOutputEvent, 500) != WAIT_OBJECT_0)
		{
			assert(0);
			nvStatus = NV_ENC_ERR_GENERIC;
		}
#endif

		return nvStatus;
	}
};

// NVEncodeAPI entry point
typedef NVENCSTATUS (NVENCAPI *MYPROC)(NV_ENCODE_API_FUNCTION_LIST*); 
