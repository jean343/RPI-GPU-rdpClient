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

#pragma comment (lib, "cuda.lib")
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3d10.lib")
#pragma comment (lib, "d3d11.lib")

#include "nvEncodeAPI.h"
#include "nvUtils.h"
#include "NvEncoder.h"
#include "nvFileIO.h"

#define BITSTREAM_BUFFER_SIZE 2 * 1024 * 1024

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

#if defined(NV_WINDOWS)
NVENCSTATUS CNvEncoder::InitD3D9(uint32_t deviceID)
{
    D3DPRESENT_PARAMETERS d3dpp;
    D3DADAPTER_IDENTIFIER9 adapterId;
    unsigned int iAdapter = NULL; // Our adapter
    HRESULT hr = S_OK;

    m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (m_pD3D == NULL)
    {
        assert(m_pD3D);
        return NV_ENC_ERR_OUT_OF_MEMORY;;
    }

    if (deviceID >= m_pD3D->GetAdapterCount())
    {
        PRINTERR("Invalid Device Id = %d\n", deviceID);
        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
    }

    hr = m_pD3D->GetAdapterIdentifier(deviceID, 0, &adapterId);
    if (hr != S_OK)
    {
        PRINTERR("Invalid Device Id = %d\n", deviceID);
        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
    d3dpp.BackBufferWidth = 640;
    d3dpp.BackBufferHeight = 480;
    d3dpp.BackBufferCount = 1;
    d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    d3dpp.Flags = D3DPRESENTFLAG_VIDEO;//D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    DWORD dwBehaviorFlags = D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED | D3DCREATE_HARDWARE_VERTEXPROCESSING;

    hr = m_pD3D->CreateDevice(deviceID,
        D3DDEVTYPE_HAL,
        GetDesktopWindow(),
        dwBehaviorFlags,
        &d3dpp,
        (IDirect3DDevice9**)(&m_pDevice));

    if (FAILED(hr))
        return NV_ENC_ERR_OUT_OF_MEMORY;

    return  NV_ENC_SUCCESS;
}

NVENCSTATUS CNvEncoder::InitD3D10(uint32_t deviceID)
{
    HRESULT hr;
    IDXGIFactory * pFactory = NULL;
    IDXGIAdapter * pAdapter;

    if (CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory) != S_OK)
    {
        return NV_ENC_ERR_GENERIC;
    }

    if (pFactory->EnumAdapters(deviceID, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        hr = D3D10CreateDevice(pAdapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
            D3D10_SDK_VERSION, (ID3D10Device**)(&m_pDevice));
        if (FAILED(hr))
        {
            PRINTERR("Problem while creating %d D3d10 device \n", deviceID);
            return NV_ENC_ERR_OUT_OF_MEMORY;
        }
    }
    else
    {
        PRINTERR("Invalid Device Id = %d\n", deviceID);
        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
    }

    return  NV_ENC_SUCCESS;
}

NVENCSTATUS CNvEncoder::InitD3D11(uint32_t deviceID)
{
    HRESULT hr;
    IDXGIFactory * pFactory = NULL;
    IDXGIAdapter * pAdapter;

    if (CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory) != S_OK)
    {
        return NV_ENC_ERR_GENERIC;
    }

    if (pFactory->EnumAdapters(deviceID, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        hr = D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0,
            NULL, 0, D3D11_SDK_VERSION, (ID3D11Device**)(&m_pDevice), NULL, NULL);
        if (FAILED(hr))
        {
            PRINTERR("Problem while creating %d D3d11 device \n", deviceID);
            return NV_ENC_ERR_OUT_OF_MEMORY;
        }
    }
    else
    {
        PRINTERR("Invalid Device Id = %d\n", deviceID);
        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
    }

    return  NV_ENC_SUCCESS;
}
#endif

NVENCSTATUS CNvEncoder::AllocateIOBuffers(uint32_t uInputWidth, uint32_t uInputHeight, uint32_t isYuv444)
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

NVENCSTATUS CNvEncoder::ReleaseIOBuffers()
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

NVENCSTATUS CNvEncoder::FlushEncoder()
{
    NVENCSTATUS nvStatus = m_pNvHWEncoder->NvEncFlushEncoderQueue(m_stEOSOutputBfr.hOutputEvent);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
        return nvStatus;
    }

    EncodeBuffer *pEncodeBufer = m_EncodeBufferQueue.GetPending();
    while (pEncodeBufer)
    {
        m_pNvHWEncoder->ProcessOutput(pEncodeBufer);
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

NVENCSTATUS CNvEncoder::Deinitialize(uint32_t devicetype)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    ReleaseIOBuffers();

    nvStatus = m_pNvHWEncoder->NvEncDestroyEncoder();

    if (m_pDevice)
    {
        switch (devicetype)
        {
#if defined(NV_WINDOWS)
        case NV_ENC_DX9:
            ((IDirect3DDevice9*)(m_pDevice))->Release();
            break;

        case NV_ENC_DX10:
            ((ID3D10Device*)(m_pDevice))->Release();
            break;

        case NV_ENC_DX11:
            ((ID3D11Device*)(m_pDevice))->Release();
            break;
#endif

        case NV_ENC_CUDA:
            CUresult cuResult = CUDA_SUCCESS;
            cuResult = cuCtxDestroy((CUcontext)m_pDevice);
            if (cuResult != CUDA_SUCCESS)
                PRINTERR("cuCtxDestroy error:0x%x\n", cuResult);
        }

        m_pDevice = NULL;
    }

#if defined (NV_WINDOWS)
    if (m_pD3D)
    {
        m_pD3D->Release();
        m_pD3D = NULL;
    }
#endif

    return nvStatus;
}

NVENCSTATUS loadframe(uint8_t *yuvInput[3], HANDLE hInputYUVFile, uint32_t frmIdx, uint32_t width, uint32_t height, uint32_t &numBytesRead, uint32_t isYuv444)
{
    uint64_t fileOffset;
    uint32_t result;
    //Set size depending on whether it is YUV 444 or YUV 420
    uint32_t dwInFrameSize = isYuv444 ? width * height * 3 : width*height + (width*height) / 2;
    fileOffset = (uint64_t)(dwInFrameSize *frmIdx);
    result = nvSetFilePointer64(hInputYUVFile, fileOffset, NULL, FILE_BEGIN);
    if (result == INVALID_SET_FILE_POINTER)
    {
        return NV_ENC_ERR_INVALID_PARAM;
    }
    if (isYuv444)
    {
        nvReadFile(hInputYUVFile, yuvInput[0], width * height, &numBytesRead, NULL);
        nvReadFile(hInputYUVFile, yuvInput[1], width * height, &numBytesRead, NULL);
        nvReadFile(hInputYUVFile, yuvInput[2], width * height, &numBytesRead, NULL);
    }
    else
    {
        nvReadFile(hInputYUVFile, yuvInput[0], width * height, &numBytesRead, NULL);
        nvReadFile(hInputYUVFile, yuvInput[1], width * height / 4, &numBytesRead, NULL);
        nvReadFile(hInputYUVFile, yuvInput[2], width * height / 4, &numBytesRead, NULL);
    }
    return NV_ENC_SUCCESS;
}

void PrintHelp()
{
    printf("Usage : NvEncoder \n"
        "-i <string>                  Specify input yuv420 file\n"
        "-o <string>                  Specify output bitstream file\n"
        "-size <int int>              Specify input resolution <width height>\n"
        "\n### Optional parameters ###\n"
        "-codec <integer>             Specify the codec \n"
        "                                 0: H264\n"
        "                                 1: HEVC\n"
        "-preset <string>             Specify the preset for encoder settings\n"
        "                                 hq : nvenc HQ \n"
        "                                 hp : nvenc HP \n"
        "                                 lowLatencyHP : nvenc low latency HP \n"
        "                                 lowLatencyHQ : nvenc low latency HQ \n"
        "                                 lossless : nvenc Lossless Default \n"
        "-startf <integer>            Specify start index for encoding. Default is 0\n"
        "-endf <integer>              Specify end index for encoding. Default is end of file\n"
        "-fps <integer>               Specify encoding frame rate\n"
        "-goplength <integer>         Specify gop length\n"
        "-numB <integer>              Specify number of B frames\n"
        "-bitrate <integer>           Specify the encoding average bitrate\n"
        "-vbvMaxBitrate <integer>     Specify the vbv max bitrate\n"
        "-vbvSize <integer>           Specify the encoding vbv/hrd buffer size\n"
        "-rcmode <integer>            Specify the rate control mode\n"
        "                                 0:  Constant QP\n"
        "                                 1:  Single pass VBR\n"
        "                                 2:  Single pass CBR\n"
        "                                 4:  Single pass VBR minQP\n"
        "                                 8:  Two pass frame quality\n"
        "                                 16: Two pass frame size cap\n"
        "                                 32: Two pass VBR\n"
        "-qp <integer>                Specify qp for Constant QP mode\n"
        "-picStruct <integer>         Specify the picture structure\n"
        "                                 1:  Progressive frame\n"
        "                                 2:  Field encoding top field first\n"
        "                                 3:  Field encoding bottom field first\n"
        "-devicetype <integer>        Specify devicetype used for encoding\n"
        "                                 0:  DX9\n"
        "                                 1:  DX11\n"
        "                                 2:  Cuda\n"
        "                                 3:  DX10\n"
        "-yuv444 <integer>             Specify the input YUV format\n"
        "                                 0: YUV 420\n"
        "                                 1: YUV 444\n"
        "-deviceID <integer>           Specify the GPU device on which encoding will take place\n"
        "-help                         Prints Help Information\n\n"
        );
}

int CNvEncoder::EncodeMain(int argc, char *argv[])
{
    HANDLE hInput;
    uint32_t numBytesRead = 0;
    uint8_t *yuv[3];
    unsigned long long lStart, lEnd, lFreq;
    int numFramesEncoded = 0;
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
    bool bError = false;
    EncodeConfig encodeConfig;

    memset(&encodeConfig, 0, sizeof(EncodeConfig));

    encodeConfig.endFrameIdx = INT_MAX;
    encodeConfig.bitrate = 5000000;
    encodeConfig.rcMode = NV_ENC_PARAMS_RC_CONSTQP;
    encodeConfig.gopLength = NVENC_INFINITE_GOPLENGTH;
    encodeConfig.deviceType = NV_ENC_CUDA;
    encodeConfig.codec = NV_ENC_H264;
    encodeConfig.fps = 30;
    encodeConfig.qp = 28;
    encodeConfig.presetGUID = NV_ENC_PRESET_DEFAULT_GUID;
    encodeConfig.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
    encodeConfig.isYuv444 = 0;

    nvStatus = m_pNvHWEncoder->ParseArguments(&encodeConfig, argc, argv);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        PrintHelp();
        return 1;
    }

    if (!encodeConfig.inputFileName || !encodeConfig.outputFileName || encodeConfig.width == 0 || encodeConfig.height == 0)
    {
        PrintHelp();
        return 1;
    }

    encodeConfig.fOutput = fopen(encodeConfig.outputFileName, "wb");
    if (encodeConfig.fOutput == NULL)
    {
        PRINTERR("Failed to create \"%s\"\n", encodeConfig.outputFileName);
        return 1;
    }

    hInput = nvOpenFile(encodeConfig.inputFileName);
    if (hInput == INVALID_HANDLE_VALUE)
    {
        PRINTERR("Failed to open \"%s\"\n", encodeConfig.inputFileName);
        return 1;
    }

    switch (encodeConfig.deviceType)
    {
#if defined(NV_WINDOWS)
    case NV_ENC_DX9:
        InitD3D9(encodeConfig.deviceID);
        break;

    case NV_ENC_DX10:
        InitD3D10(encodeConfig.deviceID);
        break;

    case NV_ENC_DX11:
        InitD3D11(encodeConfig.deviceID);
        break;
#endif
    case NV_ENC_CUDA:
        InitCuda(encodeConfig.deviceID);
        break;
    }

    if (encodeConfig.deviceType != NV_ENC_CUDA)
        nvStatus = m_pNvHWEncoder->Initialize(m_pDevice, NV_ENC_DEVICE_TYPE_DIRECTX);
    else
        nvStatus = m_pNvHWEncoder->Initialize(m_pDevice, NV_ENC_DEVICE_TYPE_CUDA);

    if (nvStatus != NV_ENC_SUCCESS)
        return 1;

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

    nvStatus = m_pNvHWEncoder->CreateEncoder(&encodeConfig);
    if (nvStatus != NV_ENC_SUCCESS)
        return 1;

    m_uEncodeBufferCount = encodeConfig.numB + 4; // min buffers is numb + 1 + 3 pipelining

    m_uPicStruct = encodeConfig.pictureStruct;

    nvStatus = AllocateIOBuffers(encodeConfig.width, encodeConfig.height, encodeConfig.isYuv444);
    if (nvStatus != NV_ENC_SUCCESS)
        return 1;

    if (encodeConfig.isYuv444)
    {
        yuv[0] = new uint8_t[encodeConfig.width*encodeConfig.height];
        yuv[1] = new uint8_t[encodeConfig.width*encodeConfig.height];
        yuv[2] = new uint8_t[encodeConfig.width*encodeConfig.height];
    }
    else
    {
        yuv[0] = new uint8_t[encodeConfig.width*encodeConfig.height];
        yuv[1] = new uint8_t[encodeConfig.width*encodeConfig.height / 4];
        yuv[2] = new uint8_t[encodeConfig.width*encodeConfig.height / 4];
    }
    NvQueryPerformanceCounter(&lStart);

    for (int frm = encodeConfig.startFrameIdx; frm <= encodeConfig.endFrameIdx; frm++)
    {
       numBytesRead = 0;
       loadframe(yuv, hInput, frm, encodeConfig.width, encodeConfig.height, numBytesRead, encodeConfig.isYuv444);
        if (numBytesRead == 0)
            break;

        EncodeFrameConfig stEncodeFrame;
        memset(&stEncodeFrame, 0, sizeof(stEncodeFrame));
        stEncodeFrame.yuv[0] = yuv[0];
        stEncodeFrame.yuv[1] = yuv[1];
        stEncodeFrame.yuv[2] = yuv[2];

        stEncodeFrame.stride[0] = encodeConfig.width;
        stEncodeFrame.stride[1] = (encodeConfig.isYuv444) ? encodeConfig.width : encodeConfig.width / 2;
        stEncodeFrame.stride[2] = (encodeConfig.isYuv444) ? encodeConfig.width : encodeConfig.width / 2;
        stEncodeFrame.width = encodeConfig.width;
        stEncodeFrame.height = encodeConfig.height;

        EncodeFrame(&stEncodeFrame, false, encodeConfig.width, encodeConfig.height);
        numFramesEncoded++;
    }

    nvStatus = EncodeFrame(NULL, true, encodeConfig.width, encodeConfig.height);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        bError = true;
        goto exit;
    }

    if (numFramesEncoded > 0)
    {
        NvQueryPerformanceCounter(&lEnd);
        NvQueryPerformanceFrequency(&lFreq);
        double elapsedTime = (double)(lEnd - lStart);
        printf("Encoded %d frames in %6.2fms\n", numFramesEncoded, (elapsedTime*1000.0)/lFreq);
        printf("Avergage Encode Time : %6.2fms\n", ((elapsedTime*1000.0)/numFramesEncoded)/lFreq);
    }

exit:
    if (encodeConfig.fOutput)
    {
        fclose(encodeConfig.fOutput);
    }

    if (hInput)
    {
        nvCloseFile(hInput);
    }

    Deinitialize(encodeConfig.deviceType);

    for (int i = 0; i < 3; i ++)
    {
        if (yuv[i])
        {
            delete [] yuv[i];
        }
    }

    return bError ? 1 : 0;
}

NVENCSTATUS CNvEncoder::EncodeFrame(EncodeFrameConfig *pEncodeFrame, bool bFlush, uint32_t width, uint32_t height)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
    uint32_t lockedPitch = 0;
    EncodeBuffer *pEncodeBuffer = NULL;

    if (bFlush)
    {
        FlushEncoder();
        return NV_ENC_SUCCESS;
    }

    if (!pEncodeFrame)
    {
        return NV_ENC_ERR_INVALID_PARAM;
    }

    pEncodeBuffer = m_EncodeBufferQueue.GetAvailable();
    if(!pEncodeBuffer)
    {
        m_pNvHWEncoder->ProcessOutput(m_EncodeBufferQueue.GetPending());
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

int main(int argc, char **argv)
{
    CNvEncoder nvEncoder;
    return nvEncoder.EncodeMain(argc, argv);
}
