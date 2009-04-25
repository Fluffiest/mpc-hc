/*
 * $Id$
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2007 see AUTHORS
 *
 * This file is part of mplayerc.
 *
 * Mplayerc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mplayerc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#include "mplayerc.h"
#include <atlbase.h>
#include <atlcoll.h>
#include "..\..\DSUtil\DSUtil.h"

#include <Videoacc.h>

#include <initguid.h>
#include "DX9AllocatorPresenter.h"
#include <d3d9.h>
#include <d3dx9.h>
#include <Vmr9.h>
#include "..\..\SubPic\DX9SubPic.h"
#include <RealMedia\pntypes.h>
#include <RealMedia\pnwintyp.h>
#include <RealMedia\pncom.h>
#include <RealMedia\rmavsurf.h>
#include "IQTVideoSurface.h"
#include <moreuuids.h>

#include "MacrovisionKicker.h"
#include "IPinHook.h"

#include "PixelShaderCompiler.h"
#include "MainFrm.h"

#include "AllocatorCommon.h"

#define FRAMERATE_MAX_DELTA			3000

CCritSec g_ffdshowReceive;
bool queueu_ffdshow_support = false;

CString GetWindowsErrorMessage(HRESULT _Error, HMODULE _Module)
{
	CString errmsg;
	LPVOID lpMsgBuf;
	if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_HMODULE,
		_Module, _Error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL))
	{
		errmsg = (LPCTSTR)lpMsgBuf;
		LocalFree(lpMsgBuf);
	}
	CString Temp;
	Temp.Format(L"0x%08x ", _Error);
	return Temp + errmsg;
}

bool IsVMR9InGraph(IFilterGraph* pFG)
{
	BeginEnumFilters(pFG, pEF, pBF)
		if(CComQIPtr<IVMRWindowlessControl9>(pBF)) return(true);
	EndEnumFilters
	return(false);
}

using namespace DSObjects;

//

HRESULT CreateAP9(const CLSID& clsid, HWND hWnd, ISubPicAllocatorPresenter** ppAP)
{
	CheckPointer(ppAP, E_POINTER);

	*ppAP = NULL;

	HRESULT hr = E_FAIL;
	CString Error; 
	if(clsid == CLSID_VMR9AllocatorPresenter && !(*ppAP = DNew CVMR9AllocatorPresenter(hWnd, hr, Error))
	|| clsid == CLSID_RM9AllocatorPresenter && !(*ppAP = DNew CRM9AllocatorPresenter(hWnd, hr, Error))
	|| clsid == CLSID_QT9AllocatorPresenter && !(*ppAP = DNew CQT9AllocatorPresenter(hWnd, hr, Error))
	|| clsid == CLSID_DXRAllocatorPresenter && !(*ppAP = DNew CDXRAllocatorPresenter(hWnd, hr, Error)))
		return E_OUTOFMEMORY;

	if(*ppAP == NULL)
		return E_FAIL;

	(*ppAP)->AddRef();

	if(FAILED(hr))
	{
		Error += L"\n";
		Error += GetWindowsErrorMessage(hr, NULL);
		
		MessageBox(hWnd, Error, L"Error creating DX9 allocation presenter", MB_OK|MB_ICONERROR);
		(*ppAP)->Release();
		*ppAP = NULL;
	}
	else if (!Error.IsEmpty())
	{
		MessageBox(hWnd, Error, L"Warning creating DX9 allocation presenter", MB_OK|MB_ICONWARNING);
	}

	return hr;
}

const wchar_t *GetD3DFormatStr(D3DFORMAT Format)
{
	switch (Format)
	{
	case D3DFMT_R8G8B8 : return L"R8G8B8";
	case D3DFMT_A8R8G8B8 : return L"A8R8G8B8";
	case D3DFMT_X8R8G8B8 : return L"X8R8G8B8";
	case D3DFMT_R5G6B5 : return L"R5G6B5";
	case D3DFMT_X1R5G5B5 : return L"X1R5G5B5";
	case D3DFMT_A1R5G5B5 : return L"A1R5G5B5";
	case D3DFMT_A4R4G4B4 : return L"A4R4G4B4";
	case D3DFMT_R3G3B2 : return L"R3G3B2";
	case D3DFMT_A8 : return L"A8";
	case D3DFMT_A8R3G3B2 : return L"A8R3G3B2";
	case D3DFMT_X4R4G4B4 : return L"X4R4G4B4";
	case D3DFMT_A2B10G10R10: return L"A2B10G10R10";
	case D3DFMT_A8B8G8R8 : return L"A8B8G8R8";
	case D3DFMT_X8B8G8R8 : return L"X8B8G8R8";
	case D3DFMT_G16R16 : return L"G16R16";
	case D3DFMT_A2R10G10B10: return L"A2R10G10B10";
	case D3DFMT_A16B16G16R16 : return L"A16B16G16R16";
	case D3DFMT_A8P8 : return L"A8P8";
	case D3DFMT_P8 : return L"P8";
	case D3DFMT_L8 : return L"L8";
	case D3DFMT_A8L8 : return L"A8L8";
	case D3DFMT_A4L4 : return L"A4L4";
	case D3DFMT_V8U8 : return L"V8U8";
	case D3DFMT_L6V5U5 : return L"L6V5U5";
	case D3DFMT_X8L8V8U8 : return L"X8L8V8U8";
	case D3DFMT_Q8W8V8U8 : return L"Q8W8V8U8";
	case D3DFMT_V16U16 : return L"V16U16";
	case D3DFMT_A2W10V10U10: return L"A2W10V10U10";
	case D3DFMT_UYVY : return L"UYVY";
	case D3DFMT_R8G8_B8G8: return L"R8G8_B8G8";
	case D3DFMT_YUY2 : return L"YUY2";
	case D3DFMT_G8R8_G8B8: return L"G8R8_G8B8";
	case D3DFMT_DXT1 : return L"DXT1";
	case D3DFMT_DXT2 : return L"DXT2";
	case D3DFMT_DXT3 : return L"DXT3";
	case D3DFMT_DXT4 : return L"DXT4";
	case D3DFMT_DXT5 : return L"DXT5";
	case D3DFMT_D16_LOCKABLE : return L"D16_LOCKABLE";
	case D3DFMT_D32: return L"D32";
	case D3DFMT_D15S1: return L"D15S1";
	case D3DFMT_D24S8: return L"D24S8";
	case D3DFMT_D24X8: return L"D24X8";
	case D3DFMT_D24X4S4: return L"D24X4S4";
	case D3DFMT_D16: return L"D16";
	case D3DFMT_D32F_LOCKABLE: return L"D32F_LOCKABLE";
	case D3DFMT_D24FS8 : return L"D24FS8";
	case D3DFMT_D32_LOCKABLE : return L"D32_LOCKABLE";
	case D3DFMT_S8_LOCKABLE: return L"S8_LOCKABLE";
	case D3DFMT_L16: return L"L16";
	case D3DFMT_VERTEXDATA : return L"VERTEXDATA";
	case D3DFMT_INDEX16: return L"INDEX16";
	case D3DFMT_INDEX32: return L"INDEX32";
	case D3DFMT_Q16W16V16U16 : return L"Q16W16V16U16";
	case D3DFMT_MULTI2_ARGB8 : return L"MULTI2_ARGB8";
	case D3DFMT_R16F : return L"R16F";
	case D3DFMT_G16R16F: return L"G16R16F";
	case D3DFMT_A16B16G16R16F: return L"A16B16G16R16F";
	case D3DFMT_R32F : return L"R32F";
	case D3DFMT_G32R32F: return L"G32R32F";
	case D3DFMT_A32B32G32R32F: return L"A32B32G32R32F";
	case D3DFMT_CxV8U8 : return L"CxV8U8";
	case D3DFMT_A1 : return L"A1";
	case D3DFMT_BINARYBUFFER : return L"BINARYBUFFER";
	}
	return L"Unknown";
}

//

#pragma pack(push, 1)
template<int texcoords>
struct MYD3DVERTEX {float x, y, z, rhw; struct {float u, v;} t[texcoords];};
template<>
struct MYD3DVERTEX<0> 
{
	float x, y, z, rhw; 
	DWORD Diffuse;
};
#pragma pack(pop)

template<int texcoords>
static void AdjustQuad(MYD3DVERTEX<texcoords>* v, float dx, float dy)
{
	float offset = 0.5f;

	for(int i = 0; i < 4; i++)
	{
		v[i].x -= offset;
		v[i].y -= offset;
		
		for(int j = 0; j < texcoords-1; j++)
		{
			v[i].t[j].u -= offset*dx;
			v[i].t[j].v -= offset*dy;
		}

		if(texcoords > 1)
		{
			v[i].t[texcoords-1].u -= offset;
			v[i].t[texcoords-1].v -= offset;
		}
	}
}

template<int texcoords>
static HRESULT TextureBlt(CComPtr<IDirect3DDevice9> pD3DDev, MYD3DVERTEX<texcoords> v[4], D3DTEXTUREFILTERTYPE filter = D3DTEXF_LINEAR)
{
	if(!pD3DDev)
		return E_POINTER;

	DWORD FVF = 0;

	switch(texcoords)
	{
	case 1: FVF = D3DFVF_TEX1; break;
	case 2: FVF = D3DFVF_TEX2; break;
	case 3: FVF = D3DFVF_TEX3; break;
	case 4: FVF = D3DFVF_TEX4; break;
	case 5: FVF = D3DFVF_TEX5; break;
	case 6: FVF = D3DFVF_TEX6; break;
	case 7: FVF = D3DFVF_TEX7; break;
	case 8: FVF = D3DFVF_TEX8; break;
	default: return E_FAIL;
	}

	HRESULT hr;

    do
	{
        hr = pD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        hr = pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
		hr = pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
		hr = pD3DDev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    	hr = pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		hr = pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); 
		hr = pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE); 
		hr = pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED); 

		for(int i = 0; i < texcoords; i++)
		{
			hr = pD3DDev->SetSamplerState(i, D3DSAMP_MAGFILTER, filter);
			hr = pD3DDev->SetSamplerState(i, D3DSAMP_MINFILTER, filter);
			hr = pD3DDev->SetSamplerState(i, D3DSAMP_MIPFILTER, filter);

			hr = pD3DDev->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
			hr = pD3DDev->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		}

		//

        hr = pD3DDev->SetFVF(D3DFVF_XYZRHW | FVF);
		// hr = pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(v[0]));

		MYD3DVERTEX<texcoords> tmp = v[2]; v[2] = v[3]; v[3] = tmp;
		hr = pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(v[0]));	

        //

		for(int i = 0; i < texcoords; i++)
		{
			pD3DDev->SetTexture(i, NULL);
		}

		return S_OK;
    }
	while(0);

    return E_FAIL;
}

static HRESULT DrawRect(CComPtr<IDirect3DDevice9> pD3DDev, MYD3DVERTEX<0> v[4])
{
	if(!pD3DDev)
		return E_POINTER;

    do
	{
        HRESULT hr = pD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        hr = pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
		hr = pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
		hr = pD3DDev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    	hr = pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		hr = pD3DDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA); 
		hr = pD3DDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA); 
		//D3DRS_COLORVERTEX 
		hr = pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); 
		hr = pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE); 
		

		hr = pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED); 

        hr = pD3DDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX0 | D3DFVF_DIFFUSE);
		// hr = pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(v[0]));

		MYD3DVERTEX<0> tmp = v[2]; v[2] = v[3]; v[3] = tmp;
		hr = pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(v[0]));	

		return S_OK;
    }
	while(0);

    return E_FAIL;
}

// CDX9AllocatorPresenter

CDX9AllocatorPresenter::CDX9AllocatorPresenter(HWND hWnd, HRESULT& hr, bool bIsEVR, CString &_Error) 
	: ISubPicAllocatorPresenterImpl(hWnd, hr, &_Error)
	, m_ScreenSize(0, 0)
	, m_RefreshRate(0)
	, m_bicubicA(0)
	, m_nTearingPos(0)
	, m_nNbDXSurface(1)
	, m_nVMR9Surfaces(0)
	, m_iVMR9Surface(0)
	, m_nCurSurface(0)
	, m_rtTimePerFrame(0)
	, m_bInterlaced(0)
	, m_nUsedBuffer(0)
	, m_bNeedPendingResetDevice(0)
	, m_bPendingResetDevice(0)
	, m_OrderedPaint(0)
	, m_bCorrectedFrameTime(0)
	, m_FrameTimeCorrection(0)
	, m_LastSampleTime(0)
	, m_LastFrameDuration(0)
	, m_bAlternativeVSync(0)
	, m_bIsEVR(bIsEVR)
	, m_VSyncMode(0)
	, m_TextScale(1.0)

{

	m_pDirectDraw = NULL;
	m_hVSyncThread = INVALID_HANDLE_VALUE;
	m_hEvtQuit = INVALID_HANDLE_VALUE;

	m_bIsFullscreen = (AfxGetApp()->m_pMainWnd != NULL) && (((CMainFrame*)AfxGetApp()->m_pMainWnd)->IsD3DFullScreenMode());

	HINSTANCE		hDll;

	if(FAILED(hr)) 
	{
		_Error += L"ISubPicAllocatorPresenterImpl failed\n";
		return;
	}

	m_pD3DXLoadSurfaceFromMemory	= NULL;
	m_pD3DXCreateLine				= NULL;
	m_pD3DXCreateFont				= NULL;
	m_pD3DXCreateSprite				= NULL;
	hDll							= AfxGetMyApp()->GetD3X9Dll();
	if(hDll)
	{
		(FARPROC&)m_pD3DXLoadSurfaceFromMemory	= GetProcAddress(hDll, "D3DXLoadSurfaceFromMemory");
		(FARPROC&)m_pD3DXCreateLine				= GetProcAddress(hDll, "D3DXCreateLine");
		(FARPROC&)m_pD3DXCreateFont				= GetProcAddress(hDll, "D3DXCreateFontW");
		(FARPROC&)m_pD3DXCreateSprite			= GetProcAddress(hDll, "D3DXCreateSprite");		
	}
	else
	{
		_Error += L"No D3DX9 dll found. To enable stats, shaders and complex resizers, please make sure to install the latest DirectX End-User Runtime.\n";
	}

	m_pDwmIsCompositionEnabled = NULL;
	m_pDwmEnableComposition = NULL;
	m_hDWMAPI = LoadLibrary(L"dwmapi.dll");
	if (m_hDWMAPI)
	{
		(FARPROC &)m_pDwmIsCompositionEnabled = GetProcAddress(m_hDWMAPI, "DwmIsCompositionEnabled");
		(FARPROC &)m_pDwmEnableComposition = GetProcAddress(m_hDWMAPI, "DwmEnableComposition");
	}

	m_hD3D9 = LoadLibrary(L"d3d9.dll");
	if (m_hD3D9)
	{
		(FARPROC &)m_pDirect3DCreate9Ex = GetProcAddress(m_hD3D9, "Direct3DCreate9Ex");
	}
	else
		m_pDirect3DCreate9Ex = NULL;

	if (m_pDirect3DCreate9Ex)
	{
		m_pDirect3DCreate9Ex(D3D_SDK_VERSION, &m_pD3DEx);
		if(!m_pD3DEx) 
		{
			m_pDirect3DCreate9Ex(D3D9b_SDK_VERSION, &m_pD3DEx);
		}
	}
	if(!m_pD3DEx) 
	{
		m_pD3D.Attach(Direct3DCreate9(D3D_SDK_VERSION));
		if(!m_pD3D) 
		{
			m_pD3D.Attach(Direct3DCreate9(D3D9b_SDK_VERSION));
		}
		if(!m_pD3D) {hr = E_FAIL; return;}
	}
	else
		m_pD3D = m_pD3DEx;


	m_DetectedFrameRate = 0.0;
	m_DetectedFrameTime = 0.0;
	m_DetectedFrameTimeStdDev = 0.0;
	m_DetectedLock = false;
	ZeroMemory(m_DetectedFrameTimeHistory, sizeof(m_DetectedFrameTimeHistory));
	ZeroMemory(m_DetectedFrameTimeHistoryHisotry, sizeof(m_DetectedFrameTimeHistoryHisotry));	
	m_DetectedFrameTimePos = 0;
	ZeroMemory(&m_VMR9AlphaBitmap, sizeof(m_VMR9AlphaBitmap));

	ZeroMemory(m_ldDetectedRefreshRateList, sizeof(m_ldDetectedRefreshRateList));
	ZeroMemory(m_ldDetectedScanlineRateList, sizeof(m_ldDetectedScanlineRateList));
	m_DetectedRefreshRatePos = 0;
	m_DetectedRefreshTimePrim = 0;
	m_DetectedScanlineTime = 0;
	m_DetectedScanlineTimePrim = 0;
	m_DetectedRefreshRate = 0;
	AppSettings& s = AfxGetAppSettings();

	if (s.m_RenderSettings.iVMRDisableDesktopComposition)
	{
		m_bDesktopCompositionDisabled = true;
		if (m_pDwmEnableComposition)
			m_pDwmEnableComposition(0);
	}
	else
	{
		m_bDesktopCompositionDisabled = false;
	}

	hr = CreateDevice(_Error);

	memset (m_pllJitter, 0, sizeof(m_pllJitter));
	memset (m_pllSyncOffset, 0, sizeof(m_pllSyncOffset));
	m_nNextJitter		= 0;
	m_nNextSyncOffset = 0;
	m_llLastPerf		= 0;
	m_fAvrFps			= 0.0;
	m_fJitterStdDev		= 0.0;
	m_fSyncOffsetStdDev = 0.0;
	m_fSyncOffsetAvr	= 0.0;
	m_bSyncStatsAvailable = false;
}

CDX9AllocatorPresenter::~CDX9AllocatorPresenter() 
{
	if (m_bDesktopCompositionDisabled)
	{
		m_bDesktopCompositionDisabled = false;
		if (m_pDwmEnableComposition)
			m_pDwmEnableComposition(1);
	}

	StopWorkerThreads();
	m_pFont		= NULL;
	m_pLine		= NULL;
    m_pD3DDev	= NULL;
	m_pD3DDevEx = NULL;
	m_pPSC.Free();
	m_pD3D.Detach();
	m_pD3DEx.Detach();
	if (m_hDWMAPI)
	{
		FreeLibrary(m_hDWMAPI);
		m_hDWMAPI = NULL;
	}
	if (m_hD3D9)
	{
		FreeLibrary(m_hD3D9);
		m_hD3D9 = NULL;
	}
}

void ModerateFloat(double& Value, double Target, double& ValuePrim, double ChangeSpeed);


void CDX9AllocatorPresenter::VSyncThread()
{
	HANDLE				hAvrt;
	HANDLE				hEvts[]		= { m_hEvtQuit};
	bool				bQuit		= false;
    TIMECAPS			tc;
	DWORD				dwResolution;
	DWORD				dwUser = 0;
	DWORD				dwTaskIndex	= 0;

	// Tell Vista Multimedia Class Scheduler we are a playback thretad (increase priority)
//	if (pfAvSetMmThreadCharacteristicsW)	
//		hAvrt = pfAvSetMmThreadCharacteristicsW (L"Playback", &dwTaskIndex);
//	if (pfAvSetMmThreadPriority)			
//		pfAvSetMmThreadPriority (hAvrt, AVRT_PRIORITY_HIGH /*AVRT_PRIORITY_CRITICAL*/);
	Sleep(2000);

    timeGetDevCaps(&tc, sizeof(TIMECAPS));
    dwResolution = min(max(tc.wPeriodMin, 0), tc.wPeriodMax);
    dwUser		= timeBeginPeriod(dwResolution);
	CMPlayerCApp *pApp = (CMPlayerCApp*)AfxGetApp();
	AppSettings& s = AfxGetAppSettings();

	while (!bQuit)
	{

		DWORD dwObject = WaitForMultipleObjects (countof(hEvts), hEvts, FALSE, 1);
		switch (dwObject)
		{
		case WAIT_OBJECT_0 :
			bQuit = true;
			break;
		case WAIT_TIMEOUT :
			{
				// Do our stuff
				if (m_pD3DDev && s.m_RenderSettings.iVMR9VSync)
				{

					int VSyncPos = GetVBlackPos();
					int WaitRange = max(m_ScreenSize.cy / 40, 5);
					int MinRange = max(min(int(0.003 * double(m_ScreenSize.cy) * double(m_RefreshRate) + 0.5), m_ScreenSize.cy/3), 5); // 1.8  ms or max 33 % of Time

					VSyncPos += MinRange + WaitRange;

					VSyncPos = VSyncPos % m_ScreenSize.cy;
					if (VSyncPos < 0)
						VSyncPos += m_ScreenSize.cy;

					int ScanLine = 0; 
					int bInVBlank = 0;
					int StartScanLine = ScanLine;
					int LastPos = ScanLine;
					ScanLine = (VSyncPos + 1) % m_ScreenSize.cy;
					if (ScanLine < 0)
						ScanLine += m_ScreenSize.cy;
					int FirstScanLine = ScanLine;
					int ScanLineMiddle = ScanLine + m_ScreenSize.cy/2;
					ScanLineMiddle = ScanLineMiddle % m_ScreenSize.cy;
					if (ScanLineMiddle < 0)
						ScanLineMiddle += m_ScreenSize.cy;

					int ScanlineStart = ScanLine;
					WaitForVBlankRange(ScanlineStart, 5, true, true, false);
					LONGLONG TimeStart = pApp->GetPerfCounter();

					WaitForVBlankRange(ScanLineMiddle, 5, true, true, false);
					LONGLONG TimeMiddle = pApp->GetPerfCounter();

					int ScanlineEnd = ScanLine;
					WaitForVBlankRange(ScanlineEnd, 5, true, true, false);
					LONGLONG TimeEnd = pApp->GetPerfCounter();

					double nSeconds = double(TimeEnd - TimeStart) / 10000000.0;
					LONGLONG DiffMiddle = TimeMiddle - TimeStart;
					LONGLONG DiffEnd = TimeEnd - TimeMiddle;
					double DiffDiff;
					if (DiffEnd > DiffMiddle)
						DiffDiff = double(DiffEnd) / double(DiffMiddle);
					else
						DiffDiff = double(DiffMiddle) / double(DiffEnd);
					if (nSeconds > 0.003 && DiffDiff < 1.3)
					{
						double ScanLineSeconds;
						double nScanLines;
						if (ScanLineMiddle > ScanlineEnd)
						{
							 ScanLineSeconds = double(TimeMiddle - TimeStart) / 10000000.0;
							 nScanLines = ScanLineMiddle - ScanlineStart;
						}
						else
						{
							 ScanLineSeconds = double(TimeEnd - TimeMiddle) / 10000000.0;
							 nScanLines = ScanlineEnd - ScanLineMiddle;
						}

						double ScanLineTime = ScanLineSeconds / nScanLines;

						int iPos = m_DetectedRefreshRatePos	% 100;
						m_ldDetectedScanlineRateList[iPos] = ScanLineTime;
						if (m_DetectedScanlineTime && ScanlineStart != ScanlineEnd)
						{
							int Diff = ScanlineEnd - ScanlineStart;
							nSeconds -= double(Diff) * m_DetectedScanlineTime;
						}
						m_ldDetectedRefreshRateList[iPos] = nSeconds;
						double Average = 0;
						double AverageScanline = 0;
						int nPos = min(iPos + 1, 100);
						for (int i = 0; i < nPos; ++i)
						{
							Average += m_ldDetectedRefreshRateList[i];
							AverageScanline += m_ldDetectedScanlineRateList[i];
						}

						Average /= double(nPos);
						AverageScanline /= double(nPos);

						double ThisValue = Average;

						{
							CAutoLock Lock(&m_RefreshRateLock);							
							++m_DetectedRefreshRatePos;
							if (m_DetectedRefreshTime == 0 || m_DetectedRefreshTime / ThisValue > 1.01 || m_DetectedRefreshTime / ThisValue < 0.99)
							{
								m_DetectedRefreshTime = ThisValue;
								m_DetectedRefreshTimePrim = 0;
							}
							ModerateFloat(m_DetectedRefreshTime, ThisValue, m_DetectedRefreshTimePrim, 1.5);
							m_DetectedRefreshRate = 1.0/m_DetectedRefreshTime;

							if (m_DetectedScanlineTime == 0 || m_DetectedScanlineTime / AverageScanline > 1.01 || m_DetectedScanlineTime / AverageScanline < 0.99)
							{
								m_DetectedScanlineTime = AverageScanline;
								m_DetectedScanlineTimePrim = 0;
							}
							ModerateFloat(m_DetectedScanlineTime, AverageScanline, m_DetectedScanlineTimePrim, 1.5);
							m_DetectedScanlinesPerFrame = m_DetectedRefreshTime / m_DetectedScanlineTime;
						}
						//TRACE("Refresh: %f\n", RefreshRate);
					}
				}
				else
				{
					m_DetectedRefreshRate = 0.0;
					m_DetectedScanlinesPerFrame = 0.0;
				}
			}
			break;
		}
	}

	timeEndPeriod (dwResolution);
//	if (pfAvRevertMmThreadCharacteristics) pfAvRevertMmThreadCharacteristics (hAvrt);
}


DWORD WINAPI CDX9AllocatorPresenter::VSyncThreadStatic(LPVOID lpParam)
{
	CDX9AllocatorPresenter*		pThis = (CDX9AllocatorPresenter*) lpParam;
	pThis->VSyncThread();
	return 0;
}

void CDX9AllocatorPresenter::StartWorkerThreads()
{
	DWORD		dwThreadId;

	m_hEvtQuit		= CreateEvent (NULL, TRUE, FALSE, NULL);
	if (m_bIsEVR)
	{
		m_hVSyncThread = ::CreateThread(NULL, 0, VSyncThreadStatic, (LPVOID)this, 0, &dwThreadId);
		SetThreadPriority(m_hVSyncThread, THREAD_PRIORITY_HIGHEST);
	}
}

void CDX9AllocatorPresenter::StopWorkerThreads()
{
	SetEvent (m_hEvtQuit);
	if ((m_hVSyncThread != INVALID_HANDLE_VALUE) && (WaitForSingleObject (m_hVSyncThread, 10000) == WAIT_TIMEOUT))
	{
		ASSERT (FALSE);
		TerminateThread (m_hVSyncThread, 0xDEAD);
	}

	if (m_hVSyncThread		 != INVALID_HANDLE_VALUE) CloseHandle (m_hVSyncThread);
	if (m_hEvtQuit		 != INVALID_HANDLE_VALUE) CloseHandle (m_hEvtQuit);
	m_hVSyncThread = INVALID_HANDLE_VALUE;
	m_hEvtQuit = INVALID_HANDLE_VALUE;

}

bool CDX9AllocatorPresenter::SettingsNeedResetDevice()
{
	AppSettings& s = AfxGetAppSettings();
	CMPlayerCApp::Settings::CRendererSettingsEVR & New = AfxGetAppSettings().m_RenderSettings;
	CMPlayerCApp::Settings::CRendererSettingsEVR & Current = m_LastRendererSettings;

	bool bRet = false;

	bRet = bRet || New.fVMR9AlterativeVSync != Current.fVMR9AlterativeVSync;
	bRet = bRet || New.iVMR9VSyncAccurate != Current.iVMR9VSyncAccurate;

	if (m_bIsFullscreen)
	{
		bRet = bRet || New.iVMR9FullscreenGUISupport != Current.iVMR9FullscreenGUISupport;
	}
	else
	{
		if (Current.iVMRDisableDesktopComposition)
		{
			if (!m_bDesktopCompositionDisabled)
			{
				m_bDesktopCompositionDisabled = true;
				if (m_pDwmEnableComposition)
					m_pDwmEnableComposition(0);
			}
		}
		else
		{
			if (m_bDesktopCompositionDisabled)
			{
				m_bDesktopCompositionDisabled = false;
				if (m_pDwmEnableComposition)
					m_pDwmEnableComposition(1);
			}
		}
	}

	if (m_bIsEVR)
	{
		bRet = bRet || New.iEVRHighColorResolution != Current.iEVRHighColorResolution;		
	}

	m_LastRendererSettings = s.m_RenderSettings;

	return bRet;
}

HRESULT CDX9AllocatorPresenter::CreateDevice(CString &_Error)
{
	StopWorkerThreads();
	AppSettings& s = AfxGetAppSettings();
	m_VBlankEndWait = 0;
	m_VBlankMin = 300000;
	m_VBlankMinCalc = 300000;
	m_VBlankMax = 0;
	m_VBlankStartWait = 0;
	m_VBlankWaitTime = 0;
	m_PresentWaitTime = 0;
	m_PresentWaitTimeMin = 3000000000;
	m_PresentWaitTimeMax = 0;

	m_LastRendererSettings = s.m_RenderSettings;

	m_VBlankEndPresent = -100000;
	m_VBlankStartMeasureTime = 0;
	m_VBlankStartMeasure = 0;

	m_PaintTime = 0;
	m_PaintTimeMin = 3000000000;
	m_PaintTimeMax = 0;

	m_RasterStatusWaitTime = 0;
	m_RasterStatusWaitTimeMin = 3000000000;
	m_RasterStatusWaitTimeMax = 0;
	m_RasterStatusWaitTimeMaxCalc = 0;
	
	m_ClockDiff = 0.0;
	m_ClockDiffPrim = 0.0;
	m_ClockDiffCalc = 0.0;

	m_ModeratedTimeSpeed = 1.0;
	m_ModeratedTimeSpeedDiff = 0.0;
	m_ModeratedTimeSpeedPrim = 0;
	ZeroMemory(m_TimeChangeHisotry, sizeof(m_TimeChangeHisotry));
	ZeroMemory(m_ClockChangeHisotry, sizeof(m_ClockChangeHisotry));
	m_ClockTimeChangeHistoryPos = 0;

	m_pPSC.Free();
    m_pD3DDev = NULL;
	m_pD3DDevEx = NULL;
	m_pDirectDraw = NULL;

	m_pResizerPixelShader[0] = 0;
	m_pResizerPixelShader[1] = 0;
	m_pResizerPixelShader[2] = 0;

	POSITION pos = m_pPixelShadersScreenSpace.GetHeadPosition();
	while(pos)
	{
		CExternalPixelShader &Shader = m_pPixelShadersScreenSpace.GetNext(pos);
		Shader.m_pPixelShader = NULL;
	}
	pos = m_pPixelShaders.GetHeadPosition();
	while(pos)
	{
		CExternalPixelShader &Shader = m_pPixelShaders.GetNext(pos);
		Shader.m_pPixelShader = NULL;
	}


	D3DDISPLAYMODE d3ddm;
	HRESULT hr;
	ZeroMemory(&d3ddm, sizeof(d3ddm));
	if(FAILED(m_pD3D->GetAdapterDisplayMode(GetAdapter(m_pD3D), &d3ddm)))
	{
		_Error += L"GetAdapterDisplayMode failed\n";
		return E_UNEXPECTED;
	}

	/*		// TODO : add nVidia PerfHUD !!!
	
// Set default settings 
UINT AdapterToUse=D3DADAPTER_DEFAULT; 
D3DDEVTYPE DeviceType=D3DDEVTYPE_HAL; 
 
#if SHIPPING_VERSION 
// When building a shipping version, disable PerfHUD (opt-out) 
#else 
// Look for 'NVIDIA PerfHUD' adapter 
// If it is present, override default settings 
for (UINT Adapter=0;Adapter<g_pD3D->GetAdapterCount();Adapter++)  
{ 
  D3DADAPTER_IDENTIFIER9  Identifier; 
      HRESULT       Res; 
 
Res = g_pD3D->GetAdapterIdentifier(Adapter,0,&Identifier); 
  if (strstr(Identifier.Description,"PerfHUD") != 0) 
 { 
  AdapterToUse=Adapter; 
  DeviceType=D3DDEVTYPE_REF; 
  break; 
 } 
} 
#endif 
 
if (FAILED(g_pD3D->CreateDevice( AdapterToUse, DeviceType, hWnd, 
  D3DCREATE_HARDWARE_VERTEXPROCESSING, 
    &d3dpp, &g_pd3dDevice) ) ) 
{ 
 return E_FAIL; 
} 
	*/


//#define ENABLE_DDRAWSYNC
#ifdef ENABLE_DDRAWSYNC
    hr = DirectDrawCreate(NULL, &m_pDirectDraw, NULL) ;
    if (hr == S_OK)
	{
	    hr = m_pDirectDraw->SetCooperativeLevel(m_hWnd, DDSCL_NORMAL) ;
	}
#endif

	m_RefreshRate = d3ddm.RefreshRate;
	m_ScreenSize.SetSize(d3ddm.Width, d3ddm.Height);

    D3DPRESENT_PARAMETERS pp;
    ZeroMemory(&pp, sizeof(pp));

	BOOL bCompositionEnabled = false;
	if (m_pDwmIsCompositionEnabled)
		m_pDwmIsCompositionEnabled(&bCompositionEnabled);

	m_bCompositionEnabled = bCompositionEnabled != 0;

	m_bAlternativeVSync = s.m_RenderSettings.fVMR9AlterativeVSync;
	m_bHighColorResolution = s.m_RenderSettings.iEVRHighColorResolution && m_bIsEVR;

	if (m_bIsFullscreen)
	{
		pp.Windowed = false; 
		pp.BackBufferWidth = d3ddm.Width; 
		pp.BackBufferHeight = d3ddm.Height; 
		pp.hDeviceWindow = m_hWnd;
		if(m_bAlternativeVSync)
		{
			pp.BackBufferCount = 3; 
			pp.SwapEffect = D3DSWAPEFFECT_DISCARD;		// Ne pas mettre D3DSWAPEFFECT_COPY car cela entraine une desynchro audio sur les MKV ! // Copy needed for sync now? FLIP only stutters.
			pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		}
		else
		{
			pp.BackBufferCount = 3; 
			pp.SwapEffect = D3DSWAPEFFECT_DISCARD;		// Ne pas mettre D3DSWAPEFFECT_COPY car cela entraine une desynchro audio sur les MKV ! // Copy needed for sync now? FLIP only stutters.
			pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
		}
		pp.Flags = D3DPRESENTFLAG_VIDEO;
		if (s.m_RenderSettings.iVMR9FullscreenGUISupport && !m_bHighColorResolution)
			pp.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
		if (m_bHighColorResolution)
			pp.BackBufferFormat = D3DFMT_A2R10G10B10;
		else
			pp.BackBufferFormat = d3ddm.Format;
		
		m_D3DDevExError = L"No m_pD3DEx";
		if (m_pD3DEx)
		{
			D3DDISPLAYMODEEX DisplayMode;
			ZeroMemory(&DisplayMode, sizeof(DisplayMode));
			DisplayMode.Size = sizeof(DisplayMode);
			m_pD3DEx->GetAdapterDisplayModeEx(GetAdapter(m_pD3DEx), &DisplayMode, NULL);

			DisplayMode.Format = pp.BackBufferFormat;
			pp.FullScreen_RefreshRateInHz = DisplayMode.RefreshRate;

			hr = m_pD3DEx->CreateDeviceEx(
								GetAdapter(m_pD3D), D3DDEVTYPE_HAL, m_hWnd,
								D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED, //D3DCREATE_MANAGED 
								&pp, &DisplayMode, &m_pD3DDevEx);

			m_D3DDevExError = GetWindowsErrorMessage(hr, m_hD3D9);
			if (m_pD3DDevEx)
			{
				m_pD3DDev = m_pD3DDevEx;
				m_BackbufferType = pp.BackBufferFormat;
				m_DisplayType = DisplayMode.Format;
			}
		}

		if (!m_pD3DDev)
		{
			hr = m_pD3D->CreateDevice(
								GetAdapter(m_pD3D), D3DDEVTYPE_HAL, m_hWnd,
								D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED, //D3DCREATE_MANAGED 
								&pp, &m_pD3DDev);
			if (m_pD3DDev)
			{
				m_BackbufferType = pp.BackBufferFormat;
				m_DisplayType = d3ddm.Format;
			}
		}
		if (m_pD3DDev && s.m_RenderSettings.iVMR9FullscreenGUISupport && !m_bHighColorResolution)
		{
			m_pD3DDev->SetDialogBoxMode(true);
			//if (m_pD3DDev->SetDialogBoxMode(true) != S_OK)
			//	ExitProcess(0);

		}

		TRACE("CreateDevice: %d\n", (LONG)hr);
		ASSERT (SUCCEEDED (hr));
	}
	else
	{
		pp.Windowed = TRUE;
		pp.hDeviceWindow = m_hWnd;
		pp.SwapEffect = D3DSWAPEFFECT_COPY;
		pp.Flags = D3DPRESENTFLAG_VIDEO;
		pp.BackBufferCount = 1; 
		pp.BackBufferWidth = d3ddm.Width;
		pp.BackBufferHeight = d3ddm.Height;
		m_BackbufferType = d3ddm.Format;
		m_DisplayType = d3ddm.Format;
		if (m_bHighColorResolution)
		{
			m_BackbufferType = D3DFMT_A2R10G10B10;
			pp.BackBufferFormat = D3DFMT_A2R10G10B10;
		}
		if (bCompositionEnabled || m_bAlternativeVSync)
		{
			// Desktop composition takes care of the VSYNC
			pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		}
		else
		{
			pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
		}

//		if(m_fVMRSyncFix = AfxGetMyApp()->m_s.fVMRSyncFix)
//			pp.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

		if (m_pD3DEx)
		{
			hr = m_pD3DEx->CreateDeviceEx(
								GetAdapter(m_pD3D), D3DDEVTYPE_HAL, m_hWnd,
								D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED, //D3DCREATE_MANAGED 
								&pp, NULL, &m_pD3DDevEx);
			if (m_pD3DDevEx)
				m_pD3DDev = m_pD3DDevEx;
		}
		else
		{
			hr = m_pD3D->CreateDevice(
							GetAdapter(m_pD3D), D3DDEVTYPE_HAL, m_hWnd,
							D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED, //D3DCREATE_MANAGED 
							&pp, &m_pD3DDev);
		}
	}

	if (m_pD3DDevEx)
	{
		m_pD3DDevEx->SetGPUThreadPriority(7);
	}

	if(FAILED(hr))
	{
		_Error += L"CreateDevice failed\n";

		return hr;
	}

	m_pPSC.Attach(DNew CPixelShaderCompiler(m_pD3DDev, true));

	//

	m_filter = D3DTEXF_NONE;

    ZeroMemory(&m_caps, sizeof(m_caps));
	m_pD3DDev->GetDeviceCaps(&m_caps);

	if((m_caps.StretchRectFilterCaps&D3DPTFILTERCAPS_MINFLINEAR)
	&& (m_caps.StretchRectFilterCaps&D3DPTFILTERCAPS_MAGFLINEAR))
		m_filter = D3DTEXF_LINEAR;

	//

	m_bicubicA = 0;

	//

	CComPtr<ISubPicProvider> pSubPicProvider;
	if(m_pSubPicQueue) m_pSubPicQueue->GetSubPicProvider(&pSubPicProvider);

	CSize size;
	switch(AfxGetAppSettings().nSPCMaxRes)
	{
	case 0: default: size = m_ScreenSize; break;
	case 1: size.SetSize(1024, 768); break;
	case 2: size.SetSize(800, 600); break;
	case 3: size.SetSize(640, 480); break;
	case 4: size.SetSize(512, 384); break;
	case 5: size.SetSize(384, 288); break;
	case 6: size.SetSize(2560, 1600); break;
	case 7: size.SetSize(1920, 1080); break;
	case 8: size.SetSize(1320, 900); break;
	case 9: size.SetSize(1280, 720); break;
	}

	if(m_pAllocator)
	{
		m_pAllocator->ChangeDevice(m_pD3DDev);
	}
	else
	{
		m_pAllocator = DNew CDX9SubPicAllocator(m_pD3DDev, size, AfxGetAppSettings().fSPCPow2Tex);
		if(!m_pAllocator)
		{
			_Error += L"CDX9SubPicAllocator failed\n";

			return E_FAIL;
		}
	}

	hr = S_OK;
	m_pSubPicQueue = AfxGetAppSettings().nSPCSize > 0 
		? (ISubPicQueue*)DNew CSubPicQueue(AfxGetAppSettings().nSPCSize, AfxGetAppSettings().fSPCDisableAnim, m_pAllocator, &hr)
		: (ISubPicQueue*)DNew CSubPicQueueNoThread(m_pAllocator, &hr);
	if(!m_pSubPicQueue || FAILED(hr))
	{
		_Error += L"m_pSubPicQueue failed\n";

		return E_FAIL;
	}

	if(pSubPicProvider) m_pSubPicQueue->SetSubPicProvider(pSubPicProvider);

	m_pFont = NULL;
	if (m_pD3DXCreateFont)
	{
		int MinSize = 1600;
		int CurrentSize = min(m_ScreenSize.cx, MinSize);
		double Scale = double(CurrentSize) / double(MinSize);
		m_TextScale = Scale;
		m_pD3DXCreateFont( m_pD3DDev,            // D3D device
							 -24.0*Scale,               // Height
							 -11.0*Scale,                     // Width
							 CurrentSize < 800 ? FW_NORMAL : FW_BOLD,               // Weight
							 0,                     // MipLevels, 0 = autogen mipmaps
							 FALSE,                 // Italic
							 DEFAULT_CHARSET,       // CharSet
							 OUT_DEFAULT_PRECIS,    // OutputPrecision
							 ANTIALIASED_QUALITY,       // Quality
							 FIXED_PITCH | FF_DONTCARE, // PitchAndFamily
							 L"Lucida Console",              // pFaceName
							 &m_pFont);              // ppFont
	}


	m_pSprite = NULL;

	if (m_pD3DXCreateSprite)
	{
		m_pD3DXCreateSprite( m_pD3DDev,            // D3D device
							 &m_pSprite);
	}

	m_pLine = NULL;
	if (m_pD3DXCreateLine)
		m_pD3DXCreateLine (m_pD3DDev, &m_pLine);

	StartWorkerThreads();

	return S_OK;
} 

HRESULT CDX9AllocatorPresenter::AllocSurfaces(D3DFORMAT Format)
{
    CAutoLock cAutoLock(this);
	CAutoLock cRenderLock(&m_RenderLock);

	AppSettings& s = AfxGetAppSettings();

	for(int i = 0; i < m_nNbDXSurface+2; i++)
	{
		m_pVideoTexture[i] = NULL;
		m_pVideoSurface[i] = NULL;
	}

	m_pScreenSizeTemporaryTexture[0] = NULL;
	m_pScreenSizeTemporaryTexture[1] = NULL;

	m_SurfaceType = Format;

	HRESULT hr;

	if(s.iAPSurfaceUsage == VIDRNDT_AP_TEXTURE2D || s.iAPSurfaceUsage == VIDRNDT_AP_TEXTURE3D)
	{
		int nTexturesNeeded = s.iAPSurfaceUsage == VIDRNDT_AP_TEXTURE3D ? m_nNbDXSurface+2 : 1;

		for(int i = 0; i < nTexturesNeeded; i++)
		{
			if(FAILED(hr = m_pD3DDev->CreateTexture(
				m_NativeVideoSize.cx, m_NativeVideoSize.cy, 1, 
				D3DUSAGE_RENDERTARGET, Format/*D3DFMT_X8R8G8B8 D3DFMT_A8R8G8B8*/, 
				D3DPOOL_DEFAULT, &m_pVideoTexture[i], NULL)))
				return hr;

			if(FAILED(hr = m_pVideoTexture[i]->GetSurfaceLevel(0, &m_pVideoSurface[i])))
				return hr;
		}

		if(s.iAPSurfaceUsage == VIDRNDT_AP_TEXTURE2D)
		{
			for(int i = 0; i < m_nNbDXSurface+2; i++)
			{
				m_pVideoTexture[i] = NULL;
			}
		}
	}
	else
	{
		if(FAILED(hr = m_pD3DDev->CreateOffscreenPlainSurface(
			m_NativeVideoSize.cx, m_NativeVideoSize.cy, 
			D3DFMT_X8R8G8B8/*D3DFMT_A8R8G8B8*/, 
			D3DPOOL_DEFAULT, &m_pVideoSurface[m_nCurSurface], NULL)))
			return hr;
	}

	hr = m_pD3DDev->ColorFill(m_pVideoSurface[m_nCurSurface], NULL, 0);

	return S_OK;
}

void CDX9AllocatorPresenter::DeleteSurfaces()
{
    CAutoLock cAutoLock(this);
	CAutoLock cRenderLock(&m_RenderLock);

	for(int i = 0; i < m_nNbDXSurface+2; i++)
	{
		m_pVideoTexture[i] = NULL;
		m_pVideoSurface[i] = NULL;
	}
}

UINT CDX9AllocatorPresenter::GetAdapter(IDirect3D9* pD3D)
{
	if(m_hWnd == NULL || pD3D == NULL)
		return D3DADAPTER_DEFAULT;

	HMONITOR hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
	if(hMonitor == NULL) return D3DADAPTER_DEFAULT;

	for(UINT adp = 0, num_adp = pD3D->GetAdapterCount(); adp < num_adp; ++adp)
	{
		HMONITOR hAdpMon = pD3D->GetAdapterMonitor(adp);
		if(hAdpMon == hMonitor) return adp;
	}

	return D3DADAPTER_DEFAULT;
}

// ISubPicAllocatorPresenter

STDMETHODIMP CDX9AllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
	return E_NOTIMPL;
}

static bool ClipToSurface(IDirect3DSurface9* pSurface, CRect& s, CRect& d)   
{   
	D3DSURFACE_DESC d3dsd;   
	ZeroMemory(&d3dsd, sizeof(d3dsd));   
	if(FAILED(pSurface->GetDesc(&d3dsd)))   
		return(false);   

	int w = d3dsd.Width, h = d3dsd.Height;   
	int sw = s.Width(), sh = s.Height();   
	int dw = d.Width(), dh = d.Height();   

	if(d.left >= w || d.right < 0 || d.top >= h || d.bottom < 0   
	|| sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)   
	{   
		s.SetRectEmpty();   
		d.SetRectEmpty();   
		return(true);   
	}   

	if(d.right > w) {s.right -= (d.right-w)*sw/dw; d.right = w;}   
	if(d.bottom > h) {s.bottom -= (d.bottom-h)*sh/dh; d.bottom = h;}   
	if(d.left < 0) {s.left += (0-d.left)*sw/dw; d.left = 0;}   
	if(d.top < 0) {s.top += (0-d.top)*sh/dh; d.top = 0;}   

	return(true);
}

HRESULT CDX9AllocatorPresenter::InitResizers(float bicubicA, bool bNeedScreenSizeTexture)
{
	HRESULT hr;

	do
	{
		if (bicubicA)
		{
			if (!m_pResizerPixelShader[0])
				break;
			if (!m_pResizerPixelShader[1])
				break;
			if (!m_pResizerPixelShader[2])
				break;
			if (m_bicubicA != bicubicA)
				break;
			if (!m_pScreenSizeTemporaryTexture[0])
				break;
			if (bNeedScreenSizeTexture)
			{
				if (!m_pScreenSizeTemporaryTexture[1])
					break;
			}
		}
		else
		{
			if (!m_pResizerPixelShader[0])
				break;
			if (bNeedScreenSizeTexture)
			{
				if (!m_pScreenSizeTemporaryTexture[0])
					break;
				if (!m_pScreenSizeTemporaryTexture[1])
					break;
			}
		}
		return S_OK;
	}
	while (0);

	m_bicubicA = bicubicA;
	m_pScreenSizeTemporaryTexture[0] = NULL;
	m_pScreenSizeTemporaryTexture[1] = NULL;

	for(int i = 0; i < countof(m_pResizerPixelShader); i++)
		m_pResizerPixelShader[i] = NULL;

	if(m_caps.PixelShaderVersion < D3DPS_VERSION(2, 0))
		return E_FAIL;

	LPCSTR pProfile = m_caps.PixelShaderVersion >= D3DPS_VERSION(3, 0) ? "ps_3_0" : "ps_2_0";

	CStringA str;
	if(!LoadResource(IDF_SHADER_RESIZER, str, _T("FILE")))
		return E_FAIL;

	CStringA A;
	A.Format("(%f)", bicubicA);
	str.Replace("_The_Value_Of_A_Is_Set_Here_", A);

	LPCSTR pEntries[] = {"main_bilinear", "main_bicubic1pass", "main_bicubic2pass"};

	ASSERT(countof(pEntries) == countof(m_pResizerPixelShader));

	for(int i = 0; i < countof(pEntries); i++)
	{
		hr = m_pPSC->CompileShader(str, pEntries[i], pProfile, 0, &m_pResizerPixelShader[i]);
		ASSERT (SUCCEEDED (hr));
		if(FAILED(hr)) return hr;
	}

	if(m_bicubicA || bNeedScreenSizeTexture)
	{
		if(FAILED(m_pD3DDev->CreateTexture(
			min(m_ScreenSize.cx, (int)m_caps.MaxTextureWidth), min(max(m_ScreenSize.cy, m_NativeVideoSize.cy), (int)m_caps.MaxTextureHeight), 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, 
			D3DPOOL_DEFAULT, &m_pScreenSizeTemporaryTexture[0], NULL)))
		{
			ASSERT(0);
			m_pScreenSizeTemporaryTexture[0] = NULL; // will do 1 pass then
		}
	}
	if(m_bicubicA || bNeedScreenSizeTexture)
	{
		if(FAILED(m_pD3DDev->CreateTexture(
			min(m_ScreenSize.cx, (int)m_caps.MaxTextureWidth), min(max(m_ScreenSize.cy, m_NativeVideoSize.cy), (int)m_caps.MaxTextureHeight), 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, 
			D3DPOOL_DEFAULT, &m_pScreenSizeTemporaryTexture[1], NULL)))
		{
			ASSERT(0);
			m_pScreenSizeTemporaryTexture[1] = NULL; // will do 1 pass then
		}
	}

	return S_OK;
}

HRESULT CDX9AllocatorPresenter::TextureCopy(CComPtr<IDirect3DTexture9> pTexture)
{
	HRESULT hr;

	D3DSURFACE_DESC desc;
	if(!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc)))
		return E_FAIL;

	float w = (float)desc.Width;
	float h = (float)desc.Height;

	MYD3DVERTEX<1> v[] =
	{
		{0, 0, 0.5f, 2.0f, 0, 0},
		{w, 0, 0.5f, 2.0f, 1, 0},
		{0, h, 0.5f, 2.0f, 0, 1},
		{w, h, 0.5f, 2.0f, 1, 1},
	};

	for(int i = 0; i < countof(v); i++)
	{
		v[i].x -= 0.5;
		v[i].y -= 0.5;
	}

	hr = m_pD3DDev->SetTexture(0, pTexture);

	return TextureBlt(m_pD3DDev, v, D3DTEXF_LINEAR);
}

HRESULT CDX9AllocatorPresenter::DrawRect(DWORD _Color, DWORD _Alpha, const CRect &_Rect)
{
	DWORD Color = D3DCOLOR_ARGB(_Alpha, GetRValue(_Color), GetGValue(_Color), GetBValue(_Color));
	MYD3DVERTEX<0> v[] =
	{
		{float(_Rect.left), float(_Rect.top), 0.5f, 2.0f, Color},
		{float(_Rect.right), float(_Rect.top), 0.5f, 2.0f, Color},
		{float(_Rect.left), float(_Rect.bottom), 0.5f, 2.0f, Color},
		{float(_Rect.right), float(_Rect.bottom), 0.5f, 2.0f, Color},
	};

	for(int i = 0; i < countof(v); i++)
	{
		v[i].x -= 0.5;
		v[i].y -= 0.5;
	}

	return ::DrawRect(m_pD3DDev, v);
}

HRESULT CDX9AllocatorPresenter::TextureResize(CComPtr<IDirect3DTexture9> pTexture, Vector dst[4], D3DTEXTUREFILTERTYPE filter)
{
	HRESULT hr;

	D3DSURFACE_DESC desc;
	if(!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc)))
		return E_FAIL;

	float w = (float)desc.Width;
	float h = (float)desc.Height;

	float dx = 0.98f/w;
	float dy = 0.98f/h;

	MYD3DVERTEX<1> v[] =
	{
		{dst[0].x, dst[0].y, dst[0].z, 1.0f/dst[0].z,  0, 0},
		{dst[1].x, dst[1].y, dst[1].z, 1.0f/dst[1].z,  1, 0},
		{dst[2].x, dst[2].y, dst[2].z, 1.0f/dst[2].z,  0, 1},
		{dst[3].x, dst[3].y, dst[3].z, 1.0f/dst[3].z,  1, 1},
	};

	AdjustQuad(v, dx, dy);

	hr = m_pD3DDev->SetTexture(0, pTexture);

	hr = m_pD3DDev->SetPixelShader(NULL);

	hr = TextureBlt(m_pD3DDev, v, filter);

	return hr;
}

HRESULT CDX9AllocatorPresenter::TextureResizeBilinear(CComPtr<IDirect3DTexture9> pTexture, Vector dst[4])
{
	HRESULT hr;

	D3DSURFACE_DESC desc;
	if(!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc)))
		return E_FAIL;

	float w = (float)desc.Width;
	float h = (float)desc.Height;

	float dx = 0.98f/w;
	float dy = 0.98f/h;

	MYD3DVERTEX<5> v[] =
	{
		{dst[0].x, dst[0].y, dst[0].z, 1.0f/dst[0].z,  0, 0,  0+dx, 0,  0, 0+dy,  0+dx, 0+dy,  0, 0},
		{dst[1].x, dst[1].y, dst[1].z, 1.0f/dst[1].z,  1, 0,  1+dx, 0,  1, 0+dy,  1+dx, 0+dy,  w, 0},
		{dst[2].x, dst[2].y, dst[2].z, 1.0f/dst[2].z,  0, 1,  0+dx, 1,  0, 1+dy,  0+dx, 1+dy,  0, h},
		{dst[3].x, dst[3].y, dst[3].z, 1.0f/dst[3].z,  1, 1,  1+dx, 1,  1, 1+dy,  1+dx, 1+dy,  w, h},
	};

	AdjustQuad(v, dx, dy);

	hr = m_pD3DDev->SetTexture(0, pTexture);
	hr = m_pD3DDev->SetTexture(1, pTexture);
	hr = m_pD3DDev->SetTexture(2, pTexture);
	hr = m_pD3DDev->SetTexture(3, pTexture);

	hr = m_pD3DDev->SetPixelShader(m_pResizerPixelShader[0]);

	hr = TextureBlt(m_pD3DDev, v, D3DTEXF_POINT);

	//

	m_pD3DDev->SetPixelShader(NULL);

	return hr;
}

HRESULT CDX9AllocatorPresenter::TextureResizeBicubic1pass(CComPtr<IDirect3DTexture9> pTexture, Vector dst[4])
{
	HRESULT hr;

	D3DSURFACE_DESC desc;
	if(!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc)))
		return E_FAIL;

	float w = (float)desc.Width;
	float h = (float)desc.Height;

	float dx = 0.98f/w;
	float dy = 0.98f/h;

	MYD3DVERTEX<2> v[] =
	{
		{dst[0].x, dst[0].y, dst[0].z, 1.0f/dst[0].z,  0, 0, 0, 0},
		{dst[1].x, dst[1].y, dst[1].z, 1.0f/dst[1].z,  1, 0, w, 0},
		{dst[2].x, dst[2].y, dst[2].z, 1.0f/dst[2].z,  0, 1, 0, h},
		{dst[3].x, dst[3].y, dst[3].z, 1.0f/dst[3].z,  1, 1, w, h},
	};

	AdjustQuad(v, dx, dy);

	hr = m_pD3DDev->SetTexture(0, pTexture);

	float fConstData[][4] = {{w, h, 0, 0}, {1.0f / w, 1.0f / h, 0, 0}, {1.0f / w, 0, 0, 0}, {0, 1.0f / h, 0, 0}};
	hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, countof(fConstData));

	hr = m_pD3DDev->SetPixelShader(m_pResizerPixelShader[1]);

	hr = TextureBlt(m_pD3DDev, v, D3DTEXF_POINT);

	m_pD3DDev->SetPixelShader(NULL);

	return hr;
}

HRESULT CDX9AllocatorPresenter::TextureResizeBicubic2pass(CComPtr<IDirect3DTexture9> pTexture, Vector dst[4])
{
	// return TextureResizeBicubic1pass(pTexture, dst);

	HRESULT hr;

	// rotated?
	if(dst[0].z != dst[1].z || dst[2].z != dst[3].z || dst[0].z != dst[3].z
	|| dst[0].y != dst[1].y || dst[0].x != dst[2].x || dst[2].y != dst[3].y || dst[1].x != dst[3].x)
		return TextureResizeBicubic1pass(pTexture, dst);

	D3DSURFACE_DESC desc;
	if(!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc)))
		return E_FAIL;

	float dx = 0.98f/desc.Width;

	float w = (float)desc.Width;
	float h = (float)desc.Height;

	CRect dst1(0, 0, (int)(dst[3].x - dst[0].x), (int)h);

	if(!m_pScreenSizeTemporaryTexture[0] || FAILED(m_pScreenSizeTemporaryTexture[0]->GetLevelDesc(0, &desc)))
		return TextureResizeBicubic1pass(pTexture, dst);

	float dy = 0.98f/desc.Height;

	float dw = (float)dst1.Width() / desc.Width;
	float dh = (float)dst1.Height() / desc.Height;

//	ASSERT(dst1.Height() == desc.Height);

	if(dst1.Width() > (int)desc.Width || dst1.Height() > (int)desc.Height)
	// if(dst1.Width() != desc.Width || dst1.Height() != desc.Height)
		return TextureResizeBicubic1pass(pTexture, dst);

	MYD3DVERTEX<5> vx[] =
	{
		{(float)dst1.left, (float)dst1.top, 0.5f, 2.0f, 0-dx, 0,  0, 0,  0+dx, 0,  0+dx*2, 0,  0, 0},
		{(float)dst1.right, (float)dst1.top, 0.5f, 2.0f,  1-dx, 0,  1, 0,  1+dx, 0,  1+dx*2, 0,  w, 0},
		{(float)dst1.left, (float)dst1.bottom, 0.5f, 2.0f,  0-dx, 1,  0, 1,  0+dx, 1,  0+dx*2, 1,  0, 0},
		{(float)dst1.right, (float)dst1.bottom, 0.5f, 2.0f,  1-dx, 1,  1, 1,  1+dx, 1,  1+dx*2, 1,  w, 0},
	};

	AdjustQuad(vx, dx, 0);		// Casimir666 : bug ici, g�n�re des bandes verticales! TODO : pourquoi ??????

	MYD3DVERTEX<5> vy[] =
	{
		{dst[0].x, dst[0].y, dst[0].z, 1.0f/dst[0].z,  0, 0-dy,  0, 0,  0, 0+dy,  0, 0+dy*2,  0, 0},
		{dst[1].x, dst[1].y, dst[1].z, 1.0f/dst[1].z,  dw, 0-dy,  dw, 0,  dw, 0+dy,  dw, 0+dy*2,  0, 0},
		{dst[2].x, dst[2].y, dst[2].z, 1.0f/dst[2].z,  0, dh-dy,  0, dh,  0, dh+dy,  0, dh+dy*2,  h, 0},
		{dst[3].x, dst[3].y, dst[3].z, 1.0f/dst[3].z,  dw, dh-dy,  dw, dh,  dw, dh+dy,  dw, dh+dy*2,  h, 0},
	};

	AdjustQuad(vy, 0, dy);		// Casimir666 : bug ici, g�n�re des bandes horizontales! TODO : pourquoi ??????

	hr = m_pD3DDev->SetPixelShader(m_pResizerPixelShader[2]);

	hr = m_pD3DDev->SetTexture(0, pTexture);
	hr = m_pD3DDev->SetTexture(1, pTexture);
	hr = m_pD3DDev->SetTexture(2, pTexture);
	hr = m_pD3DDev->SetTexture(3, pTexture);

	CComPtr<IDirect3DSurface9> pRTOld;
	hr = m_pD3DDev->GetRenderTarget(0, &pRTOld);

	CComPtr<IDirect3DSurface9> pRT;
	hr = m_pScreenSizeTemporaryTexture[0]->GetSurfaceLevel(0, &pRT);
	hr = m_pD3DDev->SetRenderTarget(0, pRT);

	hr = TextureBlt(m_pD3DDev, vx, D3DTEXF_POINT);

	hr = m_pD3DDev->SetTexture(0, m_pScreenSizeTemporaryTexture[0]);
	hr = m_pD3DDev->SetTexture(1, m_pScreenSizeTemporaryTexture[0]);
	hr = m_pD3DDev->SetTexture(2, m_pScreenSizeTemporaryTexture[0]);
	hr = m_pD3DDev->SetTexture(3, m_pScreenSizeTemporaryTexture[0]);

	hr = m_pD3DDev->SetRenderTarget(0, pRTOld);

	hr = TextureBlt(m_pD3DDev, vy, D3DTEXF_POINT);

	m_pD3DDev->SetPixelShader(NULL);

	return hr;
}

HRESULT CDX9AllocatorPresenter::AlphaBlt(RECT* pSrc, RECT* pDst, CComPtr<IDirect3DTexture9> pTexture)
{
	if(!pSrc || !pDst)
		return E_POINTER;

	CRect src(*pSrc), dst(*pDst);

	HRESULT hr;

    do
	{
		D3DSURFACE_DESC d3dsd;
		ZeroMemory(&d3dsd, sizeof(d3dsd));
		if(FAILED(pTexture->GetLevelDesc(0, &d3dsd)) /*|| d3dsd.Type != D3DRTYPE_TEXTURE*/)
			break;

        float w = (float)d3dsd.Width;
        float h = (float)d3dsd.Height;

		struct
		{
			float x, y, z, rhw;
			float tu, tv;
		}
		pVertices[] =
		{
			{(float)dst.left, (float)dst.top, 0.5f, 2.0f, (float)src.left / w, (float)src.top / h},
			{(float)dst.right, (float)dst.top, 0.5f, 2.0f, (float)src.right / w, (float)src.top / h},
			{(float)dst.left, (float)dst.bottom, 0.5f, 2.0f, (float)src.left / w, (float)src.bottom / h},
			{(float)dst.right, (float)dst.bottom, 0.5f, 2.0f, (float)src.right / w, (float)src.bottom / h},
		};
/*
		for(int i = 0; i < countof(pVertices); i++)
		{
			pVertices[i].x -= 0.5;
			pVertices[i].y -= 0.5;
		}
*/

        hr = m_pD3DDev->SetTexture(0, pTexture);

		DWORD abe, sb, db;
		hr = m_pD3DDev->GetRenderState(D3DRS_ALPHABLENDENABLE, &abe);
		hr = m_pD3DDev->GetRenderState(D3DRS_SRCBLEND, &sb);
		hr = m_pD3DDev->GetRenderState(D3DRS_DESTBLEND, &db);

        hr = m_pD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        hr = m_pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
		hr = m_pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
    	hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        hr = m_pD3DDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE); // pre-multiplied src and ...
        hr = m_pD3DDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCALPHA); // ... inverse alpha channel for dst

		hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

        hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

		hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

		/*//

		D3DCAPS9 d3dcaps9;
		hr = m_pD3DDev->GetDeviceCaps(&d3dcaps9);
		if(d3dcaps9.AlphaCmpCaps & D3DPCMPCAPS_LESS)
		{
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, (DWORD)0x000000FE);
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE); 
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, D3DPCMPCAPS_LESS);
		}

		*///

        hr = m_pD3DDev->SetPixelShader(NULL);

        hr = m_pD3DDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
		hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));

        //

		m_pD3DDev->SetTexture(0, NULL);

    	m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, abe);
        m_pD3DDev->SetRenderState(D3DRS_SRCBLEND, sb);
        m_pD3DDev->SetRenderState(D3DRS_DESTBLEND, db);

		return S_OK;
    }
	while(0);

    return E_FAIL;
}

void CDX9AllocatorPresenter::CalculateJitter(LONGLONG PerfCounter)
{
	// Calculate the jitter!
	LONGLONG	llPerf = PerfCounter;
	if ((m_rtTimePerFrame != 0) && (labs ((long)(llPerf - m_llLastPerf)) < m_rtTimePerFrame*3) )
	{
		m_nNextJitter = (m_nNextJitter+1) % NB_JITTER;
		m_pllJitter[m_nNextJitter] = llPerf - m_llLastPerf;

		m_MaxJitter = MINLONG64;
		m_MinJitter = MAXLONG64;

		// Calculate the real FPS
		LONGLONG		llJitterSum = 0;
		LONGLONG		llJitterSumAvg = 0;
		for (int i=0; i<NB_JITTER; i++)
		{
			LONGLONG Jitter = m_pllJitter[i];
			llJitterSum += Jitter;
			llJitterSumAvg += Jitter;
		}
		double FrameTimeMean = double(llJitterSumAvg)/NB_JITTER;
		m_fJitterMean = FrameTimeMean;
		double DeviationSum = 0;
		for (int i=0; i<NB_JITTER; i++)
		{
			LONGLONG DevInt = m_pllJitter[i] - FrameTimeMean;
			double Deviation = DevInt;
			DeviationSum += Deviation*Deviation;
			m_MaxJitter = max(m_MaxJitter, DevInt);
			m_MinJitter = min(m_MinJitter, DevInt);
		}
		double StdDev = sqrt(DeviationSum/NB_JITTER);

		m_fJitterStdDev = StdDev;

		m_fAvrFps = 10000000.0/(double(llJitterSum)/NB_JITTER);
	}

	m_llLastPerf = llPerf;
}

bool CDX9AllocatorPresenter::GetVBlank(int &_ScanLine, int &_bInVBlank, bool _bMeasureTime)
{
	LONGLONG llPerf;
	if (_bMeasureTime)
		llPerf = AfxGetMyApp()->GetPerfCounter();

	int ScanLine = 0;
	_ScanLine = 0;
	_bInVBlank = 0;
	if (m_pDirectDraw)
	{
		DWORD ScanLineGet = 0;
		m_pDirectDraw->GetScanLine(&ScanLineGet);
		BOOL InVBlank;
		if (m_pDirectDraw->GetVerticalBlankStatus (&InVBlank) != S_OK)
			return false;
		ScanLine = ScanLineGet;
		_bInVBlank = InVBlank;
		if (InVBlank)
			ScanLine = 0;
	}
	else
	{
		D3DRASTER_STATUS RasterStatus;
		if (m_pD3DDev->GetRasterStatus(0, &RasterStatus) != S_OK)
			return false;;
		ScanLine = RasterStatus.ScanLine;
		_bInVBlank = RasterStatus.InVBlank;
	}
	if (_bMeasureTime)
	{
		m_VBlankMax = max(m_VBlankMax, ScanLine);
		if (ScanLine != 0 && !_bInVBlank)
			m_VBlankMinCalc = min(m_VBlankMinCalc, ScanLine);
		m_VBlankMin = m_VBlankMax - m_ScreenSize.cy;
	}
	if (_bInVBlank)
		_ScanLine = 0;
	else if (m_VBlankMin != 300000)
		_ScanLine = ScanLine - m_VBlankMin;
	else
		_ScanLine = ScanLine;

	if (_bMeasureTime)
	{
		LONGLONG Time = AfxGetMyApp()->GetPerfCounter() - llPerf;
		m_RasterStatusWaitTimeMaxCalc = max(m_RasterStatusWaitTimeMaxCalc, Time);
	}

	return true;
}

bool CDX9AllocatorPresenter::WaitForVBlankRange(int &_RasterStart, int _RasterSize, bool _bWaitIfInside, bool _bNeedAccurate, bool _bMeasure)
{
	if (_bMeasure)
		m_RasterStatusWaitTimeMaxCalc = 0;
	bool bWaited = false;
	int ScanLine = 0;
	int InVBlank = 0;
	LONGLONG llPerf;
	if (_bMeasure)
		llPerf = AfxGetMyApp()->GetPerfCounter();
	GetVBlank(ScanLine, InVBlank, _bMeasure);
	if (_bMeasure)
		m_VBlankStartWait = ScanLine;

	static bool bOneWait = true;
	if (bOneWait && _bMeasure)
	{
		bOneWait = false;
		// If we are already in the wanted interval we need to wait until we aren't, this improves sync when for example you are playing 23.976 Hz material on a 24 Hz refresh rate
		int nInVBlank = 0;
		while (1)
		{
			if (!GetVBlank(ScanLine, InVBlank, _bMeasure))
				break;

			if (InVBlank && nInVBlank == 0)
			{
				nInVBlank = 1;
			}
			else if (!InVBlank && nInVBlank == 1)
			{
				nInVBlank = 2;
			}
			else if (InVBlank && nInVBlank == 2)
			{
				nInVBlank = 3;
			}
			else if (!InVBlank && nInVBlank == 3)
			{
				break;
			}
		}
	}
	if (_bWaitIfInside)
	{
		int ScanLineDiff = long(ScanLine) - _RasterStart;
		if (ScanLineDiff > m_ScreenSize.cy / 2)
			ScanLineDiff -= m_ScreenSize.cy;
		else if (ScanLineDiff < -m_ScreenSize.cy / 2)
			ScanLineDiff += m_ScreenSize.cy;

		if (ScanLineDiff >= 0 && ScanLineDiff <= _RasterSize)
		{
			bWaited = true;
			// If we are already in the wanted interval we need to wait until we aren't, this improves sync when for example you are playing 23.976 Hz material on a 24 Hz refresh rate
			int LastLineDiff = ScanLineDiff;
			while (1)
			{
				if (!GetVBlank(ScanLine, InVBlank, _bMeasure))
					break;
				int ScanLineDiff = long(ScanLine) - _RasterStart;
				if (ScanLineDiff > m_ScreenSize.cy / 2)
					ScanLineDiff -= m_ScreenSize.cy;
				else if (ScanLineDiff < -m_ScreenSize.cy / 2)
					ScanLineDiff += m_ScreenSize.cy;
				if (!(ScanLineDiff >= 0 && ScanLineDiff <= _RasterSize) || (LastLineDiff < 0 && ScanLineDiff > 0))
					break;
				LastLineDiff = ScanLineDiff;
				Sleep(1); // Just sleep
			}
		}
	}
	int MinRange = max(min(int(0.0015 * double(m_ScreenSize.cy) * double(m_RefreshRate) + 0.5), m_ScreenSize.cy/3), 5); // 1.5 ms or max 33 % of Time
	int NoSleepStart = _RasterStart - MinRange;
	int NoSleepRange = MinRange;
	if (NoSleepStart < 0)
		NoSleepStart += m_ScreenSize.cy;

	int ScanLineDiff = ScanLine - _RasterStart;
	if (ScanLineDiff > m_ScreenSize.cy / 2)
		ScanLineDiff -= m_ScreenSize.cy;
	else if (ScanLineDiff < -m_ScreenSize.cy / 2)
		ScanLineDiff += m_ScreenSize.cy;
	int LastLineDiff = ScanLineDiff;
	int LastLineDiffSleep = long(ScanLine) - NoSleepStart;
	while (1)
	{
		if (!GetVBlank(ScanLine, InVBlank, _bMeasure))
			break;
		int ScanLineDiff = long(ScanLine) - _RasterStart;
		if (ScanLineDiff > m_ScreenSize.cy / 2)
			ScanLineDiff -= m_ScreenSize.cy;
		else if (ScanLineDiff < -m_ScreenSize.cy / 2)
			ScanLineDiff += m_ScreenSize.cy;
		if ((ScanLineDiff >= 0 && ScanLineDiff <= _RasterSize) || (LastLineDiff < 0 && ScanLineDiff > 0))
			break;

		LastLineDiff = ScanLineDiff;

		bWaited = true;

		int ScanLineDiffSleep = long(ScanLine) - NoSleepStart;
		if (ScanLineDiffSleep > m_ScreenSize.cy / 2)
			ScanLineDiffSleep -= m_ScreenSize.cy;
		else if (ScanLineDiffSleep < -m_ScreenSize.cy / 2)
			ScanLineDiffSleep += m_ScreenSize.cy;

		if (!((ScanLineDiffSleep >= 0 && ScanLineDiffSleep <= NoSleepRange) || (LastLineDiffSleep < 0 && ScanLineDiffSleep > 0)) || !_bNeedAccurate)
		{
			//TRACE("%d\n", RasterStatus.ScanLine);
			Sleep(1); // Don't sleep for the last 1.5 ms scan lines, so we get maximum precision
		}
		LastLineDiffSleep = ScanLineDiffSleep;
	}
	_RasterStart = ScanLine;
	if (_bMeasure)
	{
		m_VBlankEndWait = ScanLine;
		m_VBlankWaitTime = AfxGetMyApp()->GetPerfCounter() - llPerf;

		m_RasterStatusWaitTime = m_RasterStatusWaitTimeMaxCalc;
		m_RasterStatusWaitTimeMin = min(m_RasterStatusWaitTimeMin, m_RasterStatusWaitTime);
		m_RasterStatusWaitTimeMax = max(m_RasterStatusWaitTimeMax, m_RasterStatusWaitTime);
	}

	return bWaited;
}

int CDX9AllocatorPresenter::GetVBlackPos()
{
	AppSettings& s = AfxGetAppSettings();
	BOOL bCompositionEnabled = m_bCompositionEnabled;

	int WaitRange = max(m_ScreenSize.cy / 40, 5);
	if (!bCompositionEnabled)
	{
		if (m_bAlternativeVSync)
		{
			return s.m_RenderSettings.iVMR9VSyncOffset;
		}
		else
		{
			int MinRange = max(min(int(0.005 * double(m_ScreenSize.cy) * GetRefreshRate() + 0.5), m_ScreenSize.cy/3), 5); // 5  ms or max 33 % of Time
			int WaitFor = m_ScreenSize.cy - (MinRange + WaitRange);
			return WaitFor;
		}
	}
	else
	{
		int WaitFor = m_ScreenSize.cy / 2;
		return WaitFor;
	}
}


bool CDX9AllocatorPresenter::WaitForVBlank(bool &_Waited)
{
	AppSettings& s = AfxGetAppSettings();
	if (!s.m_RenderSettings.iVMR9VSync)
	{
		_Waited = true;
		m_VBlankWaitTime = 0;
		m_VBlankEndWait = 0;
		m_VBlankStartWait = 0;
		return true;
	}
//	_Waited = true;
//	return false;

	BOOL bCompositionEnabled = m_bCompositionEnabled;
	int WaitFor = GetVBlackPos();

	if (!bCompositionEnabled)
	{
		if (m_bAlternativeVSync)
		{
			_Waited = WaitForVBlankRange(WaitFor, 0, false, true, true);
			return false;
		}
		else
		{
			_Waited = WaitForVBlankRange(WaitFor, 0, false, s.m_RenderSettings.iVMR9VSyncAccurate, true);
			return true;
		}
	}
	else
	{
		// Instead we wait for VBlack after the present, this seems to fix the stuttering problem. It's also possible to fix by removing the Sleep above, but that isn't an option.
		WaitForVBlankRange(WaitFor, 0, false, s.m_RenderSettings.iVMR9VSyncAccurate, true);

		return false;
	}
}

void CDX9AllocatorPresenter::UpdateAlphaBitmap()
{
	m_VMR9AlphaBitmapData.Free();

	if ((m_VMR9AlphaBitmap.dwFlags & VMRBITMAP_DISABLE) == 0)
	{
		HBITMAP			hBitmap = (HBITMAP)GetCurrentObject (m_VMR9AlphaBitmap.hdc, OBJ_BITMAP);
		if (!hBitmap)
			return;
		DIBSECTION		info = {0};
		if (!::GetObject(hBitmap, sizeof( DIBSECTION ), &info ))
			return;

		m_VMR9AlphaBitmapRect = CRect(0, 0, info.dsBm.bmWidth, info.dsBm.bmHeight);
		m_VMR9AlphaBitmapWidthBytes = info.dsBm.bmWidthBytes;

		if (m_VMR9AlphaBitmapData.Allocate(info.dsBm.bmWidthBytes * info.dsBm.bmHeight))
		{
			memcpy((BYTE *)m_VMR9AlphaBitmapData, info.dsBm.bmBits, info.dsBm.bmWidthBytes * info.dsBm.bmHeight);
		}
	}
}

STDMETHODIMP_(bool) CDX9AllocatorPresenter::Paint(bool fAll)
{
//	if (!fAll)
//		return false;
	AppSettings& s = AfxGetAppSettings();

//	TRACE("Thread: %d\n", (LONG)((CRITICAL_SECTION &)m_RenderLock).OwningThread);

#if 0
	if (TryEnterCriticalSection (&(CRITICAL_SECTION &)(*((CCritSec *)this))))
	{
		LeaveCriticalSection((&(CRITICAL_SECTION &)(*((CCritSec *)this))));
	}
	else
	{
		__asm {
			int 3
		};
	}
#endif

	CMPlayerCApp * pApp = AfxGetMyApp();

	LONGLONG StartPaint = pApp->GetPerfCounter();
	CAutoLock cRenderLock(&m_RenderLock);

	if(m_WindowRect.right <= m_WindowRect.left || m_WindowRect.bottom <= m_WindowRect.top
	|| m_NativeVideoSize.cx <= 0 || m_NativeVideoSize.cy <= 0
	|| !m_pVideoSurface)
	{
		if (m_OrderedPaint)
			--m_OrderedPaint;
		else
		{
			TRACE("UNORDERED PAINT!!!!!!\n");
		}


		return(false);
	}

	HRESULT hr;

	CRect rSrcVid(CPoint(0, 0), m_NativeVideoSize);
	CRect rDstVid(m_VideoRect);

	CRect rSrcPri(CPoint(0, 0), m_WindowRect.Size());
	CRect rDstPri(m_WindowRect);

	m_pD3DDev->BeginScene();

	CComPtr<IDirect3DSurface9> pBackBuffer;
	m_pD3DDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);

	m_pD3DDev->SetRenderTarget(0, pBackBuffer);

//	if(fAll)
	{
		// clear the backbuffer

		hr = m_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

		// paint the video on the backbuffer

		if(!rDstVid.IsRectEmpty())
		{
			if(m_pVideoTexture[m_nCurSurface])
			{
				CComPtr<IDirect3DTexture9> pVideoTexture = m_pVideoTexture[m_nCurSurface];

				if(m_pVideoTexture[m_nNbDXSurface] && m_pVideoTexture[m_nNbDXSurface+1] && !m_pPixelShaders.IsEmpty())
				{
					static __int64 counter = 0;
					static long start = clock();

					long stop = clock();
					long diff = stop - start;

					if(diff >= 10*60*CLOCKS_PER_SEC) start = stop; // reset after 10 min (ps float has its limits in both range and accuracy)

					float fConstData[][4] = 
					{
						{(float)m_NativeVideoSize.cx, (float)m_NativeVideoSize.cy, (float)(counter++), (float)diff / CLOCKS_PER_SEC},
						{1.0f / m_NativeVideoSize.cx, 1.0f / m_NativeVideoSize.cy, 0, 0},
					};

					hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, countof(fConstData));

					CComPtr<IDirect3DSurface9> pRT;
					hr = m_pD3DDev->GetRenderTarget(0, &pRT);

					int src = m_nCurSurface, dst = m_nNbDXSurface;

					POSITION pos = m_pPixelShaders.GetHeadPosition();
					while(pos)
					{
						pVideoTexture = m_pVideoTexture[dst];

						hr = m_pD3DDev->SetRenderTarget(0, m_pVideoSurface[dst]);
						CExternalPixelShader &Shader = m_pPixelShaders.GetNext(pos);
						if (!Shader.m_pPixelShader)
							Shader.Compile(m_pPSC);
						hr = m_pD3DDev->SetPixelShader(Shader.m_pPixelShader);
						TextureCopy(m_pVideoTexture[src]);

						//if(++src > 2) src = 1;
						//if(++dst > 2) dst = 1;
						src		= dst;
						if(++dst >= m_nNbDXSurface+2) dst = m_nNbDXSurface;
					}

					hr = m_pD3DDev->SetRenderTarget(0, pRT);
					hr = m_pD3DDev->SetPixelShader(NULL);
				}

				Vector dst[4];
				Transform(rDstVid, dst);

				DWORD iDX9Resizer = s.iDX9Resizer;

				float A = 0;

				switch(iDX9Resizer)
				{
					case 3: A = -0.60f; break;
					case 4: A = -0.751f; break;	// FIXME : 0.75 crash recent D3D, or eat CPU 
					case 5: A = -1.00f; break;
				}
				bool bScreenSpacePixelShaders = !m_pPixelShadersScreenSpace.IsEmpty();

				hr = InitResizers(A, bScreenSpacePixelShaders);

				if (!m_pScreenSizeTemporaryTexture[0] || !m_pScreenSizeTemporaryTexture[1])
					bScreenSpacePixelShaders = false;

				if (bScreenSpacePixelShaders)
				{
					CComPtr<IDirect3DSurface9> pRT;
					hr = m_pScreenSizeTemporaryTexture[1]->GetSurfaceLevel(0, &pRT);
					if (hr != S_OK)
						bScreenSpacePixelShaders = false;
					if (bScreenSpacePixelShaders)
					{
						hr = m_pD3DDev->SetRenderTarget(0, pRT);
						if (hr != S_OK)
							bScreenSpacePixelShaders = false;
						hr = m_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
					}
				}

				if(iDX9Resizer == 0 || iDX9Resizer == 1 || rSrcVid.Size() == rDstVid.Size() || FAILED(hr))
				{
					hr = TextureResize(pVideoTexture, dst, iDX9Resizer == 0 ? D3DTEXF_POINT : D3DTEXF_LINEAR);
				}
				else if(iDX9Resizer == 2)
				{
					hr = TextureResizeBilinear(pVideoTexture, dst);
				}
				else if(iDX9Resizer >= 3)
				{
					hr = TextureResizeBicubic2pass(pVideoTexture, dst);
				}

				if (bScreenSpacePixelShaders)
				{
					static __int64 counter = 555;
					static long start = clock() + 333;

					long stop = clock() + 333;
					long diff = stop - start;

					if(diff >= 10*60*CLOCKS_PER_SEC) start = stop; // reset after 10 min (ps float has its limits in both range and accuracy)

					D3DSURFACE_DESC desc;
					m_pScreenSizeTemporaryTexture[0]->GetLevelDesc(0, &desc);

#if 1
					float fConstData[][4] = 
					{
						{(float)desc.Width, (float)desc.Height, (float)(counter++), (float)diff / CLOCKS_PER_SEC},
						{1.0f / desc.Width, 1.0f / desc.Height, 0, 0},
					};
#else
					float fConstData[][4] = 
					{
						{(float)m_ScreenSize.cx, (float)m_ScreenSize.cy, (float)(counter++), (float)diff / CLOCKS_PER_SEC},
						{1.0f / m_ScreenSize.cx, 1.0f / m_ScreenSize.cy, 0, 0},
					};
#endif

					hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, countof(fConstData));

					int src = 1, dst = 0;

					POSITION pos = m_pPixelShadersScreenSpace.GetHeadPosition();
					while(pos)
					{
						if (m_pPixelShadersScreenSpace.GetTailPosition() == pos)
						{
							m_pD3DDev->SetRenderTarget(0, pBackBuffer);
						}
						else
						{
							CComPtr<IDirect3DSurface9> pRT;
							hr = m_pScreenSizeTemporaryTexture[dst]->GetSurfaceLevel(0, &pRT);
							m_pD3DDev->SetRenderTarget(0, pRT);
						}

						CExternalPixelShader &Shader = m_pPixelShadersScreenSpace.GetNext(pos);
						if (!Shader.m_pPixelShader)
							Shader.Compile(m_pPSC);
						hr = m_pD3DDev->SetPixelShader(Shader.m_pPixelShader);
						TextureCopy(m_pScreenSizeTemporaryTexture[src]);

						swap(src, dst);
					}

					hr = m_pD3DDev->SetPixelShader(NULL);
				}
			}
			else
			{
				if(pBackBuffer)
				{
					ClipToSurface(pBackBuffer, rSrcVid, rDstVid); // grrr
					// IMPORTANT: rSrcVid has to be aligned on mod2 for yuy2->rgb conversion with StretchRect!!!
					rSrcVid.left &= ~1; rSrcVid.right &= ~1;
					rSrcVid.top &= ~1; rSrcVid.bottom &= ~1;
					hr = m_pD3DDev->StretchRect(m_pVideoSurface[m_nCurSurface], rSrcVid, pBackBuffer, rDstVid, m_filter);

					// Support ffdshow queueing
					// m_pD3DDev->StretchRect may fail if ffdshow is using queue output samples.
					// Here we don't want to show the black buffer.
					if(FAILED(hr)) 
					{
						if (m_OrderedPaint)
							--m_OrderedPaint;
						else
						{
							TRACE("UNORDERED PAINT!!!!!!\n");
						}

						return false;
					}
				}
			}
		}

		// paint the text on the backbuffer

		AlphaBltSubPic(rSrcPri.Size());
	}


	// Casimir666 : affichage de l'OSD
	if (m_VMR9AlphaBitmap.dwFlags & VMRBITMAP_UPDATE)
	{
		CAutoLock BitMapLock(&m_VMR9AlphaBitmapLock);
		CRect		rcSrc (m_VMR9AlphaBitmap.rSrc);
		m_pOSDTexture	= NULL;
		m_pOSDSurface	= NULL;
		if ((m_VMR9AlphaBitmap.dwFlags & VMRBITMAP_DISABLE) == 0 && (BYTE *)m_VMR9AlphaBitmapData)
		{
			if( (m_pD3DXLoadSurfaceFromMemory != NULL) &&
				SUCCEEDED(hr = m_pD3DDev->CreateTexture(rcSrc.Width(), rcSrc.Height(), 1, 
												D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, 
												D3DPOOL_DEFAULT, &m_pOSDTexture, NULL)) )
			{
				if (SUCCEEDED (hr = m_pOSDTexture->GetSurfaceLevel(0, &m_pOSDSurface)))
				{
					hr = m_pD3DXLoadSurfaceFromMemory (m_pOSDSurface,
												NULL,
												NULL,
												(BYTE *)m_VMR9AlphaBitmapData,
												D3DFMT_A8R8G8B8,
												m_VMR9AlphaBitmapWidthBytes,
												NULL,
												&m_VMR9AlphaBitmapRect,
												D3DX_FILTER_NONE,
												m_VMR9AlphaBitmap.clrSrcKey);
				}
				if (FAILED (hr))
				{
					m_pOSDTexture	= NULL;
					m_pOSDSurface	= NULL;
				}
			}
		}
		m_VMR9AlphaBitmap.dwFlags ^= VMRBITMAP_UPDATE;

	}

	if (pApp->m_fDisplayStats)
		DrawStats();
	if (m_pOSDTexture) AlphaBlt(rSrcPri, rDstPri, m_pOSDTexture);

	m_pD3DDev->EndScene();

	BOOL bCompositionEnabled = m_bCompositionEnabled;

	bool bDoVSyncInPresent = (!bCompositionEnabled && !m_bAlternativeVSync) || !s.m_RenderSettings.iVMR9VSync;

	LONGLONG PresentWaitTime = 0;
/*	if(fAll && m_fVMRSyncFix && bDoVSyncInPresent)
	{
		LONGLONG llPerf = pApp->GetPerfCounter();
		D3DLOCKED_RECT lr;
		if(SUCCEEDED(pBackBuffer->LockRect(&lr, NULL, 0)))
			pBackBuffer->UnlockRect();
		PresentWaitTime = pApp->GetPerfCounter() - llPerf;
	}*/

	CComPtr<IDirect3DQuery9> pEventQuery;

	m_pD3DDev->CreateQuery(D3DQUERYTYPE_EVENT, &pEventQuery);
	if (pEventQuery)
		pEventQuery->Issue(D3DISSUE_END);

	if (s.m_RenderSettings.iVMRFlushGPUBeforeVSync && pEventQuery)
	{
		LONGLONG llPerf = pApp->GetPerfCounter();
		BOOL Data;
		//Sleep(5);
		while(S_FALSE == pEventQuery->GetData( &Data, sizeof(Data), D3DGETDATA_FLUSH ))
		{
			if (!s.m_RenderSettings.iVMRFlushGPUWait)
				break;
			Sleep(1);
		}
		if (s.m_RenderSettings.iVMRFlushGPUWait)
			m_WaitForGPUTime = pApp->GetPerfCounter() - llPerf;
		else
			m_WaitForGPUTime = 0;
	}
	else
		m_WaitForGPUTime = 0;
	if (fAll)
	{
		m_PaintTime = (AfxGetMyApp()->GetPerfCounter() - StartPaint);
		m_PaintTimeMin = min(m_PaintTimeMin, m_PaintTime);
		m_PaintTimeMax = max(m_PaintTimeMax, m_PaintTime);
		
	}

	bool bWaited = false;
	if (fAll)
	{
		// Only sync to refresh when redrawing all
		bool bTest = WaitForVBlank(bWaited);
		ASSERT(bTest == bDoVSyncInPresent);
		if (!bDoVSyncInPresent)
		{
			LONGLONG Time = pApp->GetPerfCounter();
			OnVBlankFinished(fAll, Time);
			if (!m_bIsEVR || m_OrderedPaint)
				CalculateJitter(Time);
		}
	}


	// Create a device pointer m_pd3dDevice

	// Create a query object


	{
		CComPtr<IDirect3DQuery9> pEventQuery;
		m_pD3DDev->CreateQuery(D3DQUERYTYPE_EVENT, &pEventQuery);

		LONGLONG llPerf = pApp->GetPerfCounter();
		if (m_pD3DDevEx)
		{
			if (m_bIsFullscreen)
				hr = m_pD3DDevEx->PresentEx(NULL, NULL, NULL, NULL, NULL);
			else
				hr = m_pD3DDevEx->PresentEx(rSrcPri, rDstPri, NULL, NULL, NULL);
		}
		else
		{
			if (m_bIsFullscreen)
				hr = m_pD3DDev->Present(NULL, NULL, NULL, NULL);
			else
				hr = m_pD3DDev->Present(rSrcPri, rDstPri, NULL, NULL);
		}
		// Issue an End event
		if (pEventQuery)
			pEventQuery->Issue(D3DISSUE_END);

		BOOL Data;

		if (s.m_RenderSettings.iVMRFlushGPUAfterPresent && pEventQuery)
		{
			LONGLONG FlushStartTime = pApp->GetPerfCounter();
			while (S_FALSE == pEventQuery->GetData( &Data, sizeof(Data), D3DGETDATA_FLUSH ))
			{
				if (!s.m_RenderSettings.iVMRFlushGPUWait)
					break;
				if (pApp->GetPerfCounter() - FlushStartTime > 10000)
					break; // timeout after 10 ms
			}
		}

		int ScanLine;
		int bInVBlank;
		GetVBlank(ScanLine, bInVBlank, false);

		if (fAll && (!m_bIsEVR || m_OrderedPaint))
		{
			m_VBlankEndPresent = ScanLine;
		}

		while (ScanLine == 0 || bInVBlank)
		{
			GetVBlank(ScanLine, bInVBlank, false);

		}
		m_VBlankStartMeasureTime = pApp->GetPerfCounter();
		m_VBlankStartMeasure = ScanLine;

		if (fAll && bDoVSyncInPresent)
		{
			m_PresentWaitTime = (pApp->GetPerfCounter() - llPerf) + PresentWaitTime;
			m_PresentWaitTimeMin = min(m_PresentWaitTimeMin, m_PresentWaitTime);
			m_PresentWaitTimeMax = max(m_PresentWaitTimeMax, m_PresentWaitTime);
		}
		else
		{
			m_PresentWaitTime = 0;
			m_PresentWaitTimeMin = min(m_PresentWaitTimeMin, m_PresentWaitTime);
			m_PresentWaitTimeMax = max(m_PresentWaitTimeMax, m_PresentWaitTime);
		}
	}

	if (bDoVSyncInPresent)
	{
		LONGLONG Time = pApp->GetPerfCounter();
		if (!m_bIsEVR || m_OrderedPaint)
			CalculateJitter(Time);
		OnVBlankFinished(fAll, Time);
	}
	
/*	if (!bWaited)
	{
		bWaited = true;
		WaitForVBlank(bWaited);
		TRACE("Double VBlank\n");
		ASSERT(bWaited);
		if (!bDoVSyncInPresent)
		{
			CalculateJitter();
			OnVBlankFinished(fAll);
		}
	}*/
	bool fResetDevice = false;

	if(hr == D3DERR_DEVICELOST && m_pD3DDev->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
	{
		fResetDevice = true;
	}

	if (SettingsNeedResetDevice())
		fResetDevice = true;

	bCompositionEnabled = false;
	if (m_pDwmIsCompositionEnabled)
		m_pDwmIsCompositionEnabled(&bCompositionEnabled);
	if ((bCompositionEnabled != 0) != m_bCompositionEnabled)
	{
		if (m_bIsFullscreen)
		{
			m_bCompositionEnabled = (bCompositionEnabled != 0);
		}
		else
			fResetDevice = true;
	}


	D3DDEVICE_CREATION_PARAMETERS Parameters;
	if(SUCCEEDED(m_pD3DDev->GetCreationParameters(&Parameters)) && m_pD3D->GetAdapterMonitor(Parameters.AdapterOrdinal) != m_pD3D->GetAdapterMonitor(GetAdapter(m_pD3D)))
	{
		fResetDevice = true;
	}

	if(fResetDevice)
	{
		if (m_bNeedPendingResetDevice)
		{
			m_bPendingResetDevice = true;
		}
		else
		{
			ResetDevice();
		}
	}

	if (m_OrderedPaint)
		--m_OrderedPaint;
	else
	{
		TRACE("UNORDERED PAINT!!!!!!\n");
	}
	return(true);
}

double CDX9AllocatorPresenter::GetFrameTime()
{
	if (m_DetectedLock)
		return m_DetectedFrameTime;

	return m_rtTimePerFrame / 10000000.0;
}

double CDX9AllocatorPresenter::GetFrameRate()
{
	if (m_DetectedLock)
		return m_DetectedFrameRate;

	return 10000000.0 / m_rtTimePerFrame;
}

bool CDX9AllocatorPresenter::ResetDevice()
{
	StopWorkerThreads();
	DeleteSurfaces();
	HRESULT hr;
	CString Error;
	// TODO: Report error messages here
	if(FAILED(hr = CreateDevice(Error)) || FAILED(hr = AllocSurfaces()))
	{
		return false;
	}
	OnResetDevice();
	return true;
}

void CDX9AllocatorPresenter::DrawText(const RECT &rc, const CString &strText, int _Priority)
{
	if (_Priority < 1)
		return;
	int Quality = 1;
	D3DXCOLOR Color1( 1.0f, 0.2f, 0.2f, 1.0f );
	D3DXCOLOR Color0( 0.0f, 0.0f, 0.0f, 1.0f );
	RECT Rect1 = rc;
	RECT Rect2 = rc;
	if (Quality == 1)
		OffsetRect (&Rect2 , 2, 2);
	else
		OffsetRect (&Rect2 , -1, -1);
	if (Quality > 0)
		m_pFont->DrawText( m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
	OffsetRect (&Rect2 , 1, 0);
	if (Quality > 3)
		m_pFont->DrawText( m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
	OffsetRect (&Rect2 , 1, 0);
	if (Quality > 2)
		m_pFont->DrawText( m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
	OffsetRect (&Rect2 , 0, 1);
	if (Quality > 3)
		m_pFont->DrawText( m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
	OffsetRect (&Rect2 , 0, 1);
	if (Quality > 1)
		m_pFont->DrawText( m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
	OffsetRect (&Rect2 , -1, 0);
	if (Quality > 3)
		m_pFont->DrawText( m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
	OffsetRect (&Rect2 , -1, 0);
	if (Quality > 2)
		m_pFont->DrawText( m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
	OffsetRect (&Rect2 , 0, -1);
	if (Quality > 3)
		m_pFont->DrawText( m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
	m_pFont->DrawText( m_pSprite, strText, -1, &Rect1, DT_NOCLIP, Color1);
}


void CDX9AllocatorPresenter::DrawStats()
{
	AppSettings& s = AfxGetAppSettings();
	CMPlayerCApp * pApp = AfxGetMyApp();
	int bDetailedStats = 2;
	switch (pApp->m_fDisplayStats)
	{
	case 1: bDetailedStats = 2; break;
	case 2: bDetailedStats = 1; break;
	case 3: bDetailedStats = 0; break;
	}	

	LONGLONG		llMaxJitter = m_MaxJitter;
	LONGLONG		llMinJitter = m_MinJitter;
	LONGLONG		llMaxSyncOffset = m_MaxSyncOffset;
	LONGLONG		llMinSyncOffset = m_MinSyncOffset;
	if (m_pFont && m_pSprite)
	{
		m_pSprite->Begin(D3DXSPRITE_ALPHABLEND);
		RECT			rc = {700, 40, 0, 0 };
		rc.left = 40;
		CString		strText;
		int TextHeight = 25.0*m_TextScale + 0.5;
//		strText.Format(L"Frame rate   : %7.03f   (%7.3f ms = %.03f, %s)   (%7.3f ms = %.03f%s)    Clock: %7.3f ms %+1.4f %%  %+1.9f  %+1.9f", m_fAvrFps, double(m_rtTimePerFrame) / 10000.0, 10000000.0 / (double)(m_rtTimePerFrame), m_bInterlaced ? L"I" : L"P", GetFrameTime() * 1000.0, GetFrameRate(), m_DetectedLock ? L" L" : L"", m_ClockDiff/10000.0, m_ModeratedTimeSpeed*100.0 - 100.0, m_ModeratedTimeSpeedDiff, m_ClockDiffCalc/10000.0);
		if (bDetailedStats > 1)
		{
			if (m_bIsEVR)
				strText.Format(L"Frame rate   : %7.03f   (%7.3f ms = %.03f, %s)   (%7.3f ms = %.03f%s, %2.03f StdDev)  Clock: %1.4f %%", m_fAvrFps, double(m_rtTimePerFrame) / 10000.0, 10000000.0 / (double)(m_rtTimePerFrame), m_bInterlaced ? L"I" : L"P", GetFrameTime() * 1000.0, GetFrameRate(), m_DetectedLock ? L" L" : L"", m_DetectedFrameTimeStdDev / 10000.0, m_ModeratedTimeSpeed*100.0);
			else
				strText.Format(L"Frame rate   : %7.03f   (%7.3f ms = %.03f, %s)", m_fAvrFps, double(m_rtTimePerFrame) / 10000.0, 10000000.0 / (double)(m_rtTimePerFrame), m_bInterlaced ? L"I" : L"P");
		}
//			strText.Format(L"Frame rate   : %7.03f   (%7.3f ms = %.03f, %s)   (%7.3f ms = %.03f%s, %2.03f StdDev)", m_fAvrFps, double(m_rtTimePerFrame) / 10000.0, 10000000.0 / (double)(m_rtTimePerFrame), m_bInterlaced ? L"I" : L"P", GetFrameTime() * 1000.0, GetFrameRate(), m_DetectedLock ? L" L" : L"", m_DetectedFrameTimeStdDev / 10000.0);
		else
			strText.Format(L"Frame rate   : %7.03f   (%.03f%s)", m_fAvrFps, GetFrameRate(), m_DetectedLock ? L" L" : L"");
		DrawText(rc, strText, 1);
		OffsetRect (&rc, 0, TextHeight);

		if (bDetailedStats > 1)
		{
			strText.Format(L"Settings     : ");

			if (m_bIsEVR)
				strText += "EVR ";
			else
				strText += "VMR9 ";

			if (s.fD3DFullscreen)
				strText += "FS ";
			if (s.m_RenderSettings.iVMR9FullscreenGUISupport)
				strText += "FSGui ";

			if (s.m_RenderSettings.iVMRDisableDesktopComposition)
				strText += "DisDC ";

			if (s.m_RenderSettings.iVMRFlushGPUBeforeVSync)
				strText += "GPUFlushBV ";
			if (s.m_RenderSettings.iVMRFlushGPUAfterPresent)
				strText += "GPUFlushAP ";

			if (s.m_RenderSettings.iVMRFlushGPUWait)
				strText += "GPUFlushWt ";

			if (s.m_RenderSettings.iVMR9VSync)
				strText += "VS ";
			if (s.m_RenderSettings.fVMR9AlterativeVSync)
				strText += "AltVS ";
			if (s.m_RenderSettings.iVMR9VSyncAccurate)
				strText += "AccVS ";
			if (s.m_RenderSettings.iVMR9VSyncOffset)
				strText.AppendFormat(L"VSOfst(%d)", s.m_RenderSettings.iVMR9VSyncOffset);

			if (m_bIsEVR)
			{
				if (s.m_RenderSettings.iEVRHighColorResolution)
					strText += "10bit ";
				if (s.m_RenderSettings.iEVREnableFrameTimeCorrection)
					strText += "FTC ";
				if (s.m_RenderSettings.iEVROutputRange == 0)
					strText += "0-255 ";
				else if (s.m_RenderSettings.iEVROutputRange == 1)
					strText += "16-235 ";
			}


			DrawText(rc, strText, 1);
			OffsetRect (&rc, 0, TextHeight);

		}

		if (bDetailedStats > 1)
		{
			strText.Format(L"Formats      : Surface %s    Backbuffer %s    Display %s     Device %s      D3DExError: %s", GetD3DFormatStr(m_SurfaceType), GetD3DFormatStr(m_BackbufferType), GetD3DFormatStr(m_DisplayType), m_pD3DDevEx ? L"D3DDevEx" : L"D3DDev", m_D3DDevExError.GetString());
			DrawText(rc, strText, 1);
			OffsetRect (&rc, 0, TextHeight);

			if (m_bIsEVR)
			{
				strText.Format(L"Refresh rate : %.05f Hz    SL: %4d     (%3d Hz)      Last Duration: %10.6f      Corrected Frame Time: %s", m_DetectedRefreshRate, int(m_DetectedScanlinesPerFrame + 0.5), m_RefreshRate, double(m_LastFrameDuration)/10000.0, m_bCorrectedFrameTime?L"Yes":L"No");
				DrawText(rc, strText, 1);
				OffsetRect (&rc, 0, TextHeight);
			}
		}

		if (m_bSyncStatsAvailable)
		{
			if (bDetailedStats > 1)
				strText.Format(L"Sync offset  : Min = %+8.3f ms, Max = %+8.3f ms, StdDev = %7.3f ms, Avr = %7.3f ms, Mode = %d", (double(llMinSyncOffset)/10000.0), (double(llMaxSyncOffset)/10000.0), m_fSyncOffsetStdDev/10000.0, m_fSyncOffsetAvr/10000.0, m_VSyncMode);
			else
				strText.Format(L"Sync offset  : Mode = %d", m_VSyncMode);
			DrawText(rc, strText, 1);
			OffsetRect (&rc, 0, TextHeight);
		}

		if (bDetailedStats > 1)
		{
			strText.Format(L"Jitter       : Min = %+8.3f ms, Max = %+8.3f ms, StdDev = %7.3f ms", (double(llMinJitter)/10000.0), (double(llMaxJitter)/10000.0), m_fJitterStdDev/10000.0);
			DrawText(rc, strText, 1);
			OffsetRect (&rc, 0, TextHeight);
		}

		if (m_pAllocator && bDetailedStats > 1)
		{
			CDX9SubPicAllocator *pAlloc = (CDX9SubPicAllocator *)m_pAllocator.p;
			int nFree = 0;
			int nAlloc = 0;
			int nSubPic = 0;
			REFERENCE_TIME QueueNow = 0;
			REFERENCE_TIME QueueStart = 0;
			REFERENCE_TIME QueueEnd = 0;
			if (m_pSubPicQueue)
			{
				m_pSubPicQueue->GetStats(nSubPic, QueueNow, QueueStart, QueueEnd);
				if (QueueStart)
					QueueStart -= QueueNow;
				if (QueueEnd)
					QueueEnd -= QueueNow;
			}
			pAlloc->GetStats(nFree, nAlloc);
			strText.Format(L"Subtitles    : Free %d     Allocated %d     Buffered %d     QueueStart %7.3f     QueueEnd %7.3f", nFree, nAlloc, nSubPic, (double(QueueStart)/10000000.0), (double(QueueEnd)/10000000.0));
			DrawText(rc, strText, 1);
	 		OffsetRect (&rc, 0, TextHeight);
		}

		if (bDetailedStats > 1)
		{
			if (m_VBlankEndPresent == -100000)
				strText.Format(L"VBlank Wait  : Start %4d   End %4d   Wait %7.3f ms   Offset %4d   Max %4d", m_VBlankStartWait, m_VBlankEndWait, (double(m_VBlankWaitTime)/10000.0), m_VBlankMin, m_VBlankMax - m_VBlankMin);
			else
				strText.Format(L"VBlank Wait  : Start %4d   End %4d   Wait %7.3f ms   Offset %4d   Max %4d   EndPresent %4d", m_VBlankStartWait, m_VBlankEndWait, (double(m_VBlankWaitTime)/10000.0), m_VBlankMin, m_VBlankMax - m_VBlankMin, m_VBlankEndPresent);
		}
		else
		{
			if (m_VBlankEndPresent == -100000)
				strText.Format(L"VBlank Wait  : Start %4d   End %4d", m_VBlankStartWait, m_VBlankEndWait);
			else
				strText.Format(L"VBlank Wait  : Start %4d   End %4d   EP %4d", m_VBlankStartWait, m_VBlankEndWait, m_VBlankEndPresent);
		}
		DrawText(rc, strText, 1);
		OffsetRect (&rc, 0, TextHeight);

		BOOL bCompositionEnabled = m_bCompositionEnabled;

		bool bDoVSyncInPresent = (!bCompositionEnabled && !m_bAlternativeVSync) || !s.m_RenderSettings.iVMR9VSync;

		if (bDetailedStats > 1 && bDoVSyncInPresent)
		{
			strText.Format(L"Present Wait : Wait %7.3f ms   Min %7.3f ms   Max %7.3f ms", (double(m_PresentWaitTime)/10000.0), (double(m_PresentWaitTimeMin)/10000.0), (double(m_PresentWaitTimeMax)/10000.0));
			DrawText(rc, strText, 1);
			OffsetRect (&rc, 0, TextHeight);
		}

		if (bDetailedStats > 1)
		{
			if (m_WaitForGPUTime)
				strText.Format(L"Paint Time   : Draw %7.3f ms   Min %7.3f ms   Max %7.3f ms   GPU %7.3f ms", (double(m_PaintTime-m_WaitForGPUTime)/10000.0), (double(m_PaintTimeMin)/10000.0), (double(m_PaintTimeMax)/10000.0), (double(m_WaitForGPUTime)/10000.0));
			else
				strText.Format(L"Paint Time   : Draw %7.3f ms   Min %7.3f ms   Max %7.3f ms", (double(m_PaintTime-m_WaitForGPUTime)/10000.0), (double(m_PaintTimeMin)/10000.0), (double(m_PaintTimeMax)/10000.0));
		}
		else
		{
			if (m_WaitForGPUTime)
				strText.Format(L"Paint Time   : Draw %7.3f ms   GPU %7.3f ms", (double(m_PaintTime - m_WaitForGPUTime)/10000.0), (double(m_WaitForGPUTime)/10000.0));
			else
				strText.Format(L"Paint Time   : Draw %7.3f ms", (double(m_PaintTime - m_WaitForGPUTime)/10000.0));
		}
		DrawText(rc, strText, 2);
		OffsetRect (&rc, 0, TextHeight);

		if (bDetailedStats > 1)
		{
			strText.Format(L"Raster Status: Wait %7.3f ms   Min %7.3f ms   Max %7.3f ms", (double(m_RasterStatusWaitTime)/10000.0), (double(m_RasterStatusWaitTimeMin)/10000.0), (double(m_RasterStatusWaitTimeMax)/10000.0));
			DrawText(rc, strText, 1);
	 		OffsetRect (&rc, 0, TextHeight);
		}

		if (bDetailedStats > 1)
		{
			if (m_bIsEVR)
				strText.Format(L"Buffering    : Buffered %3d    Free %3d    Current Surface %3d", m_nUsedBuffer, m_nNbDXSurface - m_nUsedBuffer, m_nCurSurface, m_nVMR9Surfaces, m_iVMR9Surface);
			else
				strText.Format(L"Buffering    : VMR9Surfaces %3d   VMR9Surface %3d", m_nVMR9Surfaces, m_iVMR9Surface);
		}
		else
			strText.Format(L"Buffered     : %3d", m_nUsedBuffer);
		DrawText(rc, strText, 1);
		OffsetRect (&rc, 0, TextHeight);

		if (bDetailedStats > 1)
		{
			strText.Format(L"Video size   : %d x %d  (AR = %d x %d)", m_NativeVideoSize.cx, m_NativeVideoSize.cy, m_AspectRatio.cx, m_AspectRatio.cy);
			DrawText(rc, strText, 1);
			OffsetRect (&rc, 0, TextHeight);

			strText.Format(L"%-13s: %s", GetDXVAVersion(), GetDXVADecoderDescription());
			DrawText(rc, strText, 1);
			OffsetRect (&rc, 0, TextHeight);

			strText.Format(L"DirectX SDK  : %d", AfxGetMyApp()->GetDXSdkRelease());
			DrawText(rc, strText, 1);
			OffsetRect (&rc, 0, TextHeight);

			for (int i=0; i<6; i++)
			{
				if (m_strStatsMsg[i][0])
				{
					DrawText(rc, m_strStatsMsg[i], 1);
					OffsetRect (&rc, 0, TextHeight);
				}
			}
		}
		m_pSprite->End();
	}

	if (m_pLine && bDetailedStats)
	{
		D3DXVECTOR2		Points[NB_JITTER];
		int				nIndex;

		int StartX = 0;
		int StartY = 0;
		int ScaleX = 1;
		int ScaleY = 1;
		int DrawWidth = 625 * ScaleX + 50;
		int DrawHeight = 500 * ScaleY;
		int Alpha = 80;
		StartX = m_WindowRect.Width() - (DrawWidth + 20);
		StartY = m_WindowRect.Height() - (DrawHeight + 20);

		DrawRect(RGB(0,0,0), Alpha, CRect(StartX, StartY, StartX + DrawWidth, StartY + DrawHeight));
		// === Jitter Graduation
//		m_pLine->SetWidth(2.2);          // Width 
//		m_pLine->SetAntialias(1);
		m_pLine->SetWidth(2.5);          // Width 
		m_pLine->SetAntialias(1);
//		m_pLine->SetGLLines(1);
		m_pLine->Begin();

		for (int i=10; i<500*ScaleY; i+= 20*ScaleY)
		{
			Points[0].x = (FLOAT)StartX;
			Points[0].y = (FLOAT)(StartY + i);
			Points[1].x = (FLOAT)(StartX + ((i-10)%80 ? 50 : 625 * ScaleX));
			Points[1].y = (FLOAT)(StartY + i);
			if (i == 250) Points[1].x += 50;
			m_pLine->Draw (Points, 2, D3DCOLOR_XRGB(100,100,255));
		}

		// === Jitter curve
		if (m_rtTimePerFrame)
		{
			for (int i=0; i<NB_JITTER; i++)
			{
				nIndex = (m_nNextJitter+1+i) % NB_JITTER;
				if (nIndex < 0)
					nIndex += NB_JITTER;
				double Jitter = m_pllJitter[nIndex] - m_fJitterMean;
				Points[i].x  = (FLOAT)(StartX + (i*5*ScaleX+5));
				Points[i].y  = (FLOAT)(StartY + ((Jitter*ScaleY)/5000.0 + 250.0* ScaleY));
			}		
			m_pLine->Draw (Points, NB_JITTER, D3DCOLOR_XRGB(255,100,100));

			if (m_bSyncStatsAvailable)
			{
				for (int i=0; i<NB_JITTER; i++)
				{
					nIndex = (m_nNextSyncOffset+1+i) % NB_JITTER;
					if (nIndex < 0)
						nIndex += NB_JITTER;
					Points[i].x  = (FLOAT)(StartX + (i*5*ScaleX+5));
					Points[i].y  = (FLOAT)(StartY + ((m_pllSyncOffset[nIndex]*ScaleY)/5000 + 250*ScaleY));
				}		
				m_pLine->Draw (Points, NB_JITTER, D3DCOLOR_XRGB(100,200,100));
			}
		}
		m_pLine->End();
	}

	// === Text

}

STDMETHODIMP CDX9AllocatorPresenter::GetDIB(BYTE* lpDib, DWORD* size)
{
	CheckPointer(size, E_POINTER);

	HRESULT hr;

	D3DSURFACE_DESC desc;
	memset(&desc, 0, sizeof(desc));
	m_pVideoSurface[m_nCurSurface]->GetDesc(&desc);

	DWORD required = sizeof(BITMAPINFOHEADER) + (desc.Width * desc.Height * 32 >> 3);
	if(!lpDib) {*size = required; return S_OK;}
	if(*size < required) return E_OUTOFMEMORY;
	*size = required;

	CComPtr<IDirect3DSurface9> pSurface = m_pVideoSurface[m_nCurSurface];
	D3DLOCKED_RECT r;
	if(FAILED(hr = pSurface->LockRect(&r, NULL, D3DLOCK_READONLY)))
	{
		pSurface = NULL;
		if(FAILED(hr = m_pD3DDev->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &pSurface, NULL))
		|| FAILED(hr = m_pD3DDev->GetRenderTargetData(m_pVideoSurface[m_nCurSurface], pSurface))
		|| FAILED(hr = pSurface->LockRect(&r, NULL, D3DLOCK_READONLY)))
			return hr;
	}

	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)lpDib;
	memset(bih, 0, sizeof(BITMAPINFOHEADER));
	bih->biSize = sizeof(BITMAPINFOHEADER);
	bih->biWidth = desc.Width;
	bih->biHeight = desc.Height;
	bih->biBitCount = 32;
	bih->biPlanes = 1;
	bih->biSizeImage = bih->biWidth * bih->biHeight * bih->biBitCount >> 3;

	BitBltFromRGBToRGB(
		bih->biWidth, bih->biHeight, 
		(BYTE*)(bih + 1), bih->biWidth*bih->biBitCount>>3, bih->biBitCount,
		(BYTE*)r.pBits + r.Pitch*(desc.Height-1), -(int)r.Pitch, 32);

	pSurface->UnlockRect();

	return S_OK;
}

STDMETHODIMP CDX9AllocatorPresenter::SetPixelShader(LPCSTR pSrcData, LPCSTR pTarget)
{
	return SetPixelShader2(pSrcData, pTarget, false);
}

STDMETHODIMP CDX9AllocatorPresenter::SetPixelShader2(LPCSTR pSrcData, LPCSTR pTarget, bool bScreenSpace)
{
	CAutoLock cRenderLock(&m_RenderLock);

	CAtlList<CExternalPixelShader> *pPixelShaders;
	if (bScreenSpace)
		pPixelShaders = &m_pPixelShadersScreenSpace;
	else
		pPixelShaders = &m_pPixelShaders;

	if(!pSrcData && !pTarget)
	{
		pPixelShaders->RemoveAll();
		m_pD3DDev->SetPixelShader(NULL);
		return S_OK;
	}

	if(!pSrcData || !pTarget)
		return E_INVALIDARG;

	CExternalPixelShader Shader;
	Shader.m_SourceData = pSrcData;
	Shader.m_SourceTarget = pTarget;
	
	CComPtr<IDirect3DPixelShader9> pPixelShader;

	HRESULT hr = Shader.Compile(m_pPSC);
	if(FAILED(hr)) 
		return hr;

	pPixelShaders->AddTail(Shader);

	Paint(false);

	return S_OK;
}

//
// CVMR9AllocatorPresenter
//

#define MY_USER_ID 0x6ABE51

CVMR9AllocatorPresenter::CVMR9AllocatorPresenter(HWND hWnd, HRESULT& hr, CString &_Error) 
	: CDX9AllocatorPresenter(hWnd, hr, false, _Error)
	, m_fUseInternalTimer(false)
	, m_rtPrevStart(-1)
{
}

STDMETHODIMP CVMR9AllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return 
		QI(IVMRSurfaceAllocator9)
		QI(IVMRImagePresenter9)
		QI(IVMRWindowlessControl9)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CVMR9AllocatorPresenter::CreateDevice(CString &_Error)
{
	HRESULT hr = __super::CreateDevice(_Error);
	if(FAILED(hr)) 
		return hr;

	if(m_pIVMRSurfAllocNotify)
	{
		HMONITOR hMonitor = m_pD3D->GetAdapterMonitor(GetAdapter(m_pD3D));
		if(FAILED(hr = m_pIVMRSurfAllocNotify->ChangeD3DDevice(m_pD3DDev, hMonitor)))
		{
			_Error += L"m_pIVMRSurfAllocNotify->ChangeD3DDevice failed";
			return(false);
		}
	}

	return hr;
}

void CVMR9AllocatorPresenter::DeleteSurfaces()
{
    CAutoLock cAutoLock(this);
	CAutoLock cRenderLock(&m_RenderLock);

	m_pSurfaces.RemoveAll();

	return __super::DeleteSurfaces();
}

// ISubPicAllocatorPresenter

class COuterVMR9
	: public CUnknown
	, public IVideoWindow
	, public IBasicVideo2
	, public IVMRWindowlessControl
	, public IVMRffdshow9
	, public IVMRMixerBitmap9
{
	CComPtr<IUnknown>	m_pVMR;
	VMR9AlphaBitmap*	m_pVMR9AlphaBitmap;
	CDX9AllocatorPresenter *m_pAllocatorPresenter;

public:

	COuterVMR9(const TCHAR* pName, LPUNKNOWN pUnk, VMR9AlphaBitmap* pVMR9AlphaBitmap, CDX9AllocatorPresenter *_pAllocatorPresenter) : CUnknown(pName, pUnk)
	{
		m_pVMR.CoCreateInstance(CLSID_VideoMixingRenderer9, GetOwner());
		m_pVMR9AlphaBitmap = pVMR9AlphaBitmap;
		m_pAllocatorPresenter = _pAllocatorPresenter;
	}

	~COuterVMR9()
	{
		m_pVMR = NULL;
	}

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
	{
		HRESULT hr;

		// Casimir666 : en mode Renderless faire l'incrustation � la place du VMR
		if(riid == __uuidof(IVMRMixerBitmap9))
			return GetInterface((IVMRMixerBitmap9*)this, ppv);

		hr = m_pVMR ? m_pVMR->QueryInterface(riid, ppv) : E_NOINTERFACE;
		if(m_pVMR && FAILED(hr))
		{
			if(riid == __uuidof(IVideoWindow))
				return GetInterface((IVideoWindow*)this, ppv);
			if(riid == __uuidof(IBasicVideo))
				return GetInterface((IBasicVideo*)this, ppv);
			if(riid == __uuidof(IBasicVideo2))
				return GetInterface((IBasicVideo2*)this, ppv);
			if(riid == __uuidof(IVMRffdshow9)) // Support ffdshow queueing. We show ffdshow that this is patched Media Player Classic.
				return GetInterface((IVMRffdshow9*)this, ppv);
/*			if(riid == __uuidof(IVMRWindowlessControl))
				return GetInterface((IVMRWindowlessControl*)this, ppv);
*/
		}

		return SUCCEEDED(hr) ? hr : __super::NonDelegatingQueryInterface(riid, ppv);
	}

	// IVMRWindowlessControl

	STDMETHODIMP GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight)
	{
		if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
		{
			return pWC9->GetNativeVideoSize(lpWidth, lpHeight, lpARWidth, lpARHeight);
		}

		return E_NOTIMPL;
	}
	STDMETHODIMP GetMinIdealVideoSize(LONG* lpWidth, LONG* lpHeight) {return E_NOTIMPL;}
	STDMETHODIMP GetMaxIdealVideoSize(LONG* lpWidth, LONG* lpHeight) {return E_NOTIMPL;}
	STDMETHODIMP SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect) {return E_NOTIMPL;}
    STDMETHODIMP GetVideoPosition(LPRECT lpSRCRect, LPRECT lpDSTRect)
	{
		if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
		{
			return pWC9->GetVideoPosition(lpSRCRect, lpDSTRect);
		}

		return E_NOTIMPL;
	}
	STDMETHODIMP GetAspectRatioMode(DWORD* lpAspectRatioMode)
	{
		if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
		{
			*lpAspectRatioMode = VMR_ARMODE_NONE;
			return S_OK;
		}

		return E_NOTIMPL;
	}
	STDMETHODIMP SetAspectRatioMode(DWORD AspectRatioMode) {return E_NOTIMPL;}
	STDMETHODIMP SetVideoClippingWindow(HWND hwnd) {return E_NOTIMPL;}
	STDMETHODIMP RepaintVideo(HWND hwnd, HDC hdc) {return E_NOTIMPL;}
	STDMETHODIMP DisplayModeChanged() {return E_NOTIMPL;}
	STDMETHODIMP GetCurrentImage(BYTE** lpDib) {return E_NOTIMPL;}
	STDMETHODIMP SetBorderColor(COLORREF Clr) {return E_NOTIMPL;}
	STDMETHODIMP GetBorderColor(COLORREF* lpClr) {return E_NOTIMPL;}
	STDMETHODIMP SetColorKey(COLORREF Clr) {return E_NOTIMPL;}
	STDMETHODIMP GetColorKey(COLORREF* lpClr) {return E_NOTIMPL;}

	// IVideoWindow
	STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) {return E_NOTIMPL;}
	STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) {return E_NOTIMPL;}
	STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId) {return E_NOTIMPL;}
	STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {return E_NOTIMPL;}
    STDMETHODIMP put_Caption(BSTR strCaption) {return E_NOTIMPL;}
    STDMETHODIMP get_Caption(BSTR* strCaption) {return E_NOTIMPL;}
	STDMETHODIMP put_WindowStyle(long WindowStyle) {return E_NOTIMPL;}
	STDMETHODIMP get_WindowStyle(long* WindowStyle) {return E_NOTIMPL;}
	STDMETHODIMP put_WindowStyleEx(long WindowStyleEx) {return E_NOTIMPL;}
	STDMETHODIMP get_WindowStyleEx(long* WindowStyleEx) {return E_NOTIMPL;}
	STDMETHODIMP put_AutoShow(long AutoShow) {return E_NOTIMPL;}
	STDMETHODIMP get_AutoShow(long* AutoShow) {return E_NOTIMPL;}
	STDMETHODIMP put_WindowState(long WindowState) {return E_NOTIMPL;}
	STDMETHODIMP get_WindowState(long* WindowState) {return E_NOTIMPL;}
	STDMETHODIMP put_BackgroundPalette(long BackgroundPalette) {return E_NOTIMPL;}
	STDMETHODIMP get_BackgroundPalette(long* pBackgroundPalette) {return E_NOTIMPL;}
	STDMETHODIMP put_Visible(long Visible) {return E_NOTIMPL;}
	STDMETHODIMP get_Visible(long* pVisible) {return E_NOTIMPL;}
	STDMETHODIMP put_Left(long Left) {return E_NOTIMPL;}
	STDMETHODIMP get_Left(long* pLeft) {return E_NOTIMPL;}
	STDMETHODIMP put_Width(long Width) {return E_NOTIMPL;}
	STDMETHODIMP get_Width(long* pWidth)
	{
		if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
		{
			CRect s, d;
			HRESULT hr = pWC9->GetVideoPosition(&s, &d);
			*pWidth = d.Width();
			return hr;
		}

		return E_NOTIMPL;
	}
	STDMETHODIMP put_Top(long Top) {return E_NOTIMPL;}
	STDMETHODIMP get_Top(long* pTop) {return E_NOTIMPL;}
	STDMETHODIMP put_Height(long Height) {return E_NOTIMPL;}
	STDMETHODIMP get_Height(long* pHeight)
	{
		if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
		{
			CRect s, d;
			HRESULT hr = pWC9->GetVideoPosition(&s, &d);
			*pHeight = d.Height();
			return hr;
		}

		return E_NOTIMPL;
	}
	STDMETHODIMP put_Owner(OAHWND Owner) {return E_NOTIMPL;}
	STDMETHODIMP get_Owner(OAHWND* Owner) {return E_NOTIMPL;}
	STDMETHODIMP put_MessageDrain(OAHWND Drain) {return E_NOTIMPL;}
	STDMETHODIMP get_MessageDrain(OAHWND* Drain) {return E_NOTIMPL;}
	STDMETHODIMP get_BorderColor(long* Color) {return E_NOTIMPL;}
	STDMETHODIMP put_BorderColor(long Color) {return E_NOTIMPL;}
	STDMETHODIMP get_FullScreenMode(long* FullScreenMode) {return E_NOTIMPL;}
	STDMETHODIMP put_FullScreenMode(long FullScreenMode) {return E_NOTIMPL;}
    STDMETHODIMP SetWindowForeground(long Focus) {return E_NOTIMPL;}
    STDMETHODIMP NotifyOwnerMessage(OAHWND hwnd, long uMsg, LONG_PTR wParam, LONG_PTR lParam) {return E_NOTIMPL;}
    STDMETHODIMP SetWindowPosition(long Left, long Top, long Width, long Height) {return E_NOTIMPL;}
	STDMETHODIMP GetWindowPosition(long* pLeft, long* pTop, long* pWidth, long* pHeight) {return E_NOTIMPL;}
	STDMETHODIMP GetMinIdealImageSize(long* pWidth, long* pHeight) {return E_NOTIMPL;}
	STDMETHODIMP GetMaxIdealImageSize(long* pWidth, long* pHeight) {return E_NOTIMPL;}
	STDMETHODIMP GetRestorePosition(long* pLeft, long* pTop, long* pWidth, long* pHeight) {return E_NOTIMPL;}
	STDMETHODIMP HideCursor(long HideCursor) {return E_NOTIMPL;}
	STDMETHODIMP IsCursorHidden(long* CursorHidden) {return E_NOTIMPL;}

	// IBasicVideo2
    STDMETHODIMP get_AvgTimePerFrame(REFTIME* pAvgTimePerFrame) {return E_NOTIMPL;}
    STDMETHODIMP get_BitRate(long* pBitRate) {return E_NOTIMPL;}
    STDMETHODIMP get_BitErrorRate(long* pBitErrorRate) {return E_NOTIMPL;}
    STDMETHODIMP get_VideoWidth(long* pVideoWidth) {return E_NOTIMPL;}
    STDMETHODIMP get_VideoHeight(long* pVideoHeight) {return E_NOTIMPL;}
    STDMETHODIMP put_SourceLeft(long SourceLeft) {return E_NOTIMPL;}
    STDMETHODIMP get_SourceLeft(long* pSourceLeft) {return E_NOTIMPL;}
    STDMETHODIMP put_SourceWidth(long SourceWidth) {return E_NOTIMPL;}
    STDMETHODIMP get_SourceWidth(long* pSourceWidth) {return E_NOTIMPL;}
    STDMETHODIMP put_SourceTop(long SourceTop) {return E_NOTIMPL;}
    STDMETHODIMP get_SourceTop(long* pSourceTop) {return E_NOTIMPL;}
    STDMETHODIMP put_SourceHeight(long SourceHeight) {return E_NOTIMPL;}
    STDMETHODIMP get_SourceHeight(long* pSourceHeight) {return E_NOTIMPL;}
    STDMETHODIMP put_DestinationLeft(long DestinationLeft) {return E_NOTIMPL;}
    STDMETHODIMP get_DestinationLeft(long* pDestinationLeft) {return E_NOTIMPL;}
    STDMETHODIMP put_DestinationWidth(long DestinationWidth) {return E_NOTIMPL;}
    STDMETHODIMP get_DestinationWidth(long* pDestinationWidth) {return E_NOTIMPL;}
    STDMETHODIMP put_DestinationTop(long DestinationTop) {return E_NOTIMPL;}
    STDMETHODIMP get_DestinationTop(long* pDestinationTop) {return E_NOTIMPL;}
    STDMETHODIMP put_DestinationHeight(long DestinationHeight) {return E_NOTIMPL;}
    STDMETHODIMP get_DestinationHeight(long* pDestinationHeight) {return E_NOTIMPL;}
    STDMETHODIMP SetSourcePosition(long Left, long Top, long Width, long Height) {return E_NOTIMPL;}
    STDMETHODIMP GetSourcePosition(long* pLeft, long* pTop, long* pWidth, long* pHeight)
	{
		// DVD Nav. bug workaround fix
		{
			*pLeft = *pTop = 0;
			return GetVideoSize(pWidth, pHeight);
		}
/*
		if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
		{
			CRect s, d;
			HRESULT hr = pWC9->GetVideoPosition(&s, &d);
			*pLeft = s.left;
			*pTop = s.top;
			*pWidth = s.Width();
			*pHeight = s.Height();
			return hr;
		}
*/
		return E_NOTIMPL;
	}
    STDMETHODIMP SetDefaultSourcePosition() {return E_NOTIMPL;}
    STDMETHODIMP SetDestinationPosition(long Left, long Top, long Width, long Height) {return E_NOTIMPL;}
    STDMETHODIMP GetDestinationPosition(long* pLeft, long* pTop, long* pWidth, long* pHeight)
	{
		if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
		{
			CRect s, d;
			HRESULT hr = pWC9->GetVideoPosition(&s, &d);
			*pLeft = d.left;
			*pTop = d.top;
			*pWidth = d.Width();
			*pHeight = d.Height();
			return hr;
		}

		return E_NOTIMPL;
	}
    STDMETHODIMP SetDefaultDestinationPosition() {return E_NOTIMPL;}
    STDMETHODIMP GetVideoSize(long* pWidth, long* pHeight)
	{
		if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
		{
			LONG aw, ah;
//			return pWC9->GetNativeVideoSize(pWidth, pHeight, &aw, &ah);
			// DVD Nav. bug workaround fix
			HRESULT hr = pWC9->GetNativeVideoSize(pWidth, pHeight, &aw, &ah);
			*pWidth = *pHeight * aw / ah;
			return hr;
		}

		return E_NOTIMPL;
	}
	// IVMRffdshow9
	STDMETHODIMP support_ffdshow()
	{
		queueu_ffdshow_support = true;
		return S_OK;
	}

    STDMETHODIMP GetVideoPaletteEntries(long StartIndex, long Entries, long* pRetrieved, long* pPalette) {return E_NOTIMPL;}
    STDMETHODIMP GetCurrentImage(long* pBufferSize, long* pDIBImage) {return E_NOTIMPL;}
    STDMETHODIMP IsUsingDefaultSource() {return E_NOTIMPL;}
    STDMETHODIMP IsUsingDefaultDestination() {return E_NOTIMPL;}

	STDMETHODIMP GetPreferredAspectRatio(long* plAspectX, long* plAspectY)
	{
		if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
		{
			LONG w, h;
			return pWC9->GetNativeVideoSize(&w, &h, plAspectX, plAspectY);
		}

		return E_NOTIMPL;
	}

	// IVMRMixerBitmap9
	STDMETHODIMP GetAlphaBitmapParameters(VMR9AlphaBitmap* pBmpParms)
	{
		CheckPointer(pBmpParms, E_POINTER);
		CAutoLock BitMapLock(&m_pAllocatorPresenter->m_VMR9AlphaBitmapLock);
		memcpy (pBmpParms, m_pVMR9AlphaBitmap, sizeof(VMR9AlphaBitmap));
		return S_OK;
	}
	
	STDMETHODIMP SetAlphaBitmap(const VMR9AlphaBitmap*  pBmpParms)
	{
		CheckPointer(pBmpParms, E_POINTER);
		CAutoLock BitMapLock(&m_pAllocatorPresenter->m_VMR9AlphaBitmapLock);
		memcpy (m_pVMR9AlphaBitmap, pBmpParms, sizeof(VMR9AlphaBitmap));
		m_pVMR9AlphaBitmap->dwFlags |= VMRBITMAP_UPDATE;
		m_pAllocatorPresenter->UpdateAlphaBitmap();
		return S_OK;
	}

	STDMETHODIMP UpdateAlphaBitmapParameters(const VMR9AlphaBitmap* pBmpParms)
	{
		CheckPointer(pBmpParms, E_POINTER);
		CAutoLock BitMapLock(&m_pAllocatorPresenter->m_VMR9AlphaBitmapLock);
		memcpy (m_pVMR9AlphaBitmap, pBmpParms, sizeof(VMR9AlphaBitmap));
		m_pVMR9AlphaBitmap->dwFlags |= VMRBITMAP_UPDATE;
		m_pAllocatorPresenter->UpdateAlphaBitmap();
		return S_OK;
	}
};

STDMETHODIMP CVMR9AllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
    CheckPointer(ppRenderer, E_POINTER);

	*ppRenderer = NULL;

	HRESULT hr;

	do
	{
		CMacrovisionKicker* pMK = DNew CMacrovisionKicker(NAME("CMacrovisionKicker"), NULL);
		CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)pMK;

		COuterVMR9 *pOuter = DNew COuterVMR9(NAME("COuterVMR9"), pUnk, &m_VMR9AlphaBitmap, this);


		pMK->SetInner((IUnknown*)(INonDelegatingUnknown*)pOuter);
		CComQIPtr<IBaseFilter> pBF = pUnk;

		CComPtr<IPin> pPin = GetFirstPin(pBF);
		CComQIPtr<IMemInputPin> pMemInputPin = pPin;
		m_fUseInternalTimer = HookNewSegmentAndReceive((IPinC*)(IPin*)pPin, (IMemInputPinC*)(IMemInputPin*)pMemInputPin);

		if(CComQIPtr<IAMVideoAccelerator> pAMVA = pPin)
			HookAMVideoAccelerator((IAMVideoAcceleratorC*)(IAMVideoAccelerator*)pAMVA);

		CComQIPtr<IVMRFilterConfig9> pConfig = pBF;
		if(!pConfig)
			break;

		AppSettings& s = AfxGetAppSettings();

		if(s.fVMR9MixerMode)
		{
			if(FAILED(hr = pConfig->SetNumberOfStreams(1)))
				break;

			if(s.fVMR9MixerYUV && !AfxGetMyApp()->IsVistaOrAbove())
			{
				if(CComQIPtr<IVMRMixerControl9> pMC = pBF)
				{
					DWORD dwPrefs;
					pMC->GetMixingPrefs(&dwPrefs);  
					dwPrefs &= ~MixerPref9_RenderTargetMask; 
					dwPrefs |= MixerPref9_RenderTargetYUV;
					pMC->SetMixingPrefs(dwPrefs);
				}
			}
		}

		if(FAILED(hr = pConfig->SetRenderingMode(VMR9Mode_Renderless)))
			break;

		CComQIPtr<IVMRSurfaceAllocatorNotify9> pSAN = pBF;
		if(!pSAN)
			break;

		if(FAILED(hr = pSAN->AdviseSurfaceAllocator(MY_USER_ID, static_cast<IVMRSurfaceAllocator9*>(this)))
		|| FAILED(hr = AdviseNotify(pSAN)))
			break;

		*ppRenderer = (IUnknown*)pBF.Detach();

		return S_OK;
	}
	while(0);

    return E_FAIL;
}

STDMETHODIMP_(void) CVMR9AllocatorPresenter::SetTime(REFERENCE_TIME rtNow)
{
	__super::SetTime(rtNow);
	m_fUseInternalTimer = false;
}

// IVMRSurfaceAllocator9

STDMETHODIMP CVMR9AllocatorPresenter::InitializeDevice(DWORD_PTR dwUserID, VMR9AllocationInfo* lpAllocInfo, DWORD* lpNumBuffers)
{

	if(!lpAllocInfo || !lpNumBuffers)
		return E_POINTER;

	if(!m_pIVMRSurfAllocNotify)
		return E_FAIL;

	if((GetAsyncKeyState(VK_CONTROL)&0x80000000))
	if(lpAllocInfo->Format == '21VY' || lpAllocInfo->Format == '024I')
		return E_FAIL;

	DeleteSurfaces();

	int nOriginal = *lpNumBuffers;

	if (*lpNumBuffers == 1)
	{
		*lpNumBuffers = 4;
		m_nVMR9Surfaces = 4;
	}
	else
		m_nVMR9Surfaces = 0;
	m_pSurfaces.SetCount(*lpNumBuffers);

	int w = lpAllocInfo->dwWidth;
	int h = abs((int)lpAllocInfo->dwHeight);

	HRESULT hr;

	if(lpAllocInfo->dwFlags & VMR9AllocFlag_3DRenderTarget)
		lpAllocInfo->dwFlags |= VMR9AllocFlag_TextureSurface;

	hr = m_pIVMRSurfAllocNotify->AllocateSurfaceHelper(lpAllocInfo, lpNumBuffers, &m_pSurfaces[0]);
	if(FAILED(hr)) return hr;

	m_pSurfaces.SetCount(*lpNumBuffers);

	m_NativeVideoSize = m_AspectRatio = CSize(w, h);
	int arx = lpAllocInfo->szAspectRatio.cx, ary = lpAllocInfo->szAspectRatio.cy;
	if(arx > 0 && ary > 0) m_AspectRatio.SetSize(arx, ary);

	if(FAILED(hr = AllocSurfaces()))
		return hr;

	if(!(lpAllocInfo->dwFlags & VMR9AllocFlag_TextureSurface))
	{
		// test if the colorspace is acceptable
		if(FAILED(hr = m_pD3DDev->StretchRect(m_pSurfaces[0], NULL, m_pVideoSurface[m_nCurSurface], NULL, D3DTEXF_NONE)))
		{
			DeleteSurfaces();
			return E_FAIL;
		}
	}

	hr = m_pD3DDev->ColorFill(m_pVideoSurface[m_nCurSurface], NULL, 0);

	if (m_nVMR9Surfaces && m_nVMR9Surfaces != *lpNumBuffers)
		m_nVMR9Surfaces = *lpNumBuffers;
	*lpNumBuffers = min(nOriginal, *lpNumBuffers);
	m_iVMR9Surface = 0;

	return hr;
}

STDMETHODIMP CVMR9AllocatorPresenter::TerminateDevice(DWORD_PTR dwUserID)
{
    DeleteSurfaces();
    return S_OK;
}

STDMETHODIMP CVMR9AllocatorPresenter::GetSurface(DWORD_PTR dwUserID, DWORD SurfaceIndex, DWORD SurfaceFlags, IDirect3DSurface9** lplpSurface)
{
    if(!lplpSurface)
		return E_POINTER;

	if(SurfaceIndex >= m_pSurfaces.GetCount()) 
        return E_FAIL;

	CAutoLock cRenderLock(&m_RenderLock);

	if (m_nVMR9Surfaces)
	{
		++m_iVMR9Surface;
		m_iVMR9Surface = m_iVMR9Surface % m_nVMR9Surfaces;
		(*lplpSurface = m_pSurfaces[m_iVMR9Surface + SurfaceIndex])->AddRef();
	}
	else
	{
		m_iVMR9Surface = SurfaceIndex;
		(*lplpSurface = m_pSurfaces[SurfaceIndex])->AddRef();
	}

	return S_OK;
}

STDMETHODIMP CVMR9AllocatorPresenter::AdviseNotify(IVMRSurfaceAllocatorNotify9* lpIVMRSurfAllocNotify)
{
    CAutoLock cAutoLock(this);
	CAutoLock cRenderLock(&m_RenderLock);

	m_pIVMRSurfAllocNotify = lpIVMRSurfAllocNotify;

	HRESULT hr;
    HMONITOR hMonitor = m_pD3D->GetAdapterMonitor(GetAdapter(m_pD3D));
    if(FAILED(hr = m_pIVMRSurfAllocNotify->SetD3DDevice(m_pD3DDev, hMonitor)))
		return hr;

    return S_OK;
}

// IVMRImagePresenter9

STDMETHODIMP CVMR9AllocatorPresenter::StartPresenting(DWORD_PTR dwUserID)
{
    CAutoLock cAutoLock(this);
	CAutoLock cRenderLock(&m_RenderLock);

    ASSERT(m_pD3DDev);

	return m_pD3DDev ? S_OK : E_FAIL;
}

STDMETHODIMP CVMR9AllocatorPresenter::StopPresenting(DWORD_PTR dwUserID)
{
	return S_OK;
}


STDMETHODIMP CVMR9AllocatorPresenter::PresentImage(DWORD_PTR dwUserID, VMR9PresentationInfo* lpPresInfo)
{
	CheckPointer(m_pIVMRSurfAllocNotify, E_UNEXPECTED);

	if (m_rtTimePerFrame == 0)
	{
		CComPtr<IBaseFilter>	pVMR9;
		CComPtr<IPin>			pPin;
		CMediaType				mt;
		
		if (SUCCEEDED (m_pIVMRSurfAllocNotify->QueryInterface (__uuidof(IBaseFilter), (void**)&pVMR9)) &&
			SUCCEEDED (pVMR9->FindPin(L"VMR Input0", &pPin)) &&
			SUCCEEDED (pPin->ConnectionMediaType(&mt)) )
		{
			ExtractAvgTimePerFrame (&mt, m_rtTimePerFrame);
		}
		// If framerate not set by Video Decoder choose 23.97...
		if (m_rtTimePerFrame == 0) m_rtTimePerFrame = 417166;
	}

    HRESULT hr;

	if(!lpPresInfo || !lpPresInfo->lpSurf)
		return E_POINTER;

	CAutoLock cAutoLock(this);
	CAutoLock cRenderLock(&m_RenderLock);

	CComPtr<IDirect3DTexture9> pTexture;
	lpPresInfo->lpSurf->GetContainer(IID_IDirect3DTexture9, (void**)&pTexture);

	if(pTexture)
	{
		m_pVideoSurface[m_nCurSurface] = lpPresInfo->lpSurf;
		if(m_pVideoTexture[m_nCurSurface]) 
			m_pVideoTexture[m_nCurSurface] = pTexture;
	}
	else
	{
		hr = m_pD3DDev->StretchRect(lpPresInfo->lpSurf, NULL, m_pVideoSurface[m_nCurSurface], NULL, D3DTEXF_NONE);
	}

	if(lpPresInfo->rtEnd > lpPresInfo->rtStart)
	{
		if(m_pSubPicQueue)
		{
			m_pSubPicQueue->SetFPS(m_fps);

			if(m_fUseInternalTimer)
			{
				__super::SetTime(g_tSegmentStart + g_tSampleStart);
			}
		}
	}

	CSize VideoSize = m_NativeVideoSize;
	int arx = lpPresInfo->szAspectRatio.cx, ary = lpPresInfo->szAspectRatio.cy;
	if(arx > 0 && ary > 0) VideoSize.cx = VideoSize.cy*arx/ary;
	if(VideoSize != GetVideoSize())
	{
		m_AspectRatio.SetSize(arx, ary);
		AfxGetApp()->m_pMainWnd->PostMessage(WM_REARRANGERENDERLESS);
	}

	// Tear test bars
	if (AfxGetMyApp()->m_fTearingTest)
	{
		RECT		rcTearing;
		
		rcTearing.left		= m_nTearingPos;
		rcTearing.top		= 0;
		rcTearing.right		= rcTearing.left + 4;
		rcTearing.bottom	= m_NativeVideoSize.cy;
		m_pD3DDev->ColorFill (m_pVideoSurface[m_nCurSurface], &rcTearing, D3DCOLOR_ARGB (255,255,0,0));

		rcTearing.left	= (rcTearing.right + 15) % m_NativeVideoSize.cx;
		rcTearing.right	= rcTearing.left + 4;
		m_pD3DDev->ColorFill (m_pVideoSurface[m_nCurSurface], &rcTearing, D3DCOLOR_ARGB (255,255,0,0));

		m_nTearingPos = (m_nTearingPos + 7) % m_NativeVideoSize.cx;
	}

	Paint(true);

    return S_OK;
}

// IVMRWindowlessControl9
//
// It is only implemented (partially) for the dvd navigator's 
// menu handling, which needs to know a few things about the 
// location of our window.

STDMETHODIMP CVMR9AllocatorPresenter::GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight)
{
	if(lpWidth) *lpWidth = m_NativeVideoSize.cx;
	if(lpHeight) *lpHeight = m_NativeVideoSize.cy;
	if(lpARWidth) *lpARWidth = m_AspectRatio.cx;
	if(lpARHeight) *lpARHeight = m_AspectRatio.cy;
	return S_OK;
}
STDMETHODIMP CVMR9AllocatorPresenter::GetMinIdealVideoSize(LONG* lpWidth, LONG* lpHeight) {return E_NOTIMPL;}
STDMETHODIMP CVMR9AllocatorPresenter::GetMaxIdealVideoSize(LONG* lpWidth, LONG* lpHeight) {return E_NOTIMPL;}
STDMETHODIMP CVMR9AllocatorPresenter::SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect) {return E_NOTIMPL;} // we have our own method for this
STDMETHODIMP CVMR9AllocatorPresenter::GetVideoPosition(LPRECT lpSRCRect, LPRECT lpDSTRect)
{
	CopyRect(lpSRCRect, CRect(CPoint(0, 0), m_NativeVideoSize));
	CopyRect(lpDSTRect, &m_VideoRect);
	return S_OK;
}
STDMETHODIMP CVMR9AllocatorPresenter::GetAspectRatioMode(DWORD* lpAspectRatioMode)
{
	if(lpAspectRatioMode) *lpAspectRatioMode = AM_ARMODE_STRETCHED;
	return S_OK;
}
STDMETHODIMP CVMR9AllocatorPresenter::SetAspectRatioMode(DWORD AspectRatioMode) {return E_NOTIMPL;}
STDMETHODIMP CVMR9AllocatorPresenter::SetVideoClippingWindow(HWND hwnd) {return E_NOTIMPL;}
STDMETHODIMP CVMR9AllocatorPresenter::RepaintVideo(HWND hwnd, HDC hdc) {return E_NOTIMPL;}
STDMETHODIMP CVMR9AllocatorPresenter::DisplayModeChanged() {return E_NOTIMPL;}
STDMETHODIMP CVMR9AllocatorPresenter::GetCurrentImage(BYTE** lpDib) {return E_NOTIMPL;}
STDMETHODIMP CVMR9AllocatorPresenter::SetBorderColor(COLORREF Clr) {return E_NOTIMPL;}
STDMETHODIMP CVMR9AllocatorPresenter::GetBorderColor(COLORREF* lpClr)
{
	if(lpClr) *lpClr = 0;
	return S_OK;
}

//
// CRM9AllocatorPresenter
//

CRM9AllocatorPresenter::CRM9AllocatorPresenter(HWND hWnd, HRESULT& hr, CString &_Error) 
	: CDX9AllocatorPresenter(hWnd, hr, false, _Error)
{
}

STDMETHODIMP CRM9AllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return 
		QI2(IRMAVideoSurface)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CRM9AllocatorPresenter::AllocSurfaces()
{
    CAutoLock cAutoLock(this);
	CAutoLock cRenderLock(&m_RenderLock);

	m_pVideoSurfaceOff = NULL;
	m_pVideoSurfaceYUY2 = NULL;

	HRESULT hr;

	if(FAILED(hr = m_pD3DDev->CreateOffscreenPlainSurface(
		m_NativeVideoSize.cx, m_NativeVideoSize.cy, D3DFMT_X8R8G8B8, 
		D3DPOOL_DEFAULT, &m_pVideoSurfaceOff, NULL)))
		return hr;

	m_pD3DDev->ColorFill(m_pVideoSurfaceOff, NULL, 0);

	if(FAILED(hr = m_pD3DDev->CreateOffscreenPlainSurface(
		m_NativeVideoSize.cx, m_NativeVideoSize.cy, D3DFMT_YUY2, 
		D3DPOOL_DEFAULT, &m_pVideoSurfaceYUY2, NULL)))
		m_pVideoSurfaceYUY2 = NULL;

	if(m_pVideoSurfaceYUY2)
	{
		m_pD3DDev->ColorFill(m_pVideoSurfaceOff, NULL, 0x80108010);
	}

	return __super::AllocSurfaces();
}

void CRM9AllocatorPresenter::DeleteSurfaces()
{
    CAutoLock cAutoLock(this);
	CAutoLock cRenderLock(&m_RenderLock);
	m_pVideoSurfaceOff = NULL;
	m_pVideoSurfaceYUY2 = NULL;
	__super::DeleteSurfaces();
}

// IRMAVideoSurface

STDMETHODIMP CRM9AllocatorPresenter::Blt(UCHAR* pImageData, RMABitmapInfoHeader* pBitmapInfo, REF(PNxRect) inDestRect, REF(PNxRect) inSrcRect)
{
	if(!m_pVideoSurface || !m_pVideoSurfaceOff)
		return E_FAIL;

	bool fRGB = false;
	bool fYUY2 = false;

	CRect src((RECT*)&inSrcRect), dst((RECT*)&inDestRect), src2(CPoint(0,0), src.Size());
	if(src.Width() > dst.Width() || src.Height() > dst.Height())
		return E_FAIL;

	D3DSURFACE_DESC d3dsd;
	ZeroMemory(&d3dsd, sizeof(d3dsd));
	if(FAILED(m_pVideoSurfaceOff->GetDesc(&d3dsd)))
		return E_FAIL;

	int dbpp = 
		d3dsd.Format == D3DFMT_R8G8B8 || d3dsd.Format == D3DFMT_X8R8G8B8 || d3dsd.Format == D3DFMT_A8R8G8B8 ? 32 : 
		d3dsd.Format == D3DFMT_R5G6B5 ? 16 : 0;

	if(pBitmapInfo->biCompression == '024I')
	{
		DWORD pitch = pBitmapInfo->biWidth;
		DWORD size = pitch*abs(pBitmapInfo->biHeight);

		BYTE* y = pImageData					+ src.top*pitch + src.left;
		BYTE* u = pImageData + size				+ src.top*(pitch/2) + src.left/2;
		BYTE* v = pImageData + size + size/4	+ src.top*(pitch/2) + src.left/2;

		if(m_pVideoSurfaceYUY2)
		{
			D3DLOCKED_RECT r;
			if(SUCCEEDED(m_pVideoSurfaceYUY2->LockRect(&r, src2, 0)))
			{
				BitBltFromI420ToYUY2(src.Width(), src.Height(), (BYTE*)r.pBits, r.Pitch, y, u, v, pitch);
				m_pVideoSurfaceYUY2->UnlockRect();
				fYUY2 = true;
			}
		}
		else
		{
			D3DLOCKED_RECT r;
			if(SUCCEEDED(m_pVideoSurfaceOff->LockRect(&r, src2, 0)))
			{
				BitBltFromI420ToRGB(src.Width(), src.Height(), (BYTE*)r.pBits, r.Pitch, dbpp, y, u, v, pitch);
				m_pVideoSurfaceOff->UnlockRect();
				fRGB = true;
			}
		}
	}
	else if(pBitmapInfo->biCompression == '2YUY')
	{
		DWORD w = pBitmapInfo->biWidth;
		DWORD h = abs(pBitmapInfo->biHeight);
		DWORD pitch = pBitmapInfo->biWidth*2;

		BYTE* yvyu = pImageData + src.top*pitch + src.left*2;

		if(m_pVideoSurfaceYUY2)
		{
			D3DLOCKED_RECT r;
			if(SUCCEEDED(m_pVideoSurfaceYUY2->LockRect(&r, src2, 0)))
			{
				BitBltFromYUY2ToYUY2(src.Width(), src.Height(), (BYTE*)r.pBits, r.Pitch, yvyu, pitch);
				m_pVideoSurfaceYUY2->UnlockRect();
				fYUY2 = true;
			}
		}
		else
		{
			D3DLOCKED_RECT r;
			if(SUCCEEDED(m_pVideoSurfaceOff->LockRect(&r, src2, 0)))
			{
				BitBltFromYUY2ToRGB(src.Width(), src.Height(), (BYTE*)r.pBits, r.Pitch, dbpp, yvyu, pitch);
				m_pVideoSurfaceOff->UnlockRect();
				fRGB = true;
			}
		}
	}
	else if(pBitmapInfo->biCompression == 0 || pBitmapInfo->biCompression == 3
		 || pBitmapInfo->biCompression == 'BGRA')
	{
		DWORD w = pBitmapInfo->biWidth;
		DWORD h = abs(pBitmapInfo->biHeight);
		DWORD pitch = pBitmapInfo->biWidth*pBitmapInfo->biBitCount>>3;

		BYTE* rgb = pImageData + src.top*pitch + src.left*(pBitmapInfo->biBitCount>>3);

		D3DLOCKED_RECT r;
		if(SUCCEEDED(m_pVideoSurfaceOff->LockRect(&r, src2, 0)))
		{
			BYTE* pBits = (BYTE*)r.pBits;
			if(pBitmapInfo->biHeight > 0) {pBits += r.Pitch*(src.Height()-1); r.Pitch = -r.Pitch;}
			BitBltFromRGBToRGB(src.Width(), src.Height(), pBits, r.Pitch, dbpp, rgb, pitch, pBitmapInfo->biBitCount);
			m_pVideoSurfaceOff->UnlockRect();
			fRGB = true;
		}
	}

	if(!fRGB && !fYUY2)
	{
		m_pD3DDev->ColorFill(m_pVideoSurfaceOff, NULL, 0);

		HDC hDC;
		if(SUCCEEDED(m_pVideoSurfaceOff->GetDC(&hDC)))
		{
			CString str;
			str.Format(_T("Sorry, this format is not supported"));

			SetBkColor(hDC, 0);
			SetTextColor(hDC, 0x404040);
			TextOut(hDC, 10, 10, str, str.GetLength());

			m_pVideoSurfaceOff->ReleaseDC(hDC);

			fRGB = true;
		}
	}

	HRESULT hr;
	
	if(fRGB)
		hr = m_pD3DDev->StretchRect(m_pVideoSurfaceOff, src2, m_pVideoSurface[m_nCurSurface], dst, D3DTEXF_NONE);
	if(fYUY2)
		hr = m_pD3DDev->StretchRect(m_pVideoSurfaceYUY2, src2, m_pVideoSurface[m_nCurSurface], dst, D3DTEXF_NONE);

	Paint(true);

	return PNR_OK;
}

STDMETHODIMP CRM9AllocatorPresenter::BeginOptimizedBlt(RMABitmapInfoHeader* pBitmapInfo)
{
    CAutoLock cAutoLock(this);
	CAutoLock cRenderLock(&m_RenderLock);
	DeleteSurfaces();
	m_NativeVideoSize = m_AspectRatio = CSize(pBitmapInfo->biWidth, abs(pBitmapInfo->biHeight));
	if(FAILED(AllocSurfaces())) return E_FAIL;
	return PNR_NOTIMPL;
}

STDMETHODIMP CRM9AllocatorPresenter::OptimizedBlt(UCHAR* pImageBits, REF(PNxRect) rDestRect, REF(PNxRect) rSrcRect)
{
	return PNR_NOTIMPL;
}

STDMETHODIMP CRM9AllocatorPresenter::EndOptimizedBlt()
{
	return PNR_NOTIMPL;
}

STDMETHODIMP CRM9AllocatorPresenter::GetOptimizedFormat(REF(RMA_COMPRESSION_TYPE) ulType)
{
	return PNR_NOTIMPL;
}

STDMETHODIMP CRM9AllocatorPresenter::GetPreferredFormat(REF(RMA_COMPRESSION_TYPE) ulType)
{
	ulType = RMA_I420;
	return PNR_OK;
}

//
// CQT9AllocatorPresenter
//

CQT9AllocatorPresenter::CQT9AllocatorPresenter(HWND hWnd, HRESULT& hr, CString &_Error) 
	: CDX9AllocatorPresenter(hWnd, hr, false, _Error)
{
}

STDMETHODIMP CQT9AllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return 
		QI(IQTVideoSurface)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CQT9AllocatorPresenter::AllocSurfaces()
{
	HRESULT hr;

	m_pVideoSurfaceOff = NULL;

	if(FAILED(hr = m_pD3DDev->CreateOffscreenPlainSurface(
		m_NativeVideoSize.cx, m_NativeVideoSize.cy, D3DFMT_X8R8G8B8, 
		D3DPOOL_DEFAULT, &m_pVideoSurfaceOff, NULL)))
		return hr;

	return __super::AllocSurfaces();
}

void CQT9AllocatorPresenter::DeleteSurfaces()
{
	m_pVideoSurfaceOff = NULL;

	__super::DeleteSurfaces();
}

// IQTVideoSurface

STDMETHODIMP CQT9AllocatorPresenter::BeginBlt(const BITMAP& bm)
{
    CAutoLock cAutoLock(this);
	CAutoLock cRenderLock(&m_RenderLock);
	DeleteSurfaces();
	m_NativeVideoSize = m_AspectRatio = CSize(bm.bmWidth, abs(bm.bmHeight));
	if(FAILED(AllocSurfaces())) return E_FAIL;
	return S_OK;
}

STDMETHODIMP CQT9AllocatorPresenter::DoBlt(const BITMAP& bm)
{
	if(!m_pVideoSurface || !m_pVideoSurfaceOff)
		return E_FAIL;

	bool fOk = false;

	D3DSURFACE_DESC d3dsd;
	ZeroMemory(&d3dsd, sizeof(d3dsd));
	if(FAILED(m_pVideoSurfaceOff->GetDesc(&d3dsd)))
		return E_FAIL;

	int w = bm.bmWidth;
	int h = abs(bm.bmHeight);
	int bpp = bm.bmBitsPixel;
	int dbpp = 
		d3dsd.Format == D3DFMT_R8G8B8 || d3dsd.Format == D3DFMT_X8R8G8B8 || d3dsd.Format == D3DFMT_A8R8G8B8 ? 32 : 
		d3dsd.Format == D3DFMT_R5G6B5 ? 16 : 0;

	if((bpp == 16 || bpp == 24 || bpp == 32) && w == d3dsd.Width && h == d3dsd.Height)
	{
		D3DLOCKED_RECT r;
		if(SUCCEEDED(m_pVideoSurfaceOff->LockRect(&r, NULL, 0)))
		{
			BitBltFromRGBToRGB(
				w, h,
				(BYTE*)r.pBits, r.Pitch, dbpp,
				(BYTE*)bm.bmBits, bm.bmWidthBytes, bm.bmBitsPixel);
			m_pVideoSurfaceOff->UnlockRect();
			fOk = true;
		}
	}

	if(!fOk)
	{
		m_pD3DDev->ColorFill(m_pVideoSurfaceOff, NULL, 0);

		HDC hDC;
		if(SUCCEEDED(m_pVideoSurfaceOff->GetDC(&hDC)))
		{
			CString str;
			str.Format(_T("Sorry, this color format is not supported"));

			SetBkColor(hDC, 0);
			SetTextColor(hDC, 0x404040);
			TextOut(hDC, 10, 10, str, str.GetLength());

			m_pVideoSurfaceOff->ReleaseDC(hDC);
		}
	}

	m_pD3DDev->StretchRect(m_pVideoSurfaceOff, NULL, m_pVideoSurface[m_nCurSurface], NULL, D3DTEXF_NONE);

	Paint(true);

	return S_OK;
}

//
// CDXRAllocatorPresenter
//

CDXRAllocatorPresenter::CDXRAllocatorPresenter(HWND hWnd, HRESULT& hr, CString &_Error)
	: ISubPicAllocatorPresenterImpl(hWnd, hr, &_Error)
	, m_ScreenSize(0, 0)
{
	if(FAILED(hr))
	{
		_Error += L"ISubPicAllocatorPresenterImpl failed\n";
		return;
	}

	hr = S_OK;
}

CDXRAllocatorPresenter::~CDXRAllocatorPresenter()
{
	if(m_pSRCB)
	{
		// nasty, but we have to let it know about our death somehow
		((CSubRenderCallback*)(ISubRenderCallback*)m_pSRCB)->SetDXRAP(NULL);
	}

	// the order is important here
	m_pSubPicQueue = NULL;
	m_pAllocator = NULL;
	m_pDXR = NULL;
}

STDMETHODIMP CDXRAllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
/*
	if(riid == __uuidof(IVideoWindow))
		return GetInterface((IVideoWindow*)this, ppv);
	if(riid == __uuidof(IBasicVideo))
		return GetInterface((IBasicVideo*)this, ppv);
	if(riid == __uuidof(IBasicVideo2))
		return GetInterface((IBasicVideo2*)this, ppv);
*/
/*
	if(riid == __uuidof(IVMRWindowlessControl))
		return GetInterface((IVMRWindowlessControl*)this, ppv);
*/

	if(riid != IID_IUnknown && m_pDXR)
	{
		if(SUCCEEDED(m_pDXR->QueryInterface(riid, ppv)))
			return S_OK;
	}

	return __super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CDXRAllocatorPresenter::SetDevice(IDirect3DDevice9* pD3DDev)
{
	CheckPointer(pD3DDev, E_POINTER);

	CSize size;
	switch(AfxGetAppSettings().nSPCMaxRes)
	{
	case 0: default: size = m_ScreenSize; break;
	case 1: size.SetSize(1024, 768); break;
	case 2: size.SetSize(800, 600); break;
	case 3: size.SetSize(640, 480); break;
	case 4: size.SetSize(512, 384); break;
	case 5: size.SetSize(384, 288); break;
	case 6: size.SetSize(2560, 1600); break;
	case 7: size.SetSize(1920, 1080); break;
	case 8: size.SetSize(1320, 900); break;
	case 9: size.SetSize(1280, 720); break;
	}

	if(m_pAllocator)
	{
		m_pAllocator->ChangeDevice(pD3DDev);
	}
	else
	{
		m_pAllocator = DNew CDX9SubPicAllocator(pD3DDev, size, AfxGetAppSettings().fSPCPow2Tex);
		if(!m_pAllocator)
			return E_FAIL;
	}

	HRESULT hr = S_OK;

	m_pSubPicQueue = AfxGetAppSettings().nSPCSize > 0 
		? (ISubPicQueue*)DNew CSubPicQueue(AfxGetAppSettings().nSPCSize, AfxGetAppSettings().fSPCDisableAnim, m_pAllocator, &hr)
		: (ISubPicQueue*)DNew CSubPicQueueNoThread(m_pAllocator, &hr);
	if(!m_pSubPicQueue || FAILED(hr))
		return E_FAIL;

	if(m_SubPicProvider) m_pSubPicQueue->SetSubPicProvider(m_SubPicProvider);

	return S_OK;
}

HRESULT CDXRAllocatorPresenter::Render(
	REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, REFERENCE_TIME atpf,
	int left, int top, int right, int bottom, int width, int height)
{	
	__super::SetPosition(CRect(0, 0, width, height), CRect(left, top, right, bottom)); // needed? should be already set by the player
	SetTime(rtStart);
	if(atpf > 0 && m_pSubPicQueue) m_pSubPicQueue->SetFPS(10000000.0 / atpf);
	AlphaBltSubPic(CSize(width, height));
	return S_OK;
}

// ISubPicAllocatorPresenter

STDMETHODIMP CDXRAllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
    CheckPointer(ppRenderer, E_POINTER);

	if(m_pDXR) return E_UNEXPECTED;
	m_pDXR.CoCreateInstance(CLSID_DXR, GetOwner());
	if(!m_pDXR) return E_FAIL;

	CComQIPtr<ISubRender> pSR = m_pDXR;
	if(!pSR) {m_pDXR = NULL; return E_FAIL;}

	m_pSRCB = DNew CSubRenderCallback(this);
	if(FAILED(pSR->SetCallback(m_pSRCB))) {m_pDXR = NULL; return E_FAIL;}

	(*ppRenderer = this)->AddRef();

	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	if (GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi))
		m_ScreenSize.SetSize(mi.rcMonitor.right-mi.rcMonitor.left, mi.rcMonitor.bottom-mi.rcMonitor.top);

	return S_OK;
}

STDMETHODIMP_(void) CDXRAllocatorPresenter::SetPosition(RECT w, RECT v)
{
	if(CComQIPtr<IBasicVideo> pBV = m_pDXR)
	{
		pBV->SetDefaultSourcePosition();
		pBV->SetDestinationPosition(v.left, v.top, v.right - v.left, v.bottom - v.top);
	}

	if(CComQIPtr<IVideoWindow> pVW = m_pDXR)
	{
		pVW->SetWindowPosition(w.left, w.top, w.right - w.left, w.bottom - w.top);
	}
}

STDMETHODIMP_(SIZE) CDXRAllocatorPresenter::GetVideoSize(bool fCorrectAR)
{
	SIZE size = {0, 0};

	if(!fCorrectAR)
	{
		if(CComQIPtr<IBasicVideo> pBV = m_pDXR)
			pBV->GetVideoSize(&size.cx, &size.cy);
	}
	else
	{
		if(CComQIPtr<IBasicVideo2> pBV2 = m_pDXR)
			pBV2->GetPreferredAspectRatio(&size.cx, &size.cy);
	}

	return size;
}

STDMETHODIMP_(bool) CDXRAllocatorPresenter::Paint(bool fAll)
{
	return false; // TODO
}

STDMETHODIMP CDXRAllocatorPresenter::GetDIB(BYTE* lpDib, DWORD* size)
{
	HRESULT hr = E_NOTIMPL;
	if(CComQIPtr<IBasicVideo> pBV = m_pDXR)
		hr = pBV->GetCurrentImage((long*)size, (long*)lpDib);
	return hr;
}

STDMETHODIMP CDXRAllocatorPresenter::SetPixelShader(LPCSTR pSrcData, LPCSTR pTarget)
{
	return E_NOTIMPL; // TODO
}
