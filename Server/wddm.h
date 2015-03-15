#pragma once

//#pragma comment (lib, "d3dx11.lib") 
//#pragma comment (lib, "d3dx10.lib")

#include <windows.h>
#include <windowsx.h>

//#define CINTERFACE

#include <DXGItype.h>
#include <D3D11.h>
#pragma comment (lib, "d3d11.lib") 
#include <dxgi1_2.h>

#include <tchar.h>

/* Driver types supported */
D3D_DRIVER_TYPE DriverTypes[] =
{
	D3D_DRIVER_TYPE_HARDWARE,
	D3D_DRIVER_TYPE_WARP,
	D3D_DRIVER_TYPE_REFERENCE,
};
UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

D3D_FEATURE_LEVEL FeatureLevels[] =
{
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_10_0,
	D3D_FEATURE_LEVEL_9_1
};

UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);

D3D_FEATURE_LEVEL FeatureLevel;

ID3D11Device* gDevice = NULL;
ID3D11DeviceContext* gContext = NULL;
IDXGIOutputDuplication* gOutputDuplication = NULL;

IDXGISurface* surf = NULL;
ID3D11Texture2D* sStage = NULL;

DXGI_OUTDUPL_FRAME_INFO FrameInfo;

class WDDM {
public:
	int wf_dxgi_init(UINT screenID, RECT screen)
	{
		//not sure if needed
		gAcquiredDesktopImage = NULL;

		this->screen = screen;

		if (wf_dxgi_createDevice() != 0)
		{
			return 1;
		}

		if (wf_dxgi_getDuplication(screenID) != 0)
		{
			return 1;
		}

		return 0;

	}
	int wf_dxgi_createDevice()
	{
		HRESULT status;
		UINT DriverTypeIndex;

		for (DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
		{
			/*status = D3D11CreateDevice(NULL, DriverTypes[DriverTypeIndex], NULL, D3D11_CREATE_DEVICE_DEBUG, FeatureLevels, NumFeatureLevels,
			D3D11_SDK_VERSION, &gDevice, &FeatureLevel, &gContext);
			*/
			status = D3D11CreateDevice(NULL, DriverTypes[DriverTypeIndex], NULL, NULL, FeatureLevels, NumFeatureLevels,
				D3D11_SDK_VERSION, &gDevice, &FeatureLevel, &gContext);
			if (SUCCEEDED(status))
				break;

			_tprintf(_T("D3D11CreateDevice returned [%d] for Driver Type %d\n"), status, DriverTypes[DriverTypeIndex]);
		}

		if (FAILED(status))
		{
			_tprintf(_T("Failed to create device in InitializeDx\n"));
			return 1;

			//debug
			/*
			for (DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
			{
			status = D3D11CreateDevice(NULL, DriverTypes[DriverTypeIndex], NULL, NULL, FeatureLevels, NumFeatureLevels,
			D3D11_SDK_VERSION, &gDevice, &FeatureLevel, &gContext);
			if (SUCCEEDED(status))
			break;

			_tprintf(_T("D3D11CreateDevice returned [%d] for Driver Type %d\n"), status, DriverTypes[DriverTypeIndex]);
			}

			if (FAILED(status))
			{
			_tprintf(_T("Failed to create device in InitializeDx\n"));
			return 1;
			}
			*/
		}

		return 0;
	}

	int wf_dxgi_getDuplication(UINT screenID)
	{
		HRESULT status;
		UINT dTop, i = 0;
		DXGI_OUTPUT_DESC desc;
		IDXGIOutput * pOutput;
		IDXGIDevice* DxgiDevice = NULL;
		IDXGIAdapter* DxgiAdapter = NULL;
		IDXGIOutput* DxgiOutput = NULL;
		IDXGIOutput1* DxgiOutput1 = NULL;

		status = gDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&DxgiDevice);

		if (FAILED(status))
		{
			_tprintf(_T("Failed to get QI for DXGI Device\n"));
			return 1;
		}

		status = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&DxgiAdapter);
		DxgiDevice->Release();
		DxgiDevice = NULL;

		if (FAILED(status))
		{
			_tprintf(_T("Failed to get parent DXGI Adapter\n"));
			return 1;
		}

		ZeroMemory(&desc, sizeof(desc));
		pOutput = NULL;

		while (DxgiAdapter->EnumOutputs(i, &pOutput) != DXGI_ERROR_NOT_FOUND)
		{
			DXGI_OUTPUT_DESC* pDesc = &desc;

			status = pOutput->GetDesc(pDesc);

			if (FAILED(status))
			{
				_tprintf(_T("Failed to get description\n"));
				return 1;
			}

		 wprintf(L"Output %d: [%s] [%s] (%d, %d, %d, %d)\n", i, pDesc->DeviceName, pDesc->AttachedToDesktop ? L"attached" : L"not attached",
				pDesc->DesktopCoordinates.left, pDesc->DesktopCoordinates.top, pDesc->DesktopCoordinates.right, pDesc->DesktopCoordinates.bottom);

			if (pDesc->AttachedToDesktop)
				dTop = i;

			pOutput->Release();
			++i;
		}

		dTop = screenID;

		status = DxgiAdapter->EnumOutputs(dTop, &DxgiOutput);
		DxgiAdapter->Release();
		DxgiAdapter = NULL;

		if (FAILED(status))
		{
			_tprintf(_T("Failed to get output\n"));
			return 1;
		}

		status = DxgiOutput->QueryInterface(__uuidof(DxgiOutput1), (void**)&DxgiOutput1);
		DxgiOutput->Release();
		DxgiOutput = NULL;

		if (FAILED(status))
		{
			_tprintf(_T("Failed to get IDXGIOutput1\n"));
			return 1;
		}

		status = DxgiOutput1->DuplicateOutput(gDevice, &gOutputDuplication);
		DxgiOutput1->Release();
		DxgiOutput1 = NULL;

		if (FAILED(status))
		{
			if (status == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
			{
				_tprintf(_T("There is already the maximum number of applications using the Desktop Duplication API running, please close one of those applications and then try again.\n"));
				return 1;
			}

			_tprintf(_T("Failed to get duplicate output\n"));
			return 1;
		}

		return 0;
	}
	int wf_dxgi_cleanup()
	{
		if (framesWaiting > 0)
		{
			wf_dxgi_releasePixelData();
		}

		if (gAcquiredDesktopImage)
		{
			gAcquiredDesktopImage->Release();
			gAcquiredDesktopImage = NULL;
		}

		if (gOutputDuplication)
		{
			gOutputDuplication->Release();
			gOutputDuplication = NULL;
		}

		if (gContext)
		{
			gContext->Release();
			gContext = NULL;
		}

		if (gDevice)
		{
			gDevice->Release();
			gDevice = NULL;
		}

		return 0;
	}

	int wf_dxgi_nextFrame(UINT timeout)
	{
		HRESULT status = 0;
		UINT i = 0;
		UINT DataBufferSize = 0;
		BYTE* DataBuffer = NULL;
		IDXGIResource* DesktopResource = NULL;

		if (framesWaiting > 0)
		{
			wf_dxgi_releasePixelData();
		}

		if (gAcquiredDesktopImage)
		{
			gAcquiredDesktopImage->Release();
			gAcquiredDesktopImage = NULL;
		}

		status = gOutputDuplication->AcquireNextFrame(timeout, &FrameInfo, &DesktopResource);

		if (status == DXGI_ERROR_WAIT_TIMEOUT)
		{
			return 1;
		}

		if (FAILED(status))
		{
			if (status == DXGI_ERROR_ACCESS_LOST)
			{
				_tprintf(_T("Failed to acquire next frame with status=%#X\n"), status);
				_tprintf(_T("Trying to reinitialize due to ACCESS LOST..."));
				wf_dxgi_getDuplication(0);
			}
			else
			{
				_tprintf(_T("Failed to acquire next frame with status=%#X\n"), status);
				_tprintf(_T("\tAccumulated Frames: %d\n\tRects: %d\n\tBuffSize: %d\n"),
					FrameInfo.AccumulatedFrames,
					FrameInfo.RectsCoalesced,
					FrameInfo.TotalMetadataBufferSize);

				status = gOutputDuplication->ReleaseFrame();

				if (FAILED(status))
				{
					_tprintf(_T("Failed to release frame with status=%d\n", status));
				}

				return 1;
			}
		}

		status = DesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&gAcquiredDesktopImage);
		DesktopResource->Release();
		DesktopResource = NULL;

		if (FAILED(status))
		{
			return 1;
		}

		framesWaiting = FrameInfo.AccumulatedFrames;

		return 0;
	}

	int wf_dxgi_getPixelData(BYTE** data, int* pitch, RECT* invalid)
	{
		HRESULT status;
		D3D11_BOX Box;
		DXGI_MAPPED_RECT mappedRect;
		D3D11_TEXTURE2D_DESC tDesc;

		tDesc.Width = (invalid->right - invalid->left);
		tDesc.Height = (invalid->bottom - invalid->top);
		tDesc.MipLevels = 1;
		tDesc.ArraySize = 1;
		tDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		tDesc.SampleDesc.Count = 1;
		tDesc.SampleDesc.Quality = 0;
		tDesc.Usage = D3D11_USAGE_STAGING;
		tDesc.BindFlags = 0;
		tDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		tDesc.MiscFlags = 0;

		INT OffsetX = screen.left;
		INT OffsetY = screen.top;

		Box.top = invalid->top - OffsetY;
		Box.left = invalid->left - OffsetX;
		Box.right = invalid->right - OffsetX;
		Box.bottom = invalid->bottom - OffsetY;
		Box.front = 0;
		Box.back = 1;

		status = gDevice->CreateTexture2D(&tDesc, NULL, &sStage);

		if (FAILED(status))
		{
			_tprintf(_T("Failed to create staging surface\n"));
			exit(1);
			return 1;
		}

		gContext->CopySubresourceRegion(sStage, 0, 0, 0, 0, gAcquiredDesktopImage, 0, &Box);

		status = sStage->QueryInterface(__uuidof(IDXGISurface), (void**)&surf);

		if (FAILED(status))
		{
			_tprintf(_T("Failed to QI staging surface\n"));
			exit(1);
			return 1;
		}

		surf->Map(&mappedRect, DXGI_MAP_READ);

		if (FAILED(status))
		{
			_tprintf(_T("Failed to map staging surface\n"));
			exit(1);
			return 1;
		}

		*data = mappedRect.pBits;
		*pitch = mappedRect.Pitch;

		return 0;
	}

	int wf_dxgi_releasePixelData()
	{
		HRESULT status;

		if (surf) {
			surf->Unmap();
			surf->Release();
			surf = NULL;
		}
		if (sStage) {
			sStage->Release();
			sStage = NULL;
		}

		status = gOutputDuplication->ReleaseFrame();

		if (FAILED(status))
		{
			_tprintf(_T("Failed to release frame\n"));
			return 1;
		}

		framesWaiting = 0;

		return 0;
	}
private:
	ID3D11Texture2D* gAcquiredDesktopImage;
	int framesWaiting;
	RECT screen;
};