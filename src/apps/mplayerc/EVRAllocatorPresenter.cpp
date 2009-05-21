/*
 * $Id$
 *
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
#include "..\..\SubPic\ISubPic.h"
#include "EVRAllocatorPresenter.h"
#include <d3d9.h>
#include <d3dx9.h>
#include <Vmr9.h>
#include <evr.h>
#include <mfapi.h>	// API Media Foundation
#include <Mferror.h>
#include "..\..\SubPic\DX9SubPic.h"
#include "IQTVideoSurface.h"
#include <moreuuids.h>

#include "MacrovisionKicker.h"
#include "IPinHook.h"

#include "PixelShaderCompiler.h"
#include "MainFrm.h"

#include "AllocatorCommon.h"

#if (0)		// Set to 1 to activate EVR traces
	#define TRACE_EVR		TRACE
	#define TRACE_EVR2		TRACE
#else
	#define TRACE_EVR
	#define TRACE_EVR2		TRACE
#endif

typedef enum 
{
	MSG_MIXERIN,
	MSG_MIXEROUT
} EVR_STATS_MSG;


// dxva.dll
typedef HRESULT (__stdcall *PTR_DXVA2CreateDirect3DDeviceManager9)(UINT* pResetToken, IDirect3DDeviceManager9** ppDeviceManager);

// mf.dll
typedef HRESULT (__stdcall *PTR_MFCreatePresentationClock)(IMFPresentationClock** ppPresentationClock);

// evr.dll
typedef HRESULT (__stdcall *PTR_MFCreateDXSurfaceBuffer)(REFIID riid, IUnknown* punkSurface, BOOL fBottomUpWhenLinear, IMFMediaBuffer** ppBuffer);
typedef HRESULT (__stdcall *PTR_MFCreateVideoSampleFromSurface)(IUnknown* pUnkSurface, IMFSample** ppSample);
typedef HRESULT (__stdcall *PTR_MFCreateVideoMediaType)(const MFVIDEOFORMAT* pVideoFormat, IMFVideoMediaType** ppIVideoMediaType);

// AVRT.dll
typedef HANDLE  (__stdcall *PTR_AvSetMmThreadCharacteristicsW)(LPCWSTR TaskName, LPDWORD TaskIndex);
typedef BOOL	(__stdcall *PTR_AvSetMmThreadPriority)(HANDLE AvrtHandle, AVRT_PRIORITY Priority);
typedef BOOL	(__stdcall *PTR_AvRevertMmThreadCharacteristics)(HANDLE AvrtHandle);


// Guid to tag IMFSample with DirectX surface index
static const GUID GUID_SURFACE_INDEX = { 0x30c8e9f6, 0x415, 0x4b81, { 0xa3, 0x15, 0x1, 0xa, 0xc6, 0xa9, 0xda, 0x19 } };


// === Helper functions
#define CheckHR(exp) {if(FAILED(hr = exp)) return hr;}

MFOffset MakeOffset(float v)
{
    MFOffset offset;
    offset.value = short(v);
    offset.fract = WORD(65536 * (v-offset.value));
    return offset;
}

MFVideoArea MakeArea(float x, float y, DWORD width, DWORD height)
{
    MFVideoArea area;
    area.OffsetX = MakeOffset(x);
    area.OffsetY = MakeOffset(y);
    area.Area.cx = width;
    area.Area.cy = height;
    return area;
}


/// === Outer EVR

class CEVRAllocatorPresenter;

class COuterEVR
	: public CUnknown
	, public IVMRffdshow9
	, public IVMRMixerBitmap9
	, public IBaseFilter
{
	CComPtr<IUnknown>	m_pEVR;
	VMR9AlphaBitmap*	m_pVMR9AlphaBitmap;
	CEVRAllocatorPresenter *m_pAllocatorPresenter;

public:

	// IBaseFilter
    virtual HRESULT STDMETHODCALLTYPE EnumPins(__out  IEnumPins **ppEnum)
	{
		CComPtr<IBaseFilter> pEVRBase;
		if (m_pEVR)
			m_pEVR->QueryInterface(&pEVRBase);
		if (pEVRBase)
			return pEVRBase->EnumPins(ppEnum);
		return E_NOTIMPL;
	}
    
    virtual HRESULT STDMETHODCALLTYPE FindPin(LPCWSTR Id, __out  IPin **ppPin)
	{
		CComPtr<IBaseFilter> pEVRBase;
		if (m_pEVR)
			m_pEVR->QueryInterface(&pEVRBase);
		if (pEVRBase)
			return pEVRBase->FindPin(Id, ppPin);
		return E_NOTIMPL;
	}
    
    virtual HRESULT STDMETHODCALLTYPE QueryFilterInfo(__out  FILTER_INFO *pInfo)
	{
		CComPtr<IBaseFilter> pEVRBase;
		if (m_pEVR)
			m_pEVR->QueryInterface(&pEVRBase);
		if (pEVRBase)
			return pEVRBase->QueryFilterInfo(pInfo);
		return E_NOTIMPL;
	}
    
    virtual HRESULT STDMETHODCALLTYPE JoinFilterGraph(__in_opt  IFilterGraph *pGraph, __in_opt  LPCWSTR pName)
	{
				CComPtr<IBaseFilter> pEVRBase;
		if (m_pEVR)
			m_pEVR->QueryInterface(&pEVRBase);
		if (pEVRBase)
			return pEVRBase->JoinFilterGraph(pGraph, pName);
		return E_NOTIMPL;
	}
    
    virtual HRESULT STDMETHODCALLTYPE QueryVendorInfo(__out  LPWSTR *pVendorInfo)
	{
		CComPtr<IBaseFilter> pEVRBase;
		if (m_pEVR)
			m_pEVR->QueryInterface(&pEVRBase);
		if (pEVRBase)
			return pEVRBase->QueryVendorInfo(pVendorInfo);
		return E_NOTIMPL;
	}

    virtual HRESULT STDMETHODCALLTYPE Stop( void)
	{
		CComPtr<IBaseFilter> pEVRBase;
		if (m_pEVR)
			m_pEVR->QueryInterface(&pEVRBase);
		if (pEVRBase)
			return pEVRBase->Stop();
		return E_NOTIMPL;
	}
    
    virtual HRESULT STDMETHODCALLTYPE Pause( void)
	{
		CComPtr<IBaseFilter> pEVRBase;
		if (m_pEVR)
			m_pEVR->QueryInterface(&pEVRBase);
		if (pEVRBase)
			return pEVRBase->Pause();
		return E_NOTIMPL;
	}
    
    virtual HRESULT STDMETHODCALLTYPE Run( REFERENCE_TIME tStart)
	{
		CComPtr<IBaseFilter> pEVRBase;
		if (m_pEVR)
			m_pEVR->QueryInterface(&pEVRBase);
		if (pEVRBase)
			return pEVRBase->Run(tStart);
		return E_NOTIMPL;
	}
    
    virtual HRESULT STDMETHODCALLTYPE GetState( DWORD dwMilliSecsTimeout, __out  FILTER_STATE *State);
    
    virtual HRESULT STDMETHODCALLTYPE SetSyncSource(__in_opt  IReferenceClock *pClock)
	{
		CComPtr<IBaseFilter> pEVRBase;
		if (m_pEVR)
			m_pEVR->QueryInterface(&pEVRBase);
		if (pEVRBase)
			return pEVRBase->SetSyncSource(pClock);
		return E_NOTIMPL;
	}
    
    virtual HRESULT STDMETHODCALLTYPE GetSyncSource(__deref_out_opt  IReferenceClock **pClock)
	{
		CComPtr<IBaseFilter> pEVRBase;
		if (m_pEVR)
			m_pEVR->QueryInterface(&pEVRBase);
		if (pEVRBase)
			return pEVRBase->GetSyncSource(pClock);
		return E_NOTIMPL;
	}

    virtual HRESULT STDMETHODCALLTYPE GetClassID(__RPC__out CLSID *pClassID)
	{
		CComPtr<IBaseFilter> pEVRBase;
		if (m_pEVR)
			m_pEVR->QueryInterface(&pEVRBase);
		if (pEVRBase)
			return pEVRBase->GetClassID(pClassID);
		return E_NOTIMPL;
	}

	COuterEVR(const TCHAR* pName, LPUNKNOWN pUnk, HRESULT& hr, VMR9AlphaBitmap* pVMR9AlphaBitmap, CEVRAllocatorPresenter *pAllocatorPresenter) : CUnknown(pName, pUnk)
	{
		hr = m_pEVR.CoCreateInstance(CLSID_EnhancedVideoRenderer, GetOwner());
		m_pVMR9AlphaBitmap = pVMR9AlphaBitmap;
		m_pAllocatorPresenter = pAllocatorPresenter;


	}

	~COuterEVR();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
	{
		HRESULT hr;

		if(riid == __uuidof(IVMRMixerBitmap9))
			return GetInterface((IVMRMixerBitmap9*)this, ppv);

		if (riid == __uuidof(IBaseFilter))
		{
			return GetInterface((IBaseFilter*)this, ppv);
		}

		if (riid == __uuidof(IMediaFilter))
		{
			return GetInterface((IMediaFilter*)this, ppv);
		}
		if (riid == __uuidof(IPersist))
		{
			return GetInterface((IPersist*)this, ppv);
		}
		if (riid == __uuidof(IBaseFilter))
		{
			return GetInterface((IBaseFilter*)this, ppv);
		}

		hr = m_pEVR ? m_pEVR->QueryInterface(riid, ppv) : E_NOINTERFACE;
		if(m_pEVR && FAILED(hr))
		{
			if(riid == __uuidof(IVMRffdshow9)) // Support ffdshow queueing. We show ffdshow that this is patched Media Player Classic.
				return GetInterface((IVMRffdshow9*)this, ppv);
		}

		return SUCCEEDED(hr) ? hr : __super::NonDelegatingQueryInterface(riid, ppv);
	}

	// IVMRffdshow9
	STDMETHODIMP support_ffdshow()
	{
		queueu_ffdshow_support = true;
		return S_OK;
	}

	// IVMRMixerBitmap9
	STDMETHODIMP GetAlphaBitmapParameters(VMR9AlphaBitmap* pBmpParms);
	
	STDMETHODIMP SetAlphaBitmap(const VMR9AlphaBitmap*  pBmpParms);

	STDMETHODIMP UpdateAlphaBitmapParameters(const VMR9AlphaBitmap* pBmpParms);
};







class CEVRAllocatorPresenter : 
	public CDX9AllocatorPresenter,
	public IMFGetService,
	public IMFTopologyServiceLookupClient,
	public IMFVideoDeviceID,
	public IMFVideoPresenter,
	public IDirect3DDeviceManager9,

	public IMFAsyncCallback,
	public IQualProp,
	public IMFRateSupport,				
	public IMFVideoDisplayControl,
	public IEVRTrustedVideoPlugin
/*	public IMFVideoPositionMapper,		// Non mandatory EVR Presenter Interfaces (see later...)
*/
{
public:
	CEVRAllocatorPresenter(HWND hWnd, HRESULT& hr, CString &_Error);
	~CEVRAllocatorPresenter(void);

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	STDMETHODIMP	CreateRenderer(IUnknown** ppRenderer);
	STDMETHODIMP_(bool) Paint(bool fAll);
	STDMETHODIMP	GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight);
	STDMETHODIMP	InitializeDevice(AM_MEDIA_TYPE*	pMediaType);


	// IMFClockStateSink
	STDMETHODIMP	OnClockStart(/* [in] */ MFTIME hnsSystemTime, /* [in] */ LONGLONG llClockStartOffset);        
	STDMETHODIMP	STDMETHODCALLTYPE OnClockStop(/* [in] */ MFTIME hnsSystemTime);
	STDMETHODIMP	STDMETHODCALLTYPE OnClockPause(/* [in] */ MFTIME hnsSystemTime);
	STDMETHODIMP	STDMETHODCALLTYPE OnClockRestart(/* [in] */ MFTIME hnsSystemTime);
	STDMETHODIMP	STDMETHODCALLTYPE OnClockSetRate(/* [in] */ MFTIME hnsSystemTime, /* [in] */ float flRate);

	// IBaseFilter delegate
    bool			GetState( DWORD dwMilliSecsTimeout, FILTER_STATE *State, HRESULT &_ReturnValue);

	// IQualProp (EVR statistics window)
    STDMETHODIMP	get_FramesDroppedInRenderer		(int *pcFrames);
    STDMETHODIMP	get_FramesDrawn					(int *pcFramesDrawn);
    STDMETHODIMP	get_AvgFrameRate				(int *piAvgFrameRate);
    STDMETHODIMP	get_Jitter						(int *iJitter);
    STDMETHODIMP	get_AvgSyncOffset				(int *piAvg);
    STDMETHODIMP	get_DevSyncOffset				(int *piDev);


	// IMFRateSupport
    STDMETHODIMP	GetSlowestRate(MFRATE_DIRECTION eDirection, BOOL fThin, float *pflRate);
    STDMETHODIMP	GetFastestRate(MFRATE_DIRECTION eDirection, BOOL fThin, float *pflRate);
    STDMETHODIMP	IsRateSupported(BOOL fThin, float flRate, float *pflNearestSupportedRate);

	float			GetMaxRate(BOOL bThin);


	// IMFVideoPresenter
	STDMETHODIMP	ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam);
	STDMETHODIMP	GetCurrentMediaType(__deref_out  IMFVideoMediaType **ppMediaType);

	// IMFTopologyServiceLookupClient        
	STDMETHODIMP	InitServicePointers(/* [in] */ __in  IMFTopologyServiceLookup *pLookup);
	STDMETHODIMP	ReleaseServicePointers();

	// IMFVideoDeviceID
	STDMETHODIMP	GetDeviceID(/* [out] */	__out  IID *pDeviceID);

	// IMFGetService
	STDMETHODIMP	GetService (/* [in] */ __RPC__in REFGUID guidService,
								/* [in] */ __RPC__in REFIID riid,
								/* [iid_is][out] */ __RPC__deref_out_opt LPVOID *ppvObject);

	// IMFAsyncCallback
	STDMETHODIMP	GetParameters(	/* [out] */ __RPC__out DWORD *pdwFlags, /* [out] */ __RPC__out DWORD *pdwQueue);
	STDMETHODIMP	Invoke		 (	/* [in] */ __RPC__in_opt IMFAsyncResult *pAsyncResult);

	// IMFVideoDisplayControl
    STDMETHODIMP GetNativeVideoSize(SIZE *pszVideo, SIZE *pszARVideo);    
    STDMETHODIMP GetIdealVideoSize(SIZE *pszMin, SIZE *pszMax);
    STDMETHODIMP SetVideoPosition(const MFVideoNormalizedRect *pnrcSource, const LPRECT prcDest);
    STDMETHODIMP GetVideoPosition(MFVideoNormalizedRect *pnrcSource, LPRECT prcDest);
    STDMETHODIMP SetAspectRatioMode(DWORD dwAspectRatioMode);
    STDMETHODIMP GetAspectRatioMode(DWORD *pdwAspectRatioMode);
    STDMETHODIMP SetVideoWindow(HWND hwndVideo);
    STDMETHODIMP GetVideoWindow(HWND *phwndVideo);
    STDMETHODIMP RepaintVideo( void);
    STDMETHODIMP GetCurrentImage(BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib, LONGLONG *pTimeStamp);
    STDMETHODIMP SetBorderColor(COLORREF Clr);
    STDMETHODIMP GetBorderColor(COLORREF *pClr);
    STDMETHODIMP SetRenderingPrefs(DWORD dwRenderFlags);
    STDMETHODIMP GetRenderingPrefs(DWORD *pdwRenderFlags);
    STDMETHODIMP SetFullscreen(BOOL fFullscreen);
    STDMETHODIMP GetFullscreen(BOOL *pfFullscreen);

	// IEVRTrustedVideoPlugin
    STDMETHODIMP IsInTrustedVideoMode(BOOL *pYes);
    STDMETHODIMP CanConstrict(BOOL *pYes);
    STDMETHODIMP SetConstriction(DWORD dwKPix);
    STDMETHODIMP DisableImageExport(BOOL bDisable);

	// IDirect3DDeviceManager9
	STDMETHODIMP	ResetDevice(IDirect3DDevice9 *pDevice,UINT resetToken);        
	STDMETHODIMP	OpenDeviceHandle(HANDLE *phDevice);
	STDMETHODIMP	CloseDeviceHandle(HANDLE hDevice);        
    STDMETHODIMP	TestDevice(HANDLE hDevice);
	STDMETHODIMP	LockDevice(HANDLE hDevice, IDirect3DDevice9 **ppDevice, BOOL fBlock);
	STDMETHODIMP	UnlockDevice(HANDLE hDevice, BOOL fSaveState);
	STDMETHODIMP	GetVideoService(HANDLE hDevice, REFIID riid, void **ppService);

protected :
	void			OnResetDevice();
	virtual void OnVBlankFinished(bool fAll, LONGLONG PerformanceCounter);
	
	double m_ModeratedTime;
	LONGLONG m_ModeratedTimeLast;
	LONGLONG m_ModeratedClockLast;
	LONGLONG m_ModeratedTimer;
	MFCLOCK_STATE m_LastClockState;
	LONGLONG GetClockTime(LONGLONG PerformanceCounter);

private :

	typedef enum
	{
		Started = State_Running,
		Stopped = State_Stopped,
		Paused = State_Paused,
		Shutdown = State_Running + 1
	} RENDER_STATE;

	COuterEVR *m_pOuterEVR;
	CComPtr<IMFClock>						m_pClock;
	CComPtr<IDirect3DDeviceManager9>		m_pD3DManager;
	CComPtr<IMFTransform>					m_pMixer;
	CComPtr<IMediaEventSink>				m_pSink;
	CComPtr<IMFVideoMediaType>				m_pMediaType;
	MFVideoAspectRatioMode					m_dwVideoAspectRatioMode;
	MFVideoRenderPrefs						m_dwVideoRenderPrefs;
	COLORREF								m_BorderColor;


	HANDLE									m_hEvtQuit;			// Stop rendering thread event
	bool									m_bEvtQuit;
	HANDLE									m_hEvtFlush;		// Discard all buffers
	bool									m_bEvtFlush;

	bool									m_fUseInternalTimer;
	int32									m_LastSetOutputRange;
	bool									m_bPendingRenegotiate;
	bool									m_bPendingMediaFinished;

	HANDLE									m_hThread;
	HANDLE									m_hGetMixerThread;
	RENDER_STATE							m_nRenderState;
	
	CCritSec								m_SampleQueueLock;
	CCritSec								m_ImageProcessingLock;

	CInterfaceList<IMFSample, &IID_IMFSample>		m_FreeSamples;
	CInterfaceList<IMFSample, &IID_IMFSample>		m_ScheduledSamples;
	IMFSample *								m_pCurrentDisplaydSample;
	bool									m_bWaitingSample;
	bool									m_bLastSampleOffsetValid;
	LONGLONG								m_LastScheduledSampleTime;
	double									m_LastScheduledSampleTimeFP;
	LONGLONG								m_LastScheduledUncorrectedSampleTime;
	LONGLONG								m_MaxSampleDuration;
	LONGLONG								m_LastSampleOffset;
	LONGLONG								m_VSyncOffsetHistory[5];
	LONGLONG								m_LastPredictedSync;
	int										m_VSyncOffsetHistoryPos;

	UINT									m_nResetToken;
	int										m_nStepCount;

	bool									m_bSignaledStarvation; 
	LONGLONG								m_StarvationClock;

	// Stats variable for IQualProp
	UINT									m_pcFrames;
	UINT									m_nDroppedUpdate;
	UINT									m_pcFramesDrawn;	// Retrieves the number of frames drawn since streaming started
	UINT									m_piAvg;
	UINT									m_piDev;


	void									GetMixerThread();
	static DWORD WINAPI						GetMixerThreadStatic(LPVOID lpParam);

	bool									GetImageFromMixer();
	void									RenderThread();
	static DWORD WINAPI						PresentThread(LPVOID lpParam);
	void									ResetStats();
	void									StartWorkerThreads();
	void									StopWorkerThreads();
	HRESULT									CheckShutdown() const;
	void									CompleteFrameStep(bool bCancel);
	void									CheckWaitingSampleFromMixer();

	void									RemoveAllSamples();
	HRESULT									GetFreeSample(IMFSample** ppSample);
	HRESULT									GetScheduledSample(IMFSample** ppSample, int &_Count);
	void									MoveToFreeList(IMFSample* pSample, bool bTail);
	void									MoveToScheduledList(IMFSample* pSample, bool _bSorted);
	void									FlushSamples();
	void									FlushSamplesInternal();

	// === Media type negociation functions
	HRESULT									RenegotiateMediaType();
	HRESULT									IsMediaTypeSupported(IMFMediaType* pMixerType);
	HRESULT									CreateProposedOutputType(IMFMediaType* pMixerType, IMFMediaType** pType);
	HRESULT									SetMediaType(IMFMediaType* pType);

	// === Functions pointers on Vista / .Net3 specifics library
	PTR_DXVA2CreateDirect3DDeviceManager9	pfDXVA2CreateDirect3DDeviceManager9;
	PTR_MFCreateDXSurfaceBuffer				pfMFCreateDXSurfaceBuffer;
	PTR_MFCreateVideoSampleFromSurface		pfMFCreateVideoSampleFromSurface;
	PTR_MFCreateVideoMediaType				pfMFCreateVideoMediaType;

#if 0
	HRESULT (__stdcall *pMFCreateMediaType)(__deref_out IMFMediaType**  ppMFType);
	HRESULT (__stdcall *pMFInitMediaTypeFromAMMediaType)(__in IMFMediaType *pMFType, __in const AM_MEDIA_TYPE *pAMType);
	HRESULT (__stdcall *pMFInitAMMediaTypeFromMFMediaType)(__in IMFMediaType *pMFType, __in GUID guidFormatBlockType, __inout AM_MEDIA_TYPE *pAMType);
#endif
											
	PTR_AvSetMmThreadCharacteristicsW		pfAvSetMmThreadCharacteristicsW;
	PTR_AvSetMmThreadPriority				pfAvSetMmThreadPriority;
	PTR_AvRevertMmThreadCharacteristics		pfAvRevertMmThreadCharacteristics;
};


HRESULT STDMETHODCALLTYPE COuterEVR::GetState( DWORD dwMilliSecsTimeout, __out  FILTER_STATE *State)
{
	HRESULT ReturnValue;
	if (m_pAllocatorPresenter->GetState(dwMilliSecsTimeout, State, ReturnValue))
		return ReturnValue;
	CComPtr<IBaseFilter> pEVRBase;
	if (m_pEVR)
		m_pEVR->QueryInterface(&pEVRBase);
	if (pEVRBase)
		return pEVRBase->GetState(dwMilliSecsTimeout, State);
	return E_NOTIMPL;
}

STDMETHODIMP COuterEVR::GetAlphaBitmapParameters(VMR9AlphaBitmap* pBmpParms)
{
	CheckPointer(pBmpParms, E_POINTER);
	CAutoLock BitMapLock(&m_pAllocatorPresenter->m_VMR9AlphaBitmapLock);
	memcpy (pBmpParms, m_pVMR9AlphaBitmap, sizeof(VMR9AlphaBitmap));
	return S_OK;
}

STDMETHODIMP COuterEVR::SetAlphaBitmap(const VMR9AlphaBitmap*  pBmpParms)
{
	CheckPointer(pBmpParms, E_POINTER);
	CAutoLock BitMapLock(&m_pAllocatorPresenter->m_VMR9AlphaBitmapLock);
	memcpy (m_pVMR9AlphaBitmap, pBmpParms, sizeof(VMR9AlphaBitmap));
	m_pVMR9AlphaBitmap->dwFlags |= VMRBITMAP_UPDATE;
	m_pAllocatorPresenter->UpdateAlphaBitmap();
	return S_OK;
}

STDMETHODIMP COuterEVR::UpdateAlphaBitmapParameters(const VMR9AlphaBitmap* pBmpParms)
{
	CheckPointer(pBmpParms, E_POINTER);
	CAutoLock BitMapLock(&m_pAllocatorPresenter->m_VMR9AlphaBitmapLock);
	memcpy (m_pVMR9AlphaBitmap, pBmpParms, sizeof(VMR9AlphaBitmap));
	m_pVMR9AlphaBitmap->dwFlags |= VMRBITMAP_UPDATE;
	m_pAllocatorPresenter->UpdateAlphaBitmap();
	return S_OK;
}

COuterEVR::~COuterEVR()
{
}


CString GetWindowsErrorMessage(HRESULT _Error, HMODULE _Module);

HRESULT CreateEVR(const CLSID& clsid, HWND hWnd, ISubPicAllocatorPresenter** ppAP)
{
	HRESULT		hr = E_FAIL;
	if (clsid == CLSID_EVRAllocatorPresenter)
	{
		CString Error;
		*ppAP	= DNew CEVRAllocatorPresenter(hWnd, hr, Error);
		(*ppAP)->AddRef();

		if(FAILED(hr))
		{
			Error += L"\n";
			Error += GetWindowsErrorMessage(hr, NULL);
			MessageBox(hWnd, Error, L"Error creating EVR Custom renderer", MB_OK | MB_ICONERROR);
			(*ppAP)->Release();
			*ppAP = NULL;
		}
		else if (!Error.IsEmpty())
		{
			MessageBox(hWnd, Error, L"Warning creating EVR Custom renderer", MB_OK|MB_ICONWARNING);
		}
	}

	return hr;
}


CEVRAllocatorPresenter::CEVRAllocatorPresenter(HWND hWnd, HRESULT& hr, CString &_Error)
	: CDX9AllocatorPresenter(hWnd, hr, true, _Error)
{
	HMODULE		hLib;
	AppSettings& s = AfxGetAppSettings();

	m_nResetToken	 = 0;
	m_hThread		 = INVALID_HANDLE_VALUE;
	m_hGetMixerThread= INVALID_HANDLE_VALUE;
	m_hEvtFlush		 = INVALID_HANDLE_VALUE;
	m_hEvtQuit		 = INVALID_HANDLE_VALUE;
	m_bEvtQuit = 0;
	m_bEvtFlush = 0;
	m_ModeratedTime = 0;
	m_ModeratedTimeLast = -1;
	m_ModeratedClockLast = -1;

	m_bNeedPendingResetDevice = true;

	if (FAILED (hr)) 
	{
		_Error += L"DX9AllocatorPresenter failed\n";

		return;
	}

	// Load EVR specifics DLLs
	hLib = LoadLibrary (L"dxva2.dll");
	pfDXVA2CreateDirect3DDeviceManager9	= hLib ? (PTR_DXVA2CreateDirect3DDeviceManager9) GetProcAddress (hLib, "DXVA2CreateDirect3DDeviceManager9") : NULL;
	
	// Load EVR functions
	hLib = LoadLibrary (L"evr.dll");
	pfMFCreateDXSurfaceBuffer			= hLib ? (PTR_MFCreateDXSurfaceBuffer)			GetProcAddress (hLib, "MFCreateDXSurfaceBuffer") : NULL;
	pfMFCreateVideoSampleFromSurface	= hLib ? (PTR_MFCreateVideoSampleFromSurface)	GetProcAddress (hLib, "MFCreateVideoSampleFromSurface") : NULL;
	pfMFCreateVideoMediaType			= hLib ? (PTR_MFCreateVideoMediaType)			GetProcAddress (hLib, "MFCreateVideoMediaType") : NULL;

	if (!pfDXVA2CreateDirect3DDeviceManager9 || !pfMFCreateDXSurfaceBuffer || !pfMFCreateVideoSampleFromSurface || !pfMFCreateVideoMediaType)
	{
		if (!pfDXVA2CreateDirect3DDeviceManager9)
			_Error += L"Could not find DXVA2CreateDirect3DDeviceManager9 (dxva2.dll)\n";
		if (!pfMFCreateDXSurfaceBuffer)
			_Error += L"Could not find MFCreateDXSurfaceBuffer (evr.dll)\n";
		if (!pfMFCreateVideoSampleFromSurface)
			_Error += L"Could not find MFCreateVideoSampleFromSurface (evr.dll)\n";
		if (!pfMFCreateVideoMediaType)
			_Error += L"Could not find MFCreateVideoMediaType (evr.dll)\n";
		hr = E_FAIL;
		return;
	}

	// Load mfplat fuctions
#if 0
	hLib = LoadLibrary (L"mfplat.dll");
	(FARPROC &)pMFCreateMediaType = GetProcAddress(hLib, "MFCreateMediaType");
	(FARPROC &)pMFInitMediaTypeFromAMMediaType = GetProcAddress(hLib, "MFInitMediaTypeFromAMMediaType");
	(FARPROC &)pMFInitAMMediaTypeFromMFMediaType = GetProcAddress(hLib, "MFInitAMMediaTypeFromMFMediaType");

	if (!pMFCreateMediaType || !pMFInitMediaTypeFromAMMediaType || !pMFInitAMMediaTypeFromMFMediaType)
	{
		hr = E_FAIL;
		return;
	}
#endif

	// Load Vista specifics DLLs
	hLib = LoadLibrary (L"AVRT.dll");
	pfAvSetMmThreadCharacteristicsW		= hLib ? (PTR_AvSetMmThreadCharacteristicsW)	GetProcAddress (hLib, "AvSetMmThreadCharacteristicsW") : NULL;
	pfAvSetMmThreadPriority				= hLib ? (PTR_AvSetMmThreadPriority)			GetProcAddress (hLib, "AvSetMmThreadPriority") : NULL;
	pfAvRevertMmThreadCharacteristics	= hLib ? (PTR_AvRevertMmThreadCharacteristics)	GetProcAddress (hLib, "AvRevertMmThreadCharacteristics") : NULL;

	// Init DXVA manager
	hr = pfDXVA2CreateDirect3DDeviceManager9(&m_nResetToken, &m_pD3DManager);
	if (SUCCEEDED (hr)) 
	{
		hr = m_pD3DManager->ResetDevice(m_pD3DDev, m_nResetToken);
		if (!SUCCEEDED (hr)) 
		{
			_Error += L"m_pD3DManager->ResetDevice failed\n";
		}
	}
	else
		_Error += L"DXVA2CreateDirect3DDeviceManager9 failed\n";

	CComPtr<IDirectXVideoDecoderService>	pDecoderService;
	HANDLE							hDevice;
	if (SUCCEEDED (m_pD3DManager->OpenDeviceHandle(&hDevice)) &&
		SUCCEEDED (m_pD3DManager->GetVideoService (hDevice, __uuidof(IDirectXVideoDecoderService), (void**)&pDecoderService)))
	{
		TRACE_EVR ("DXVA2 : device handle = 0x%08x", hDevice);
		HookDirectXVideoDecoderService (pDecoderService);

		m_pD3DManager->CloseDeviceHandle (hDevice);
	}


	// Bufferize frame only with 3D texture!
	if (s.iAPSurfaceUsage == VIDRNDT_AP_TEXTURE3D)
		m_nNbDXSurface	= max (min (s.iEvrBuffers, MAX_PICTURE_SLOTS-2), 4);
	else
		m_nNbDXSurface = 1;

	ResetStats();
	m_nRenderState				= Shutdown;
	m_fUseInternalTimer			= false;
	m_LastSetOutputRange		= -1;
	m_bPendingRenegotiate		= false;
	m_bPendingMediaFinished		= false;
	m_bWaitingSample			= false;
	m_pCurrentDisplaydSample	= NULL;
	m_nStepCount				= 0;
	m_dwVideoAspectRatioMode	= MFVideoARMode_PreservePicture;
	m_dwVideoRenderPrefs		= (MFVideoRenderPrefs)0;
	m_BorderColor				= RGB (0,0,0);
	m_bSignaledStarvation		= false;
	m_StarvationClock			= 0;
	m_pOuterEVR					= NULL;
	m_LastScheduledSampleTime	= -1;
	m_LastScheduledUncorrectedSampleTime = -1;
	m_MaxSampleDuration			= 0;
	m_LastSampleOffset			= 0;
	ZeroMemory(m_VSyncOffsetHistory, sizeof(m_VSyncOffsetHistory));
	m_VSyncOffsetHistoryPos = 0;
	m_bLastSampleOffsetValid	= false;
}

CEVRAllocatorPresenter::~CEVRAllocatorPresenter(void)
{
	StopWorkerThreads();	// If not already done...
	m_pMediaType	= NULL;
	m_pClock		= NULL;

	m_pD3DManager	= NULL;
}


void CEVRAllocatorPresenter::ResetStats()
{
	m_pcFrames			= 0;
	m_nDroppedUpdate	= 0;
	m_pcFramesDrawn		= 0;
	m_piAvg				= 0;
	m_piDev				= 0;
}


HRESULT CEVRAllocatorPresenter::CheckShutdown() const 
{
    if (m_nRenderState == Shutdown)
    {
        return MF_E_SHUTDOWN;
    }
    else
    {
        return S_OK;
    }
}


void CEVRAllocatorPresenter::StartWorkerThreads()
{
	DWORD		dwThreadId;

	if (m_nRenderState == Shutdown)
	{
		m_hEvtQuit		= CreateEvent (NULL, TRUE, FALSE, NULL);
		m_hEvtFlush		= CreateEvent (NULL, TRUE, FALSE, NULL);

		m_hThread		= ::CreateThread(NULL, 0, PresentThread, (LPVOID)this, 0, &dwThreadId);
		SetThreadPriority(m_hThread, THREAD_PRIORITY_TIME_CRITICAL);
		m_hGetMixerThread = ::CreateThread(NULL, 0, GetMixerThreadStatic, (LPVOID)this, 0, &dwThreadId);
		SetThreadPriority(m_hGetMixerThread, THREAD_PRIORITY_HIGHEST);

		m_nRenderState		= Stopped;
		TRACE_EVR ("Worker threads started...\n");
	}
}

void CEVRAllocatorPresenter::StopWorkerThreads()
{
	if (m_nRenderState != Shutdown)
	{
		SetEvent (m_hEvtFlush);
		m_bEvtFlush = true;
		SetEvent (m_hEvtQuit);
		m_bEvtQuit = true;
		if ((m_hThread != INVALID_HANDLE_VALUE) && (WaitForSingleObject (m_hThread, 10000) == WAIT_TIMEOUT))
		{
			ASSERT (FALSE);
			TerminateThread (m_hThread, 0xDEAD);
		}
		if ((m_hGetMixerThread != INVALID_HANDLE_VALUE) && (WaitForSingleObject (m_hGetMixerThread, 10000) == WAIT_TIMEOUT))
		{
			ASSERT (FALSE);
			TerminateThread (m_hGetMixerThread, 0xDEAD);
		}

		if (m_hThread		 != INVALID_HANDLE_VALUE) CloseHandle (m_hThread);
		if (m_hGetMixerThread		 != INVALID_HANDLE_VALUE) CloseHandle (m_hGetMixerThread);
		if (m_hEvtFlush		 != INVALID_HANDLE_VALUE) CloseHandle (m_hEvtFlush);
		if (m_hEvtQuit		 != INVALID_HANDLE_VALUE) CloseHandle (m_hEvtQuit);

		m_bEvtFlush = false;
		m_bEvtQuit = false;


		TRACE_EVR ("Worker threads stopped...\n");
	}
	m_nRenderState = Shutdown;
}


STDMETHODIMP CEVRAllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
    CheckPointer(ppRenderer, E_POINTER);

	*ppRenderer = NULL;

	HRESULT					hr = E_FAIL;

	do
	{
		CMacrovisionKicker*		pMK  = DNew CMacrovisionKicker(NAME("CMacrovisionKicker"), NULL);
		CComPtr<IUnknown>		pUnk = (IUnknown*)(INonDelegatingUnknown*)pMK;

		COuterEVR *pOuterEVR = DNew COuterEVR(NAME("COuterEVR"), pUnk, hr, &m_VMR9AlphaBitmap, this);
		m_pOuterEVR = pOuterEVR;

		pMK->SetInner((IUnknown*)(INonDelegatingUnknown*)pOuterEVR);
		CComQIPtr<IBaseFilter> pBF = pUnk;

		if (FAILED (hr)) break;

		// Set EVR custom presenter
		CComPtr<IMFVideoPresenter>		pVP;
		CComPtr<IMFVideoRenderer>		pMFVR;
		CComQIPtr<IMFGetService, &__uuidof(IMFGetService)> pMFGS = pBF;

		hr = pMFGS->GetService (MR_VIDEO_RENDER_SERVICE, IID_IMFVideoRenderer, (void**)&pMFVR);

		if(SUCCEEDED(hr)) hr = QueryInterface (__uuidof(IMFVideoPresenter), (void**)&pVP);
		if(SUCCEEDED(hr)) hr = pMFVR->InitializeRenderer (NULL, pVP);

#if 1
		CComPtr<IPin>			pPin = GetFirstPin(pBF);
		CComQIPtr<IMemInputPin> pMemInputPin = pPin;
		
		// No NewSegment : no chocolate :o)
		m_fUseInternalTimer = HookNewSegmentAndReceive((IPinC*)(IPin*)pPin, (IMemInputPinC*)(IMemInputPin*)pMemInputPin);
#else
		m_fUseInternalTimer = false;
#endif

		if(FAILED(hr))
			*ppRenderer = NULL;
		else
			*ppRenderer = pBF.Detach();

	} while (0);

	return hr;
}

STDMETHODIMP_(bool) CEVRAllocatorPresenter::Paint(bool fAll)
{
	
	return __super::Paint (fAll);
}

STDMETHODIMP CEVRAllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	HRESULT		hr;
	if(riid == __uuidof(IMFClockStateSink))
		hr = GetInterface((IMFClockStateSink*)this, ppv);
	else if(riid == __uuidof(IMFVideoPresenter))
		hr = GetInterface((IMFVideoPresenter*)this, ppv);
	else if(riid == __uuidof(IMFTopologyServiceLookupClient))
		hr = GetInterface((IMFTopologyServiceLookupClient*)this, ppv);
	else if(riid == __uuidof(IMFVideoDeviceID))
		hr = GetInterface((IMFVideoDeviceID*)this, ppv);
	else if(riid == __uuidof(IMFGetService))
		hr = GetInterface((IMFGetService*)this, ppv);
	else if(riid == __uuidof(IMFAsyncCallback))
		hr = GetInterface((IMFAsyncCallback*)this, ppv);
	else if(riid == __uuidof(IMFVideoDisplayControl))
		hr = GetInterface((IMFVideoDisplayControl*)this, ppv);
	else if(riid == __uuidof(IEVRTrustedVideoPlugin))
		hr = GetInterface((IEVRTrustedVideoPlugin*)this, ppv);
	else if(riid == IID_IQualProp)
		hr = GetInterface((IQualProp*)this, ppv);
	else if(riid == __uuidof(IMFRateSupport))
		hr = GetInterface((IMFRateSupport*)this, ppv);
	else if(riid == __uuidof(IDirect3DDeviceManager9))
//		hr = GetInterface((IDirect3DDeviceManager9*)this, ppv);
		hr = m_pD3DManager->QueryInterface (__uuidof(IDirect3DDeviceManager9), (void**) ppv);
	else
		hr = __super::NonDelegatingQueryInterface(riid, ppv);

	return hr;
}


// IMFClockStateSink
STDMETHODIMP CEVRAllocatorPresenter::OnClockStart(MFTIME hnsSystemTime,  LONGLONG llClockStartOffset)
{
	m_nRenderState		= Started;

	TRACE_EVR ("OnClockStart  hnsSystemTime = %I64d,   llClockStartOffset = %I64d\n", hnsSystemTime, llClockStartOffset);
	m_ModeratedTimeLast = -1;
	m_ModeratedClockLast = -1;

	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::OnClockStop(MFTIME hnsSystemTime)
{
	TRACE_EVR ("OnClockStop  hnsSystemTime = %I64d\n", hnsSystemTime);
	m_nRenderState		= Stopped;

	m_ModeratedClockLast = -1;
	m_ModeratedTimeLast = -1;
	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::OnClockPause(MFTIME hnsSystemTime)
{
	TRACE_EVR ("OnClockPause  hnsSystemTime = %I64d\n", hnsSystemTime);
	if (!m_bSignaledStarvation)
		m_nRenderState		= Paused;
	m_ModeratedTimeLast = -1;
	m_ModeratedClockLast = -1;
	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::OnClockRestart(MFTIME hnsSystemTime)
{
	m_nRenderState	= Started;

	m_ModeratedTimeLast = -1;
	m_ModeratedClockLast = -1;
	TRACE_EVR ("OnClockRestart  hnsSystemTime = %I64d\n", hnsSystemTime);

	return S_OK;
}


STDMETHODIMP CEVRAllocatorPresenter::OnClockSetRate(MFTIME hnsSystemTime, float flRate)
{
	ASSERT (FALSE);
	return E_NOTIMPL;
}


// IBaseFilter delegate
bool CEVRAllocatorPresenter::GetState( DWORD dwMilliSecsTimeout, FILTER_STATE *State, HRESULT &_ReturnValue)
{
	CAutoLock lock(&m_SampleQueueLock);

	if (m_bSignaledStarvation)
	{
		int nSamples = max(m_nNbDXSurface / 2, 1);
		if ((m_ScheduledSamples.GetCount() < nSamples || m_LastSampleOffset < -m_rtTimePerFrame*2) && !g_bNoDuration)
		{			
			*State = (FILTER_STATE)Paused;
			_ReturnValue = VFW_S_STATE_INTERMEDIATE;
			return true;
		}
		m_bSignaledStarvation = false;
	}
	return false;
}

// IQualProp
STDMETHODIMP CEVRAllocatorPresenter::get_FramesDroppedInRenderer(int *pcFrames)
{
	*pcFrames	= m_pcFrames;
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::get_FramesDrawn(int *pcFramesDrawn)
{
	*pcFramesDrawn = m_pcFramesDrawn;
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::get_AvgFrameRate(int *piAvgFrameRate)
{
	*piAvgFrameRate = (int)(m_fAvrFps * 100);
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::get_Jitter(int *iJitter)
{
	*iJitter = (int)((m_fJitterStdDev/10000.0) + 0.5);
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::get_AvgSyncOffset(int *piAvg)
{
	*piAvg = (int)((m_fSyncOffsetAvr/10000.0) + 0.5);
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::get_DevSyncOffset(int *piDev)
{
	*piDev = (int)((m_fSyncOffsetStdDev/10000.0) + 0.5);
	return S_OK;
}


// IMFRateSupport
STDMETHODIMP CEVRAllocatorPresenter::GetSlowestRate(MFRATE_DIRECTION eDirection, BOOL fThin, float *pflRate)
{
	// TODO : not finished...
	*pflRate = 0;
	return S_OK;
}
    
STDMETHODIMP CEVRAllocatorPresenter::GetFastestRate(MFRATE_DIRECTION eDirection, BOOL fThin, float *pflRate)
{
	HRESULT		hr = S_OK;
	float		fMaxRate = 0.0f;

	CAutoLock lock(this);

	CheckPointer(pflRate, E_POINTER);
	CheckHR(CheckShutdown());
    
	// Get the maximum forward rate.
	fMaxRate = GetMaxRate(fThin);

	// For reverse playback, swap the sign.
	if (eDirection == MFRATE_REVERSE)
		fMaxRate = -fMaxRate;

	*pflRate = fMaxRate;

	return hr;
}
    
STDMETHODIMP CEVRAllocatorPresenter::IsRateSupported(BOOL fThin, float flRate, float *pflNearestSupportedRate)
{
    // fRate can be negative for reverse playback.
    // pfNearestSupportedRate can be NULL.

    CAutoLock lock(this);

    HRESULT hr = S_OK;
    float   fMaxRate = 0.0f;
    float   fNearestRate = flRate;   // Default.

	CheckPointer (pflNearestSupportedRate, E_POINTER);
    CheckHR(hr = CheckShutdown());

    // Find the maximum forward rate.
    fMaxRate = GetMaxRate(fThin);

    if (fabsf(flRate) > fMaxRate)
    {
        // The (absolute) requested rate exceeds the maximum rate.
        hr = MF_E_UNSUPPORTED_RATE;

        // The nearest supported rate is fMaxRate.
        fNearestRate = fMaxRate;
        if (flRate < 0)
        {
            // For reverse playback, swap the sign.
            fNearestRate = -fNearestRate;
        }
    }

    // Return the nearest supported rate if the caller requested it.
    if (pflNearestSupportedRate != NULL)
        *pflNearestSupportedRate = fNearestRate;

    return hr;
}


float CEVRAllocatorPresenter::GetMaxRate(BOOL bThin)
{
	float   fMaxRate		= FLT_MAX;  // Default.
	UINT32  fpsNumerator	= 0, fpsDenominator = 0;
	UINT    MonitorRateHz	= 0; 

	if (!bThin && (m_pMediaType != NULL))
	{
		// Non-thinned: Use the frame rate and monitor refresh rate.
        
		// Frame rate:
		MFGetAttributeRatio(m_pMediaType, MF_MT_FRAME_RATE, 
			&fpsNumerator, &fpsDenominator);

		// Monitor refresh rate:
		MonitorRateHz = m_RefreshRate; // D3DDISPLAYMODE

		if (fpsDenominator && fpsNumerator && MonitorRateHz)
		{
			// Max Rate = Refresh Rate / Frame Rate
			fMaxRate = (float)MulDiv(
				MonitorRateHz, fpsDenominator, fpsNumerator);
		}
	}
	return fMaxRate;
}

void CEVRAllocatorPresenter::CompleteFrameStep(bool bCancel)
{
	if (m_nStepCount > 0)
	{
		if (bCancel || (m_nStepCount == 1)) 
		{
			m_pSink->Notify(EC_STEP_COMPLETE, bCancel ? TRUE : FALSE, 0);
			m_nStepCount = 0;
		}
		else
			m_nStepCount--;
	}
}

// IMFVideoPresenter
STDMETHODIMP CEVRAllocatorPresenter::ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam)
{
	HRESULT						hr = S_OK;

	switch (eMessage)
	{
	case MFVP_MESSAGE_BEGINSTREAMING :			// The EVR switched from stopped to paused. The presenter should allocate resources
		ResetStats();		
		TRACE_EVR ("MFVP_MESSAGE_BEGINSTREAMING\n");
		break;

	case MFVP_MESSAGE_CANCELSTEP :				// Cancels a frame step
		TRACE_EVR ("MFVP_MESSAGE_CANCELSTEP\n");
		CompleteFrameStep (true);
		break;

	case MFVP_MESSAGE_ENDOFSTREAM :				// All input streams have ended. 
		TRACE_EVR ("MFVP_MESSAGE_ENDOFSTREAM\n");
		m_bPendingMediaFinished = true;
		break;

	case MFVP_MESSAGE_ENDSTREAMING :			// The EVR switched from running or paused to stopped. The presenter should free resources
		TRACE_EVR ("MFVP_MESSAGE_ENDSTREAMING\n");
		break;

	case MFVP_MESSAGE_FLUSH :					// The presenter should discard any pending samples
		SetEvent(m_hEvtFlush);
		m_bEvtFlush = true;
		TRACE_EVR ("MFVP_MESSAGE_FLUSH\n");
		while (WaitForSingleObject(m_hEvtFlush, 1) == WAIT_OBJECT_0);
		break;

	case MFVP_MESSAGE_INVALIDATEMEDIATYPE :		// The mixer's output format has changed. The EVR will initiate format negotiation, as described previously
		/*
			1) The EVR sets the media type on the reference stream.
			2) The EVR calls IMFVideoPresenter::ProcessMessage on the presenter with the MFVP_MESSAGE_INVALIDATEMEDIATYPE message.
			3) The presenter sets the media type on the mixer's output stream.
			4) The EVR sets the media type on the substreams.
		*/
		m_bPendingRenegotiate = true;
		while (*((volatile bool *)&m_bPendingRenegotiate))
			Sleep(1);
		break;

	case MFVP_MESSAGE_PROCESSINPUTNOTIFY :		// One input stream on the mixer has received a new sample
//		GetImageFromMixer();
		break;

	case MFVP_MESSAGE_STEP :					// Requests a frame step.
		TRACE_EVR ("MFVP_MESSAGE_STEP\n");
		m_nStepCount = ulParam;
		hr = S_OK;
		break;

	default :
		ASSERT (FALSE);
		break;
	}
	return hr;
}


HRESULT CEVRAllocatorPresenter::IsMediaTypeSupported(IMFMediaType* pMixerType)
{
	HRESULT				hr;
	AM_MEDIA_TYPE*		pAMMedia;
	UINT				nInterlaceMode;

	CheckHR (pMixerType->GetRepresentation(FORMAT_VideoInfo2, (void**)&pAMMedia));
	CheckHR (pMixerType->GetUINT32 (MF_MT_INTERLACE_MODE, &nInterlaceMode));


/*	if ( (pAMMedia->majortype != MEDIATYPE_Video) ||
		 (nInterlaceMode != MFVideoInterlace_Progressive) ||
		 ( (pAMMedia->subtype != MEDIASUBTYPE_RGB32) && (pAMMedia->subtype != MEDIASUBTYPE_RGB24) &&
		   (pAMMedia->subtype != MEDIASUBTYPE_YUY2)  && (pAMMedia->subtype != MEDIASUBTYPE_NV12) ) )
		   hr = MF_E_INVALIDMEDIATYPE;*/
	if ( (pAMMedia->majortype != MEDIATYPE_Video))
		hr = MF_E_INVALIDMEDIATYPE;
	pMixerType->FreeRepresentation (FORMAT_VideoInfo2, (void*)pAMMedia);
	return hr;
}


HRESULT CEVRAllocatorPresenter::CreateProposedOutputType(IMFMediaType* pMixerType, IMFMediaType** pType)
{
	HRESULT				hr;
	AM_MEDIA_TYPE*		pAMMedia = NULL;
	LARGE_INTEGER		i64Size;
	MFVIDEOFORMAT*		VideoFormat;

	CheckHR (pMixerType->GetRepresentation  (FORMAT_MFVideoFormat, (void**)&pAMMedia));
	
	VideoFormat = (MFVIDEOFORMAT*)pAMMedia->pbFormat;
	hr = pfMFCreateVideoMediaType  (VideoFormat, &m_pMediaType);

	if (0)
	{
		// This code doesn't work, use same method as VMR9 instead
		if (VideoFormat->videoInfo.FramesPerSecond.Numerator != 0)
		{
			switch (VideoFormat->videoInfo.InterlaceMode)
			{
				case MFVideoInterlace_Progressive:
				case MFVideoInterlace_MixedInterlaceOrProgressive:
				default:
					{
						m_rtTimePerFrame = (10000000I64*VideoFormat->videoInfo.FramesPerSecond.Denominator)/VideoFormat->videoInfo.FramesPerSecond.Numerator;
						m_bInterlaced = false;
					}
					break;
				case MFVideoInterlace_FieldSingleUpper:
				case MFVideoInterlace_FieldSingleLower:
				case MFVideoInterlace_FieldInterleavedUpperFirst:
				case MFVideoInterlace_FieldInterleavedLowerFirst:
					{
						m_rtTimePerFrame = (20000000I64*VideoFormat->videoInfo.FramesPerSecond.Denominator)/VideoFormat->videoInfo.FramesPerSecond.Numerator;
						m_bInterlaced = true;
					}
					break;
			}
		}
	}

	m_AspectRatio.cx	= VideoFormat->videoInfo.PixelAspectRatio.Numerator;
	m_AspectRatio.cy	= VideoFormat->videoInfo.PixelAspectRatio.Denominator;

	if (SUCCEEDED (hr))
	{
		i64Size.HighPart = VideoFormat->videoInfo.dwWidth;
		i64Size.LowPart	 = VideoFormat->videoInfo.dwHeight;
		m_pMediaType->SetUINT64 (MF_MT_FRAME_SIZE, i64Size.QuadPart);

		m_pMediaType->SetUINT32 (MF_MT_PAN_SCAN_ENABLED, 0);

		AppSettings& s = AfxGetAppSettings();
		
#if 1
		if (s.m_RenderSettings.iEVROutputRange == 1)
			m_pMediaType->SetUINT32 (MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_16_235);
		else
			m_pMediaType->SetUINT32 (MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_0_255);

//		m_pMediaType->SetUINT32 (MF_MT_TRANSFER_FUNCTION, MFVideoTransFunc_10);
		
#else

		m_pMediaType->SetUINT32 (MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_0_255);
		if (s.iEVROutputRange == 1)
			m_pMediaType->SetUINT32 (MF_MT_YUV_MATRIX, MFVideoTransferMatrix_BT601);
		else
			m_pMediaType->SetUINT32 (MF_MT_YUV_MATRIX, MFVideoTransferMatrix_BT709);
#endif
		

		m_LastSetOutputRange = s.m_RenderSettings.iEVROutputRange;

		i64Size.HighPart = m_AspectRatio.cx;
		i64Size.LowPart  = m_AspectRatio.cy;
		m_pMediaType->SetUINT64 (MF_MT_PIXEL_ASPECT_RATIO, i64Size.QuadPart);

		MFVideoArea Area = MakeArea (0, 0, VideoFormat->videoInfo.dwWidth, VideoFormat->videoInfo.dwHeight);
		m_pMediaType->SetBlob(MF_MT_GEOMETRIC_APERTURE, (UINT8*)&Area, sizeof(MFVideoArea));

	}

	m_AspectRatio.cx	*= VideoFormat->videoInfo.dwWidth;
	m_AspectRatio.cy	*= VideoFormat->videoInfo.dwHeight;

	bool bDoneSomething = true;

	while (bDoneSomething)
	{
		bDoneSomething = false;
		INT MinNum = min(m_AspectRatio.cx, m_AspectRatio.cy);
		INT i;
		for (i = 2; i < MinNum+1; ++i)
		{
			if (m_AspectRatio.cx%i == 0 && m_AspectRatio.cy%i ==0)
				break;
		}
		if (i != MinNum + 1)
		{
			m_AspectRatio.cx = m_AspectRatio.cx / i;
			m_AspectRatio.cy = m_AspectRatio.cy / i;
			bDoneSomething = true;
		}
	}

	pMixerType->FreeRepresentation (FORMAT_MFVideoFormat, (void*)pAMMedia);
	m_pMediaType->QueryInterface (__uuidof(IMFMediaType), (void**) pType);

	return hr;
}

HRESULT CEVRAllocatorPresenter::SetMediaType(IMFMediaType* pType)
{
	HRESULT				hr;
	AM_MEDIA_TYPE*		pAMMedia = NULL;
	CString				strTemp;

	CheckPointer (pType, E_POINTER);
	CheckHR (pType->GetRepresentation(FORMAT_VideoInfo2, (void**)&pAMMedia));
	
	hr = InitializeDevice (pAMMedia);
	if (SUCCEEDED (hr))
	{
		strTemp = GetMediaTypeName (pAMMedia->subtype);
		strTemp.Replace (L"MEDIASUBTYPE_", L"");
		m_strStatsMsg[MSG_MIXEROUT].Format (L"Mixer output : %s", strTemp);
	}

	pType->FreeRepresentation (FORMAT_VideoInfo2, (void*)pAMMedia);

	return hr;
}

LONGLONG GetMediaTypeMerit(IMFMediaType *pMediaType)
{
	AM_MEDIA_TYPE*		pAMMedia = NULL;
	MFVIDEOFORMAT*		VideoFormat;

	HRESULT hr;
	CheckHR (pMediaType->GetRepresentation  (FORMAT_MFVideoFormat, (void**)&pAMMedia));
	VideoFormat = (MFVIDEOFORMAT*)pAMMedia->pbFormat;

	LONGLONG Merit = 0;
	switch (VideoFormat->surfaceInfo.Format)
	{
		case FCC('NV12'): Merit = 90000000; break;
		case FCC('YV12'): Merit = 80000000; break;
		case FCC('YUY2'): Merit = 70000000; break;
		case FCC('UYVY'): Merit = 60000000; break;

		case D3DFMT_X8R8G8B8: // Never opt for RGB
		case D3DFMT_A8R8G8B8: 
		case D3DFMT_R8G8B8: 
		case D3DFMT_R5G6B5: 
			Merit = 0; 
			break;
		default: Merit = 1000; break;
	}
			
	pMediaType->FreeRepresentation (FORMAT_MFVideoFormat, (void*)pAMMedia);

	return Merit;
}



typedef struct
{
  const int				Format;
  const LPCTSTR			Description;
} D3DFORMAT_TYPE;

extern const D3DFORMAT_TYPE	D3DFormatType[];

LPCTSTR FindD3DFormat(const D3DFORMAT Format);

LPCTSTR GetMediaTypeFormatDesc(IMFMediaType *pMediaType)
{
	AM_MEDIA_TYPE*		pAMMedia = NULL;
	MFVIDEOFORMAT*		VideoFormat;

	HRESULT hr;
	hr = pMediaType->GetRepresentation  (FORMAT_MFVideoFormat, (void**)&pAMMedia);
	VideoFormat = (MFVIDEOFORMAT*)pAMMedia->pbFormat;

	LPCTSTR Type = FindD3DFormat((D3DFORMAT)VideoFormat->surfaceInfo.Format);
			
	pMediaType->FreeRepresentation (FORMAT_MFVideoFormat, (void*)pAMMedia);

	return Type;
}

HRESULT CEVRAllocatorPresenter::RenegotiateMediaType()
{
    HRESULT			hr = S_OK;

    CComPtr<IMFMediaType>	pMixerType;
    CComPtr<IMFMediaType>	pType;

    if (!m_pMixer)
    {
        return MF_E_INVALIDREQUEST;
    }

	CInterfaceArray<IMFMediaType> ValidMixerTypes;

    // Loop through all of the mixer's proposed output types.
    DWORD iTypeIndex = 0;
    while ((hr != MF_E_NO_MORE_TYPES))
    {
        pMixerType	 = NULL;
        pType		 = NULL;
		m_pMediaType = NULL;

        // Step 1. Get the next media type supported by mixer.
        hr = m_pMixer->GetOutputAvailableType(0, iTypeIndex++, &pMixerType);
        if (FAILED(hr))
        {
            break;
        }

        // Step 2. Check if we support this media type.
        if (SUCCEEDED(hr))
            hr = IsMediaTypeSupported(pMixerType);

        if (SUCCEEDED(hr))
	        hr = CreateProposedOutputType(pMixerType, &pType);
	
        // Step 4. Check if the mixer will accept this media type.
        if (SUCCEEDED(hr))
            hr = m_pMixer->SetOutputType(0, pType, MFT_SET_TYPE_TEST_ONLY);

        if (SUCCEEDED(hr))
		{
			LONGLONG Merit = GetMediaTypeMerit(pType);

			int nTypes = ValidMixerTypes.GetCount();
			int iInsertPos = 0;
			for (int i = 0; i < nTypes; ++i)
			{
				LONGLONG ThisMerit = GetMediaTypeMerit(ValidMixerTypes[i]);
				if (Merit > ThisMerit)
				{
					iInsertPos = i;
					break;
				}
				else
					iInsertPos = i+1;
			}

			ValidMixerTypes.InsertAt(iInsertPos, pType);
		}
    }


	int nValidTypes = ValidMixerTypes.GetCount();
	for (int i = 0; i < nValidTypes; ++i)
	{
        // Step 3. Adjust the mixer's type to match our requirements.
		pType = ValidMixerTypes[i];
		TRACE("Valid mixer output type: %ws\n", GetMediaTypeFormatDesc(pType));
	}

	for (int i = 0; i < nValidTypes; ++i)
	{
        // Step 3. Adjust the mixer's type to match our requirements.
		pType = ValidMixerTypes[i];

		
		TRACE("Trying mixer output type: %ws\n", GetMediaTypeFormatDesc(pType));

        // Step 5. Try to set the media type on ourselves.
		hr = SetMediaType(pType);

        // Step 6. Set output media type on mixer.
        if (SUCCEEDED(hr))
        {
            hr = m_pMixer->SetOutputType(0, pType, 0);

            // If something went wrong, clear the media type.
            if (FAILED(hr))
            {
                SetMediaType(NULL);
            }
			else
				break;
        }
	}

    pMixerType	= NULL;
    pType		= NULL;
    return hr;
}


bool CEVRAllocatorPresenter::GetImageFromMixer()
{
	MFT_OUTPUT_DATA_BUFFER		Buffer;
	HRESULT						hr = S_OK;
	DWORD						dwStatus;
	REFERENCE_TIME				nsSampleTime;
	LONGLONG					llClockBefore = 0;
	LONGLONG					llClockAfter  = 0;
	LONGLONG					llMixerLatency;
	UINT						dwSurface;

	bool bDoneSomething = false;

	while (SUCCEEDED(hr))
	{
		CComPtr<IMFSample>		pSample;

		if (FAILED (GetFreeSample (&pSample)))
		{
			m_bWaitingSample = true;
			break;
		}

		memset (&Buffer, 0, sizeof(Buffer));
		Buffer.pSample	= pSample;
		pSample->GetUINT32 (GUID_SURFACE_INDEX, &dwSurface);

		{
			llClockBefore = AfxGetMyApp()->GetPerfCounter();
			hr = m_pMixer->ProcessOutput (0 , 1, &Buffer, &dwStatus);
			llClockAfter = AfxGetMyApp()->GetPerfCounter();
		}

		if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) 
		{
			MoveToFreeList (pSample, false);
			break;
		}

		if (m_pSink) 
		{
			//CAutoLock autolock(this); We shouldn't need to lock here, m_pSink is thread safe
			llMixerLatency = llClockAfter - llClockBefore;
			m_pSink->Notify (EC_PROCESSING_LATENCY, (LONG_PTR)&llMixerLatency, 0);
		}

		pSample->GetSampleTime (&nsSampleTime);
		REFERENCE_TIME				nsDuration;
		pSample->GetSampleDuration (&nsDuration);

		if (AfxGetMyApp()->m_fTearingTest)
		{
			RECT		rcTearing;
			
			rcTearing.left		= m_nTearingPos;
			rcTearing.top		= 0;
			rcTearing.right		= rcTearing.left + 4;
			rcTearing.bottom	= m_NativeVideoSize.cy;
			m_pD3DDev->ColorFill (m_pVideoSurface[dwSurface], &rcTearing, D3DCOLOR_ARGB (255,255,0,0));

			rcTearing.left	= (rcTearing.right + 15) % m_NativeVideoSize.cx;
			rcTearing.right	= rcTearing.left + 4;
			m_pD3DDev->ColorFill (m_pVideoSurface[dwSurface], &rcTearing, D3DCOLOR_ARGB (255,255,0,0));
			m_nTearingPos = (m_nTearingPos + 7) % m_NativeVideoSize.cx;
		}	

		LONGLONG TimePerFrame = m_rtTimePerFrame;
		TRACE_EVR ("Get from Mixer : %d  (%I64d) (%I64d)\n", dwSurface, nsSampleTime, nsSampleTime/TimePerFrame);

		MoveToScheduledList (pSample, false);
		bDoneSomething = true;
		if (m_rtTimePerFrame == 0)
			break;
	}

	return bDoneSomething;
}



STDMETHODIMP CEVRAllocatorPresenter::GetCurrentMediaType(__deref_out  IMFVideoMediaType **ppMediaType)
{
    HRESULT hr = S_OK;
    CAutoLock lock(this);  // Hold the critical section.

    CheckPointer (ppMediaType, E_POINTER);
    CheckHR (CheckShutdown());

    if (m_pMediaType == NULL)
        CheckHR(MF_E_NOT_INITIALIZED);

    CheckHR(m_pMediaType->QueryInterface( __uuidof(IMFVideoMediaType), (void**)&ppMediaType));

    return hr;
}



// IMFTopologyServiceLookupClient        
STDMETHODIMP CEVRAllocatorPresenter::InitServicePointers(/* [in] */ __in  IMFTopologyServiceLookup *pLookup)
{
	HRESULT						hr;
	DWORD						dwObjects = 1;

	TRACE_EVR ("EVR : CEVRAllocatorPresenter::InitServicePointers\n");
	hr = pLookup->LookupService (MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_MIXER_SERVICE,
								  __uuidof (IMFTransform), (void**)&m_pMixer, &dwObjects);

	hr = pLookup->LookupService (MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_RENDER_SERVICE,
								  __uuidof (IMediaEventSink ), (void**)&m_pSink, &dwObjects);

	hr = pLookup->LookupService (MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_RENDER_SERVICE,
								  __uuidof (IMFClock ), (void**)&m_pClock, &dwObjects);


	StartWorkerThreads();
	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::ReleaseServicePointers()
{
	TRACE_EVR ("EVR : CEVRAllocatorPresenter::ReleaseServicePointers\n");
	StopWorkerThreads();
	m_pMixer	= NULL;
	m_pSink		= NULL;
	m_pClock	= NULL;
	return S_OK;
}


// IMFVideoDeviceID
STDMETHODIMP CEVRAllocatorPresenter::GetDeviceID(/* [out] */	__out  IID *pDeviceID)
{
	CheckPointer(pDeviceID, E_POINTER);
	*pDeviceID = IID_IDirect3DDevice9;
	return S_OK;
}


// IMFGetService
STDMETHODIMP CEVRAllocatorPresenter::GetService (/* [in] */ __RPC__in REFGUID guidService,
								/* [in] */ __RPC__in REFIID riid,
								/* [iid_is][out] */ __RPC__deref_out_opt LPVOID *ppvObject)
{
	if (guidService == MR_VIDEO_RENDER_SERVICE)
		return NonDelegatingQueryInterface (riid, ppvObject);
	else if (guidService == MR_VIDEO_ACCELERATION_SERVICE)
		return m_pD3DManager->QueryInterface (__uuidof(IDirect3DDeviceManager9), (void**) ppvObject);

	return E_NOINTERFACE;
}


// IMFAsyncCallback
STDMETHODIMP CEVRAllocatorPresenter::GetParameters(	/* [out] */ __RPC__out DWORD *pdwFlags, /* [out] */ __RPC__out DWORD *pdwQueue)
{
	return E_NOTIMPL;
}

STDMETHODIMP CEVRAllocatorPresenter::Invoke		 (	/* [in] */ __RPC__in_opt IMFAsyncResult *pAsyncResult)
{
	return E_NOTIMPL;
}


// IMFVideoDisplayControl
STDMETHODIMP CEVRAllocatorPresenter::GetNativeVideoSize(SIZE *pszVideo, SIZE *pszARVideo)
{
	if (pszVideo)
	{
		pszVideo->cx	= m_NativeVideoSize.cx;
		pszVideo->cy	= m_NativeVideoSize.cy;
	}
	if (pszARVideo)
	{
		pszARVideo->cx	= m_NativeVideoSize.cx * m_AspectRatio.cx;
		pszARVideo->cy	= m_NativeVideoSize.cy * m_AspectRatio.cy;
	}
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::GetIdealVideoSize(SIZE *pszMin, SIZE *pszMax)
{
	if (pszMin)
	{
		pszMin->cx	= 1;
		pszMin->cy	= 1;
	}

	if (pszMax)
	{
		D3DDISPLAYMODE	d3ddm;

		ZeroMemory(&d3ddm, sizeof(d3ddm));
		if(SUCCEEDED(m_pD3D->GetAdapterDisplayMode(GetAdapter(m_pD3D), &d3ddm)))
		{
			pszMax->cx	= d3ddm.Width;
			pszMax->cy	= d3ddm.Height;
		}
	}

	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::SetVideoPosition(const MFVideoNormalizedRect *pnrcSource, const LPRECT prcDest)
{
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::GetVideoPosition(MFVideoNormalizedRect *pnrcSource, LPRECT prcDest)
{
	// Always all source rectangle ?
	if (pnrcSource)
	{
		pnrcSource->left	= 0.0;
		pnrcSource->top		= 0.0;
		pnrcSource->right	= 1.0;
		pnrcSource->bottom	= 1.0;
	}

	if (prcDest)
		memcpy (prcDest, &m_VideoRect, sizeof(m_VideoRect));//GetClientRect (m_hWnd, prcDest);

	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::SetAspectRatioMode(DWORD dwAspectRatioMode)
{
	m_dwVideoAspectRatioMode = (MFVideoAspectRatioMode)dwAspectRatioMode;
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::GetAspectRatioMode(DWORD *pdwAspectRatioMode)
{
	CheckPointer (pdwAspectRatioMode, E_POINTER);
	*pdwAspectRatioMode = m_dwVideoAspectRatioMode;
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::SetVideoWindow(HWND hwndVideo)
{
	ASSERT (m_hWnd == hwndVideo);	// What if not ??
//	m_hWnd = hwndVideo;
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::GetVideoWindow(HWND *phwndVideo)
{
	CheckPointer (phwndVideo, E_POINTER);
	*phwndVideo = m_hWnd;
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::RepaintVideo()
{
	Paint (true);
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::GetCurrentImage(BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib, LONGLONG *pTimeStamp)
{
	ASSERT (FALSE);
	return E_NOTIMPL;
}
STDMETHODIMP CEVRAllocatorPresenter::SetBorderColor(COLORREF Clr)
{
	m_BorderColor = Clr;
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::GetBorderColor(COLORREF *pClr)
{
	CheckPointer (pClr, E_POINTER);
	*pClr = m_BorderColor;
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::SetRenderingPrefs(DWORD dwRenderFlags)
{
	m_dwVideoRenderPrefs = (MFVideoRenderPrefs)dwRenderFlags;
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::GetRenderingPrefs(DWORD *pdwRenderFlags)
{
	CheckPointer(pdwRenderFlags, E_POINTER);
	*pdwRenderFlags = m_dwVideoRenderPrefs;
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::SetFullscreen(BOOL fFullscreen)
{
	ASSERT (FALSE);
	return E_NOTIMPL;
}
STDMETHODIMP CEVRAllocatorPresenter::GetFullscreen(BOOL *pfFullscreen)
{
	ASSERT (FALSE);
	return E_NOTIMPL;
}


// IEVRTrustedVideoPlugin
STDMETHODIMP CEVRAllocatorPresenter::IsInTrustedVideoMode(BOOL *pYes)
{
	CheckPointer(pYes, E_POINTER);
	*pYes = TRUE;
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::CanConstrict(BOOL *pYes)
{
	CheckPointer(pYes, E_POINTER);
	*pYes = TRUE;
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::SetConstriction(DWORD dwKPix)
{
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::DisableImageExport(BOOL bDisable)
{
	return S_OK;
}


// IDirect3DDeviceManager9
STDMETHODIMP CEVRAllocatorPresenter::ResetDevice(IDirect3DDevice9 *pDevice,UINT resetToken)
{
	HRESULT		hr = m_pD3DManager->ResetDevice (pDevice, resetToken);
	return hr;
}
STDMETHODIMP CEVRAllocatorPresenter::OpenDeviceHandle(HANDLE *phDevice)
{
	HRESULT		hr = m_pD3DManager->OpenDeviceHandle (phDevice);
	return hr;
}
STDMETHODIMP CEVRAllocatorPresenter::CloseDeviceHandle(HANDLE hDevice)
{
	HRESULT		hr = m_pD3DManager->CloseDeviceHandle(hDevice);
	return hr;
}
STDMETHODIMP CEVRAllocatorPresenter::TestDevice(HANDLE hDevice)
{
	HRESULT		hr = m_pD3DManager->TestDevice(hDevice);
	return hr;
}
STDMETHODIMP CEVRAllocatorPresenter::LockDevice(HANDLE hDevice, IDirect3DDevice9 **ppDevice, BOOL fBlock)
{
	HRESULT		hr = m_pD3DManager->LockDevice(hDevice, ppDevice, fBlock);
	return hr;
}
STDMETHODIMP CEVRAllocatorPresenter::UnlockDevice(HANDLE hDevice, BOOL fSaveState)
{
	HRESULT		hr = m_pD3DManager->UnlockDevice(hDevice, fSaveState);
	return hr;
}
STDMETHODIMP CEVRAllocatorPresenter::GetVideoService(HANDLE hDevice, REFIID riid, void **ppService)
{
	HRESULT		hr = m_pD3DManager->GetVideoService(hDevice, riid, ppService);

	if (riid == __uuidof(IDirectXVideoDecoderService))
	{
		UINT		nNbDecoder = 5;
		GUID*		pDecoderGuid;
		IDirectXVideoDecoderService*		pDXVAVideoDecoder = (IDirectXVideoDecoderService*) *ppService;
		pDXVAVideoDecoder->GetDecoderDeviceGuids (&nNbDecoder, &pDecoderGuid);
	}
	else if (riid == __uuidof(IDirectXVideoProcessorService))
	{
		IDirectXVideoProcessorService*		pDXVAProcessor = (IDirectXVideoProcessorService*) *ppService;
	}

	return hr;
}


STDMETHODIMP CEVRAllocatorPresenter::GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight)
{
	// This function should be called...
	ASSERT (FALSE);

	if(lpWidth)		*lpWidth	= m_NativeVideoSize.cx;
	if(lpHeight)	*lpHeight	= m_NativeVideoSize.cy;
	if(lpARWidth)	*lpARWidth	= m_AspectRatio.cx;
	if(lpARHeight)	*lpARHeight	= m_AspectRatio.cy;
	return S_OK;
}


STDMETHODIMP CEVRAllocatorPresenter::InitializeDevice(AM_MEDIA_TYPE*	pMediaType)
{
	HRESULT			hr;
	CAutoLock lock(this);
	CAutoLock lock2(&m_ImageProcessingLock);
	CAutoLock cRenderLock(&m_RenderLock);

	RemoveAllSamples();
	DeleteSurfaces();

	VIDEOINFOHEADER2*		vih2 = (VIDEOINFOHEADER2*) pMediaType->pbFormat;
	int						w = vih2->bmiHeader.biWidth;
	int						h = abs(vih2->bmiHeader.biHeight);

	m_NativeVideoSize = CSize(w, h);
	if (m_bHighColorResolution)
		hr = AllocSurfaces(D3DFMT_A2R10G10B10);
	else
		hr = AllocSurfaces(D3DFMT_X8R8G8B8);
	

	for(int i = 0; i < m_nNbDXSurface; i++)
	{
		CComPtr<IMFSample>		pMFSample;
		hr = pfMFCreateVideoSampleFromSurface (m_pVideoSurface[i], &pMFSample);

		if (SUCCEEDED (hr))
		{
			pMFSample->SetUINT32 (GUID_SURFACE_INDEX, i);
			m_FreeSamples.AddTail (pMFSample);
		}
		ASSERT (SUCCEEDED (hr));
	}


	return hr;
}


DWORD WINAPI CEVRAllocatorPresenter::GetMixerThreadStatic(LPVOID lpParam)
{
	CEVRAllocatorPresenter*		pThis = (CEVRAllocatorPresenter*) lpParam;
	pThis->GetMixerThread();
	return 0;
}


DWORD WINAPI CEVRAllocatorPresenter::PresentThread(LPVOID lpParam)
{
	CEVRAllocatorPresenter*		pThis = (CEVRAllocatorPresenter*) lpParam;
	pThis->RenderThread();
	return 0;
}


void CEVRAllocatorPresenter::CheckWaitingSampleFromMixer()
{
	if (m_bWaitingSample)
	{
		m_bWaitingSample = false;
		//GetImageFromMixer(); // Do this in processing thread instead
	}
}


bool ExtractInterlaced(const AM_MEDIA_TYPE* pmt)
{
	if (pmt->formattype==FORMAT_VideoInfo)
		return false;
	else if (pmt->formattype==FORMAT_VideoInfo2)
		return (((VIDEOINFOHEADER2*)pmt->pbFormat)->dwInterlaceFlags & AMINTERLACE_IsInterlaced) != 0;
	else if (pmt->formattype==FORMAT_MPEGVideo)
		return false;
	else if (pmt->formattype==FORMAT_MPEG2Video)
		return (((MPEG2VIDEOINFO*)pmt->pbFormat)->hdr.dwInterlaceFlags & AMINTERLACE_IsInterlaced) != 0;
	else
		return false;
}


void CEVRAllocatorPresenter::GetMixerThread()
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

    timeGetDevCaps(&tc, sizeof(TIMECAPS));
    dwResolution = min(max(tc.wPeriodMin, 0), tc.wPeriodMax);
    dwUser		= timeBeginPeriod(dwResolution);

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
				bool bDoneSomething = false;
				{
					CAutoLock AutoLock(&m_ImageProcessingLock);
					bDoneSomething = GetImageFromMixer();
				}
				if (m_rtTimePerFrame == 0 && bDoneSomething)
				{
					//CAutoLock lock(this);
					//CAutoLock lock2(&m_ImageProcessingLock);
					//CAutoLock cRenderLock(&m_RenderLock);

					// Use the code from VMR9 to get the movie fps, as this method is reliable.
					CComPtr<IPin>			pPin;
					CMediaType				mt;
					if (
						SUCCEEDED (m_pOuterEVR->FindPin(L"EVR Input0", &pPin)) &&
						SUCCEEDED (pPin->ConnectionMediaType(&mt)) )
					{
						ExtractAvgTimePerFrame (&mt, m_rtTimePerFrame);

						m_bInterlaced = ExtractInterlaced(&mt);

					}
					// If framerate not set by Video Decoder choose 23.97...
					if (m_rtTimePerFrame == 0) 
						m_rtTimePerFrame = 417166;

					// Update internal subtitle clock
					if(m_fUseInternalTimer && m_pSubPicQueue)
					{
						m_fps = (float)(10000000.0 / m_rtTimePerFrame);
						m_pSubPicQueue->SetFPS(m_fps);
					}

				}

			}
			break;
		}
	}

	timeEndPeriod (dwResolution);
//	if (pfAvRevertMmThreadCharacteristics) pfAvRevertMmThreadCharacteristics (hAvrt);
}

void ModerateFloat(double& Value, double Target, double& ValuePrim, double ChangeSpeed)
{
	double xbiss = (-(ChangeSpeed)*(ValuePrim) - (Value-Target)*(ChangeSpeed*ChangeSpeed)*0.25f);
	ValuePrim += xbiss;
	Value += ValuePrim;
}

LONGLONG CEVRAllocatorPresenter::GetClockTime(LONGLONG PerformanceCounter)
{
	LONGLONG			llClockTime;
	MFTIME				nsCurrentTime;
	m_pClock->GetCorrelatedTime(0, &llClockTime, &nsCurrentTime);
	DWORD Characteristics = 0;
	m_pClock->GetClockCharacteristics(&Characteristics);
	MFCLOCK_STATE State;
	m_pClock->GetState(0, &State);

	if (!(Characteristics & MFCLOCK_CHARACTERISTICS_FLAG_FREQUENCY_10MHZ))
	{
		MFCLOCK_PROPERTIES Props;
		if (m_pClock->GetProperties(&Props) == S_OK)
			llClockTime = (llClockTime * 10000000) / Props.qwClockFrequency; // Make 10 MHz

	}
	LONGLONG llPerf = PerformanceCounter;
//	return llClockTime + (llPerf - nsCurrentTime);
	double Target = llClockTime + (llPerf - nsCurrentTime) * m_ModeratedTimeSpeed;

	bool bReset = false;
	if (m_ModeratedTimeLast < 0 || State != m_LastClockState || m_ModeratedClockLast < 0)
	{
		bReset = true;
		m_ModeratedTimeLast = llPerf;
		m_ModeratedClockLast = llClockTime;
	}

	m_LastClockState = State;

	double TimeChange = llPerf - m_ModeratedTimeLast;
	double ClockChange = llClockTime - m_ModeratedClockLast;

	m_ModeratedTimeLast = llPerf;
	m_ModeratedClockLast = llClockTime;

#if 1

	if (bReset)
	{
		m_ModeratedTimeSpeed = 1.0;
		m_ModeratedTimeSpeedPrim = 0.0;
		ZeroMemory(m_TimeChangeHisotry, sizeof(m_TimeChangeHisotry));
		ZeroMemory(m_ClockChangeHisotry, sizeof(m_ClockChangeHisotry));
		m_ClockTimeChangeHistoryPos = 0;
	}
	if (TimeChange)
	{
		int Pos = m_ClockTimeChangeHistoryPos % 100;
		int nHistory = min(m_ClockTimeChangeHistoryPos, 100);
		++m_ClockTimeChangeHistoryPos;
		if (nHistory > 50)
		{
			int iLastPos = (Pos - (nHistory)) % 100;
			if (iLastPos < 0)
				iLastPos += 100;

			double TimeChange = llPerf - m_TimeChangeHisotry[iLastPos];
			double ClockChange = llClockTime - m_ClockChangeHisotry[iLastPos];

			double ClockSpeedTarget = ClockChange / TimeChange;
			double ChangeSpeed = 0.1;
			if (ClockSpeedTarget > m_ModeratedTimeSpeed)
			{
				if (ClockSpeedTarget / m_ModeratedTimeSpeed > 0.1)
					ChangeSpeed = 0.1;
				else
					ChangeSpeed = 0.01;
			}
			else 
			{
				if (m_ModeratedTimeSpeed / ClockSpeedTarget > 0.1)
					ChangeSpeed = 0.1;
				else
					ChangeSpeed = 0.01;
			}
			ModerateFloat(m_ModeratedTimeSpeed, ClockSpeedTarget, m_ModeratedTimeSpeedPrim, ChangeSpeed);
//			m_ModeratedTimeSpeed = TimeChange / ClockChange;
		}
		m_TimeChangeHisotry[Pos] = llPerf;
		m_ClockChangeHisotry[Pos] = llClockTime;
	}

	return Target;
#else
	double EstimateTime = m_ModeratedTime + TimeChange * m_ModeratedTimeSpeed + m_ClockDiffCalc;
	double Diff = Target - EstimateTime;
	
	// > 5 ms just set it
	if ((fabs(Diff) > 50000.0 || bReset))
	{

//		TRACE("Reset clock at diff: %f ms\n", (m_ModeratedTime - Target) /10000.0);
		if (State == MFCLOCK_STATE_RUNNING)
		{
			if (bReset)
			{
				m_ModeratedTimeSpeed = 1.0;
				m_ModeratedTimeSpeedPrim = 0.0;
				m_ClockDiffCalc = 0;
				m_ClockDiffPrim = 0;
				m_ModeratedTime = Target;
				m_ModeratedTimer = llPerf;
			}
			else
			{
				EstimateTime = m_ModeratedTime + TimeChange * m_ModeratedTimeSpeed;
				Diff = Target - EstimateTime;
				m_ClockDiffCalc = Diff;
				m_ClockDiffPrim = 0;
			}
		}
		else
		{
			m_ModeratedTimeSpeed = 0.0;
			m_ModeratedTimeSpeedPrim = 0.0;
			m_ClockDiffCalc = 0;
			m_ClockDiffPrim = 0;
			m_ModeratedTime = Target;
			m_ModeratedTimer = llPerf;
		}
	}

	{
		LONGLONG ModerateTime = 10000;
		double ChangeSpeed = 1.00;
/*		if (m_ModeratedTimeSpeedPrim != 0.0)
		{
			if (m_ModeratedTimeSpeedPrim < 0.1)
				ChangeSpeed = 0.1;
		}*/

		int nModerate = 0;
		double Change = 0;
		while (m_ModeratedTimer < llPerf - ModerateTime)
		{
			m_ModeratedTimer += ModerateTime;
			m_ModeratedTime += double(ModerateTime) * m_ModeratedTimeSpeed;

			double TimerDiff = llPerf - m_ModeratedTimer;

			double Diff = (double)(m_ModeratedTime - (Target - TimerDiff));

			double TimeSpeedTarget;
			double AbsDiff = fabs(Diff);
			TimeSpeedTarget = 1.0 - (Diff / 1000000.0);
//			TimeSpeedTarget = m_ModeratedTimeSpeed - (Diff / 100000000000.0);
			//if (AbsDiff > 20000.0)
//				TimeSpeedTarget = 1.0 - (Diff / 1000000.0);
			/*else if (AbsDiff > 5000.0)
				TimeSpeedTarget = 1.0 - (Diff / 100000000.0);
			else
				TimeSpeedTarget = 1.0 - (Diff / 500000000.0);*/
			double StartMod = m_ModeratedTimeSpeed;
			ModerateFloat(m_ModeratedTimeSpeed, TimeSpeedTarget, m_ModeratedTimeSpeedPrim, ChangeSpeed);
			m_ModeratedTimeSpeed = TimeSpeedTarget;
			++nModerate;
			Change += m_ModeratedTimeSpeed - StartMod;
		}
		if (nModerate)
			m_ModeratedTimeSpeedDiff = Change / nModerate;
		
		double Ret = m_ModeratedTime + double(llPerf - m_ModeratedTimer) * m_ModeratedTimeSpeed;
		double Diff = Target - Ret;
		ModerateFloat(m_ClockDiffCalc, Diff, m_ClockDiffPrim, ChangeSpeed*0.1);
		
		Ret += m_ClockDiffCalc;
		Diff = Target - Ret;
		m_ClockDiff = Diff;
		return LONGLONG(Ret + 0.5);
	}

	return Target;
	return LONGLONG(m_ModeratedTime + 0.5);
#endif
}

void CEVRAllocatorPresenter::OnVBlankFinished(bool fAll, LONGLONG PerformanceCounter)
{
	if (!m_pCurrentDisplaydSample || !m_OrderedPaint || !fAll)
		return;

	LONGLONG			llClockTime;
	LONGLONG			nsSampleTime;
	LONGLONG SampleDuration = 0; 
	if (!m_bSignaledStarvation)
	{
		llClockTime = GetClockTime(PerformanceCounter);
		m_StarvationClock = llClockTime;
	}
	else
	{
		llClockTime = m_StarvationClock;
	}
	m_pCurrentDisplaydSample->GetSampleDuration(&SampleDuration);

	m_pCurrentDisplaydSample->GetSampleTime(&nsSampleTime);
	LONGLONG TimePerFrame = m_rtTimePerFrame;
	if (!TimePerFrame)
		return;
	if (SampleDuration > 1)
		TimePerFrame = SampleDuration;	

	{
		m_nNextSyncOffset = (m_nNextSyncOffset+1) % NB_JITTER;
		LONGLONG SyncOffset = nsSampleTime - llClockTime;

		m_pllSyncOffset[m_nNextSyncOffset] = SyncOffset;
//		TRACE("SyncOffset(%d, %d): %8I64d     %8I64d     %8I64d \n", m_nCurSurface, m_VSyncMode, m_LastPredictedSync, -SyncOffset, m_LastPredictedSync - (-SyncOffset));

		m_MaxSyncOffset = MINLONG64;
		m_MinSyncOffset = MAXLONG64;

		LONGLONG AvrageSum = 0;
		for (int i=0; i<NB_JITTER; i++)
		{
			LONGLONG Offset = m_pllSyncOffset[i];
			AvrageSum += Offset;
			m_MaxSyncOffset = max(m_MaxSyncOffset, Offset);
			m_MinSyncOffset = min(m_MinSyncOffset, Offset);
		}
		double MeanOffset = double(AvrageSum)/NB_JITTER;
		double DeviationSum = 0;
		for (int i=0; i<NB_JITTER; i++)
		{
			double Deviation = double(m_pllSyncOffset[i]) - MeanOffset;
			DeviationSum += Deviation*Deviation;
		}
		double StdDev = sqrt(DeviationSum/NB_JITTER);

		m_fSyncOffsetAvr = MeanOffset;
		 m_bSyncStatsAvailable = true;
		m_fSyncOffsetStdDev = StdDev;

		
	}
}

void CEVRAllocatorPresenter::RenderThread()
{
	HANDLE				hAvrt;
	DWORD				dwTaskIndex	= 0;
	HANDLE				hEvts[]		= { m_hEvtQuit, m_hEvtFlush};
	bool				bQuit		= false;
    TIMECAPS			tc;
	DWORD				dwResolution;
	MFTIME				nsSampleTime;
	LONGLONG			llClockTime;
	DWORD				dwUser = 0;
	DWORD				dwObject;

	
	// Tell Vista Multimedia Class Scheduler we are a playback thretad (increase priority)
	if (pfAvSetMmThreadCharacteristicsW)	hAvrt = pfAvSetMmThreadCharacteristicsW (L"Playback", &dwTaskIndex);
	if (pfAvSetMmThreadPriority)			pfAvSetMmThreadPriority (hAvrt, AVRT_PRIORITY_HIGH /*AVRT_PRIORITY_CRITICAL*/);

    timeGetDevCaps(&tc, sizeof(TIMECAPS));
    dwResolution = min(max(tc.wPeriodMin, 0), tc.wPeriodMax);
    dwUser		= timeBeginPeriod(dwResolution);
	AppSettings& s = AfxGetAppSettings();

	int NextSleepTime = 1;
	while (!bQuit)
	{
		LONGLONG	llPerf = AfxGetMyApp()->GetPerfCounter();
		if (!s.m_RenderSettings.iVMR9VSyncAccurate && NextSleepTime == 0)
			NextSleepTime = 1;
		dwObject = WaitForMultipleObjects (countof(hEvts), hEvts, FALSE, max(NextSleepTime < 0 ? 1 : NextSleepTime, 0));
/*		dwObject = WAIT_TIMEOUT;
		if (m_bEvtFlush)
			dwObject = WAIT_OBJECT_0 + 1;
		else if (m_bEvtQuit)
			dwObject = WAIT_OBJECT_0;*/
//		if (NextSleepTime)
//			TRACE("Sleep: %7.3f\n", double(AfxGetMyApp()->GetPerfCounter()-llPerf) / 10000.0);
		if (NextSleepTime > 1)
			NextSleepTime = 0;
		else if (NextSleepTime == 0)
			NextSleepTime = -1;			
		switch (dwObject)
		{
		case WAIT_OBJECT_0 :
			bQuit = true;
			break;
		case WAIT_OBJECT_0 + 1 :
			// Flush pending samples!
			FlushSamples();
			m_bEvtFlush = false;
			ResetEvent(m_hEvtFlush);
			TRACE_EVR ("Flush done!\n");
			break;

		case WAIT_TIMEOUT :

			if (m_LastSetOutputRange != -1 && m_LastSetOutputRange != s.m_RenderSettings.iEVROutputRange || m_bPendingRenegotiate)
			{
				FlushSamples();
				RenegotiateMediaType();
				m_bPendingRenegotiate = false;
			}
			if (m_bPendingResetDevice)
			{
				m_bPendingResetDevice = false;
				CAutoLock lock(this);
				CAutoLock lock2(&m_ImageProcessingLock);
				CAutoLock cRenderLock(&m_RenderLock);

				
				RemoveAllSamples();

				CDX9AllocatorPresenter::ResetDevice();

				for(int i = 0; i < m_nNbDXSurface; i++)
				{
					CComPtr<IMFSample>		pMFSample;
					HRESULT hr = pfMFCreateVideoSampleFromSurface (m_pVideoSurface[i], &pMFSample);

					if (SUCCEEDED (hr))
					{
						pMFSample->SetUINT32 (GUID_SURFACE_INDEX, i);
						m_FreeSamples.AddTail (pMFSample);
					}
					ASSERT (SUCCEEDED (hr));
				}

			}
			// Discard timer events if playback stop
//			if ((dwObject == WAIT_OBJECT_0 + 3) && (m_nRenderState != Started)) continue;

//			TRACE_EVR ("RenderThread ==>> Waiting buffer\n");
			
//			if (WaitForMultipleObjects (countof(hEvtsBuff), hEvtsBuff, FALSE, INFINITE) == WAIT_OBJECT_0+2)
			{
				CComPtr<IMFSample>		pMFSample;
				LONGLONG	llPerf = AfxGetMyApp()->GetPerfCounter();
				int nSamplesLeft = 0;
				if (SUCCEEDED (GetScheduledSample(&pMFSample, nSamplesLeft)))
				{ 
//					pMFSample->GetUINT32 (GUID_SURFACE_INDEX, (UINT32*)&m_nCurSurface);
					m_pCurrentDisplaydSample = pMFSample;

					bool bValidSampleTime = true;
					HRESULT hGetSampleTime = pMFSample->GetSampleTime (&nsSampleTime);
					if (hGetSampleTime != S_OK || nsSampleTime == 0)
					{
						bValidSampleTime = false;
					}
					// We assume that all samples have the same duration
					LONGLONG SampleDuration = 0; 
					pMFSample->GetSampleDuration(&SampleDuration);

//					TRACE_EVR ("RenderThread ==>> Presenting surface %d  (%I64d)\n", m_nCurSurface, nsSampleTime);

					bool bStepForward = false;

					if (m_nStepCount < 0)
					{
						// Drop frame
						TRACE_EVR ("Dropped frame\n");
						m_pcFrames++;
						bStepForward = true;
						m_nStepCount = 0;
					}
					else if (m_nStepCount > 0)
					{
						pMFSample->GetUINT32(GUID_SURFACE_INDEX, (UINT32 *)&m_nCurSurface);
						++m_OrderedPaint;
						if (!g_bExternalSubtitleTime)
							__super::SetTime (g_tSegmentStart + nsSampleTime);
						Paint(true);
						m_nDroppedUpdate = 0;
						CompleteFrameStep (false);
						bStepForward = true;
					}
					else if ((m_nRenderState == Started))
					{
						LONGLONG CurrentCounter = AfxGetMyApp()->GetPerfCounter();
						// Calculate wake up timer
						if (!m_bSignaledStarvation)
						{
							llClockTime = GetClockTime(CurrentCounter);
							m_StarvationClock = llClockTime;
						}
						else
						{
							llClockTime = m_StarvationClock;
						}

						if (!bValidSampleTime)
						{
							// Just play as fast as possible
							bStepForward = true;
							pMFSample->GetUINT32(GUID_SURFACE_INDEX, (UINT32 *)&m_nCurSurface);
							++m_OrderedPaint;
							if (!g_bExternalSubtitleTime)
								__super::SetTime (g_tSegmentStart + nsSampleTime);
							Paint(true);
						}
						else
						{
							LONGLONG TimePerFrame = GetFrameTime() * 10000000.0;
							LONGLONG DrawTime = (m_PaintTime) * 0.9 - 20000.0; // 2 ms offset
							//if (!s.iVMR9VSync)
								DrawTime = 0;

							LONGLONG SyncOffset = 0;
							LONGLONG VSyncTime = 0;
							LONGLONG RefreshTime = 0;
							LONGLONG TimeToNextVSync = -1;
							bool bVSyncCorrection = false;
							double DetectedRefreshTime;
							double DetectedScanlinesPerFrame;
							double DetectedScanlineTime;
							int DetectedRefreshRatePos;
							{
								CAutoLock Lock(&m_RefreshRateLock);	
								DetectedRefreshTime = m_DetectedRefreshTime;
								DetectedRefreshRatePos = m_DetectedRefreshRatePos;
								DetectedScanlinesPerFrame = m_DetectedScanlinesPerFrame;
								DetectedScanlineTime = m_DetectedScanlineTime;
							}

							if (DetectedRefreshRatePos < 20 || !DetectedRefreshTime || !DetectedScanlinesPerFrame)
							{
								DetectedRefreshTime = 1.0/m_RefreshRate;
								DetectedScanlinesPerFrame = m_ScreenSize.cy;
								DetectedScanlineTime = DetectedRefreshTime / double(m_ScreenSize.cy);
							}

							if (s.m_RenderSettings.iVMR9VSync)
							{
								bVSyncCorrection = true;
								double TargetVSyncPos = GetVBlackPos();
								double RefreshLines = DetectedScanlinesPerFrame;
								double RefreshTime = DetectedRefreshTime;
								double ScanlinesPerSecond = 1.0/DetectedScanlineTime;
								double CurrentVSyncPos = fmod(double(m_VBlankStartMeasure) + ScanlinesPerSecond * ((CurrentCounter - m_VBlankStartMeasureTime) / 10000000.0), RefreshLines);
								double LinesUntilVSync = 0;
								//TargetVSyncPos -= ScanlinesPerSecond * (DrawTime/10000000.0);
								//TargetVSyncPos -= 10;
								TargetVSyncPos = fmod(TargetVSyncPos, RefreshLines);
								if (TargetVSyncPos < 0)
									TargetVSyncPos += RefreshLines;
								if (TargetVSyncPos > CurrentVSyncPos)
									LinesUntilVSync = TargetVSyncPos - CurrentVSyncPos;
								else
									LinesUntilVSync = (RefreshLines - CurrentVSyncPos) + TargetVSyncPos;
								double TimeUntilVSync = LinesUntilVSync * DetectedScanlineTime;
								TimeToNextVSync = TimeUntilVSync * 10000000.0;
								VSyncTime = DetectedRefreshTime * 10000000.0;
								RefreshTime = VSyncTime;

								LONGLONG ClockTimeAtNextVSync = llClockTime + (TimeUntilVSync * 10000000.0) * m_ModeratedTimeSpeed;
	
								SyncOffset = (nsSampleTime - ClockTimeAtNextVSync);

//								if (SyncOffset < 0)
//									TRACE("SyncOffset(%d): %I64d     %I64d     %I64d\n", m_nCurSurface, SyncOffset, TimePerFrame, VSyncTime);
							}
							else
								SyncOffset = (nsSampleTime - llClockTime);
							
							//LONGLONG SyncOffset = nsSampleTime - llClockTime;
							TRACE_EVR ("SyncOffset: %I64d SampleFrame: %I64d ClockFrame: %I64d\n", SyncOffset, nsSampleTime/TimePerFrame, llClockTime /TimePerFrame);
							if (SampleDuration > 1 && !m_DetectedLock)
								TimePerFrame = SampleDuration;

							LONGLONG MinMargin;
							if (m_FrameTimeCorrection && 0)
								MinMargin = 15000.0;
							else
								MinMargin = 15000.0 + min(m_DetectedFrameTimeStdDev, 20000.0);
							LONGLONG TimePerFrameMargin = min(double(TimePerFrame)*0.11, max(double(TimePerFrame)*0.02, MinMargin));
							LONGLONG TimePerFrameMargin0 = TimePerFrameMargin/2;
							LONGLONG TimePerFrameMargin1 = 0;

							if (m_DetectedLock && TimePerFrame < VSyncTime)
								VSyncTime = TimePerFrame;

							if (m_VSyncMode == 1)
								TimePerFrameMargin1 = -TimePerFrameMargin;
							else if (m_VSyncMode == 2)
								TimePerFrameMargin1 = TimePerFrameMargin;

							m_LastSampleOffset = SyncOffset;
							m_bLastSampleOffsetValid = true;

							LONGLONG VSyncOffset0 = 0;
							bool bDoVSyncCorrection = false;
							if ((SyncOffset < -(TimePerFrame + TimePerFrameMargin0 - TimePerFrameMargin1)) && nSamplesLeft > 0) // Only drop if we have something else to display at once
							{
								// Drop frame
								TRACE_EVR ("Dropped frame\n");
								m_pcFrames++;
								bStepForward = true;
								++m_nDroppedUpdate;
								NextSleepTime = 0;
//								VSyncOffset0 = (-SyncOffset) - VSyncTime;
								//VSyncOffset0 = (-SyncOffset) - VSyncTime + TimePerFrameMargin1;
								//m_LastPredictedSync = VSyncOffset0;
								bDoVSyncCorrection = false;
							}
							else if (SyncOffset < TimePerFrameMargin1)
							{

								if (bVSyncCorrection)
								{
//									VSyncOffset0 = -SyncOffset;
									VSyncOffset0 = -SyncOffset;
									bDoVSyncCorrection = true;
								}

								// Paint and prepare for next frame
								TRACE_EVR ("Normalframe\n");
								m_nDroppedUpdate = 0;
								bStepForward = true;
								pMFSample->GetUINT32(GUID_SURFACE_INDEX, (UINT32 *)&m_nCurSurface);
								m_LastFrameDuration = nsSampleTime - m_LastSampleTime;
								m_LastSampleTime = nsSampleTime;
								m_LastPredictedSync = VSyncOffset0;

								++m_OrderedPaint;

								if (!g_bExternalSubtitleTime)
									__super::SetTime (g_tSegmentStart + nsSampleTime);
								Paint(true);
								//m_pSink->Notify(EC_SCRUB_TIME, LODWORD(nsSampleTime), HIDWORD(nsSampleTime));
								
								NextSleepTime = 0;
								m_pcFramesDrawn++;
							}
							else
							{
								if (TimeToNextVSync >= 0 && SyncOffset > 0)
								{
									NextSleepTime = ((TimeToNextVSync)/10000) - 2;
								}
								else
									NextSleepTime = ((SyncOffset)/10000) - 2;

								if (NextSleepTime > TimePerFrame)
									NextSleepTime = 1;

								if (NextSleepTime < 0)
									NextSleepTime = 0;
								NextSleepTime = 1;
								//TRACE_EVR ("Delay\n");
							}

							if (bDoVSyncCorrection)
							{
								//LONGLONG VSyncOffset0 = (((SyncOffset) % VSyncTime) + VSyncTime) % VSyncTime;
								LONGLONG Margin = TimePerFrameMargin;

								LONGLONG VSyncOffsetMin = 30000000000000;
								LONGLONG VSyncOffsetMax = -30000000000000;
								for (int i = 0; i < 5; ++i)
								{
									VSyncOffsetMin = min(m_VSyncOffsetHistory[i], VSyncOffsetMin);
									VSyncOffsetMax = max(m_VSyncOffsetHistory[i], VSyncOffsetMax);
								}

								m_VSyncOffsetHistory[m_VSyncOffsetHistoryPos] = VSyncOffset0;
								m_VSyncOffsetHistoryPos = (m_VSyncOffsetHistoryPos + 1) % 5;

//								LONGLONG VSyncTime2 = VSyncTime2 + (VSyncOffsetMax - VSyncOffsetMin);
								//VSyncOffsetMin; = (((VSyncOffsetMin) % VSyncTime) + VSyncTime) % VSyncTime;
								//VSyncOffsetMax = (((VSyncOffsetMax) % VSyncTime) + VSyncTime) % VSyncTime;

//								TRACE("SyncOffset(%d, %d): %8I64d     %8I64d     %8I64d     %8I64d\n", m_nCurSurface, m_VSyncMode,VSyncOffset0, VSyncOffsetMin, VSyncOffsetMax, VSyncOffsetMax - VSyncOffsetMin);

								if (m_VSyncMode == 0)
								{
									// 23.976 in 60 Hz
									if (VSyncOffset0 < Margin && VSyncOffsetMax > (VSyncTime - Margin))
									{
										m_VSyncMode = 2;
									}
									else if (VSyncOffset0 > (VSyncTime - Margin) && VSyncOffsetMin < Margin)
									{
										m_VSyncMode = 1;
									}
								}
								else if (m_VSyncMode == 2)
								{
									if (VSyncOffsetMin > (Margin))
									{
										m_VSyncMode = 0;
									}
								}
								else if (m_VSyncMode == 1)
								{
									if (VSyncOffsetMax < (VSyncTime - Margin))
									{
										m_VSyncMode = 0;
									}
								}
							}

						}
					}

					m_pCurrentDisplaydSample = NULL;
					if (bStepForward)
					{
						MoveToFreeList(pMFSample, true);
						CheckWaitingSampleFromMixer();
						m_MaxSampleDuration = max(SampleDuration, m_MaxSampleDuration);
					}
					else
						MoveToScheduledList(pMFSample, true);
				}
				else if (m_bLastSampleOffsetValid && m_LastSampleOffset < -10000000) // Only starve if we are 1 seconds behind
				{
					if (m_nRenderState == Started && !g_bNoDuration)
					{
						m_pSink->Notify(EC_STARVATION, 0, 0);
						m_bSignaledStarvation = true;
					}
				}
				//GetImageFromMixer();
			}
//			else
//			{				
//				TRACE_EVR ("RenderThread ==>> Flush before rendering frame!\n");
//			}

			break;
		}
	}

	timeEndPeriod (dwResolution);
	if (pfAvRevertMmThreadCharacteristics) pfAvRevertMmThreadCharacteristics (hAvrt);
}

void CEVRAllocatorPresenter::OnResetDevice()
{
	HRESULT		hr;

	// Reset DXVA Manager, and get new buffers
	hr = m_pD3DManager->ResetDevice(m_pD3DDev, m_nResetToken);

	// Not necessary, but Microsoft documentation say Presenter should send this message...
	if (m_pSink)
		m_pSink->Notify (EC_DISPLAY_CHANGED, 0, 0);
}



void CEVRAllocatorPresenter::RemoveAllSamples()
{
	CAutoLock AutoLock(&m_ImageProcessingLock);

	FlushSamples();
	m_ScheduledSamples.RemoveAll();
	m_FreeSamples.RemoveAll();
	m_LastScheduledSampleTime = -1;
	m_LastScheduledUncorrectedSampleTime = -1;
	ASSERT(m_nUsedBuffer == 0);
	m_nUsedBuffer = 0;
}

HRESULT CEVRAllocatorPresenter::GetFreeSample(IMFSample** ppSample)
{
	CAutoLock lock(&m_SampleQueueLock);
	HRESULT		hr = S_OK;

	if (m_FreeSamples.GetCount() > 1)	// <= Cannot use first free buffer (can be currently displayed)
	{
		InterlockedIncrement (&m_nUsedBuffer);
		*ppSample = m_FreeSamples.RemoveHead().Detach();
	}
	else
		hr = MF_E_SAMPLEALLOCATOR_EMPTY;

	return hr;
}


HRESULT CEVRAllocatorPresenter::GetScheduledSample(IMFSample** ppSample, int &_Count)
{
	CAutoLock lock(&m_SampleQueueLock);
	HRESULT		hr = S_OK;

	_Count = m_ScheduledSamples.GetCount();
	if (_Count > 0)
	{
		*ppSample = m_ScheduledSamples.RemoveHead().Detach();
		--_Count;
	}
	else
		hr = MF_E_SAMPLEALLOCATOR_EMPTY;

	return hr;
}


void CEVRAllocatorPresenter::MoveToFreeList(IMFSample* pSample, bool bTail)
{
	CAutoLock lock(&m_SampleQueueLock);
	InterlockedDecrement (&m_nUsedBuffer);
	if (m_bPendingMediaFinished && m_nUsedBuffer == 0)
	{
		m_bPendingMediaFinished = false;
		m_pSink->Notify (EC_COMPLETE, 0, 0);
	}
	if (bTail)
		m_FreeSamples.AddTail (pSample);
	else
		m_FreeSamples.AddHead(pSample);
}


void CEVRAllocatorPresenter::MoveToScheduledList(IMFSample* pSample, bool _bSorted)
{

	if (_bSorted)
	{
		CAutoLock lock(&m_SampleQueueLock);
		// Insert sorted
/*		POSITION Iterator = m_ScheduledSamples.GetHeadPosition();

		LONGLONG NewSampleTime;
		pSample->GetSampleTime(&NewSampleTime);

		while (Iterator != NULL)
		{
			POSITION CurrentPos = Iterator;
			IMFSample *pIter = m_ScheduledSamples.GetNext(Iterator);
			LONGLONG SampleTime;
			pIter->GetSampleTime(&SampleTime);
			if (NewSampleTime < SampleTime)
			{
				m_ScheduledSamples.InsertBefore(CurrentPos, pSample);
				return;
			}
		}*/

		m_ScheduledSamples.AddHead(pSample);
	}
	else
	{

		CAutoLock lock(&m_SampleQueueLock);

		AppSettings& s = AfxGetAppSettings();
		double ForceFPS = 0.0;
//		double ForceFPS = 59.94;
//		double ForceFPS = 23.976;
		if (ForceFPS != 0.0)
			m_rtTimePerFrame = 10000000.0 / ForceFPS;
		LONGLONG Duration = m_rtTimePerFrame;
		LONGLONG PrevTime = m_LastScheduledUncorrectedSampleTime;
		LONGLONG Time;
		LONGLONG SetDuration;
		pSample->GetSampleDuration(&SetDuration);
		pSample->GetSampleTime(&Time);
		m_LastScheduledUncorrectedSampleTime = Time;

		m_bCorrectedFrameTime = false;
		double LastFP = m_LastScheduledSampleTimeFP;

		LONGLONG Diff2 = PrevTime - m_LastScheduledSampleTimeFP*10000000.0;
		LONGLONG Diff = Time - PrevTime;
		if (PrevTime == -1)
			Diff = 0;
		if (Diff < 0)
			Diff = -Diff;
		if (Diff2 < 0)
			Diff2 = -Diff2;
		if (Diff < m_rtTimePerFrame*8 && m_rtTimePerFrame && Diff2 < m_rtTimePerFrame*8) // Detect seeking
		{
			int iPos = (m_DetectedFrameTimePos++) % 60;
			LONGLONG Diff = Time - PrevTime;
			if (PrevTime == -1)
				Diff = 0;
			m_DetectedFrameTimeHistory[iPos] = Diff;
		
			if (m_DetectedFrameTimePos >= 10)
			{
				int nFrames = min(m_DetectedFrameTimePos, 60);
				LONGLONG DectedSum = 0;
				for (int i = 0; i < nFrames; ++i)
				{
					DectedSum += m_DetectedFrameTimeHistory[i];
				}

				double Average = double(DectedSum) / double(nFrames);
				double DeviationSum = 0.0;
				for (int i = 0; i < nFrames; ++i)
				{
					double Deviation = m_DetectedFrameTimeHistory[i] - Average;
					DeviationSum += Deviation*Deviation;
				}
		
				double StdDev = sqrt(DeviationSum/double(nFrames));

				m_DetectedFrameTimeStdDev = StdDev;

				double DetectedRate = 1.0/ (double(DectedSum) / (nFrames * 10000000.0) );

				double AllowedError = 0.0003;

				static double AllowedValues[] = {60.0, 59.94, 50.0, 48.0, 47.952, 30.0, 29.97, 25.0, 24.0, 23.976};

				int nAllowed = sizeof(AllowedValues) / sizeof(AllowedValues[0]);
				for (int i = 0; i < nAllowed; ++i)
				{
					if (fabs(1.0 - DetectedRate / AllowedValues[i]) < AllowedError)
					{
						DetectedRate = AllowedValues[i];
						break;
					}
				}

				m_DetectedFrameTimeHistoryHisotry[m_DetectedFrameTimePos % 500] = DetectedRate;

				class CAutoInt
				{
				public:

					int m_Int;

					CAutoInt()
					{
						m_Int = 0;
					}
					CAutoInt(int _Other)
					{
						m_Int = _Other;
					}

					operator int ()
					{
						return m_Int;
					}

					CAutoInt &operator ++ ()
					{
						++m_Int;
						return *this;
					}
				};


				CMap<double, double, CAutoInt, CAutoInt> Map;

				for (int i = 0; i < 500; ++i)
				{
					++Map[m_DetectedFrameTimeHistoryHisotry[i]];
				}

				POSITION Pos = Map.GetStartPosition();
				double BestVal = 0.0;
				int BestNum = 5;
				while (Pos)
				{
					double Key;
					CAutoInt Value;
					Map.GetNextAssoc(Pos, Key, Value);
					if (Value.m_Int > BestNum && Key != 0.0)
					{
						BestNum = Value.m_Int;
						BestVal = Key;
					}
				}

				m_DetectedLock = false;
				for (int i = 0; i < nAllowed; ++i)
				{
					if (BestVal == AllowedValues[i])
					{
						m_DetectedLock = true;
						break;
					}
				}
				if (BestVal != 0.0)
				{
					m_DetectedFrameRate = BestVal;
					m_DetectedFrameTime = 1.0 / BestVal;
				}
			}

			LONGLONG PredictedNext = PrevTime + m_rtTimePerFrame;
			LONGLONG PredictedDiff = PredictedNext - Time;
			if (PredictedDiff < 0)
				PredictedDiff = -PredictedDiff;

			if (m_DetectedFrameTime != 0.0 
				//&& PredictedDiff > 15000 
				&& m_DetectedLock && s.m_RenderSettings.iEVREnableFrameTimeCorrection)
			{
				double CurrentTime = Time / 10000000.0;
				double LastTime = m_LastScheduledSampleTimeFP;
				double PredictedTime = LastTime + m_DetectedFrameTime;
				if (fabs(PredictedTime - CurrentTime) > 0.0015) // 1.5 ms wrong, lets correct
				{
					CurrentTime = PredictedTime;
					Time = CurrentTime * 10000000.0;
					pSample->SetSampleTime(Time);
					pSample->SetSampleDuration(m_DetectedFrameTime * 10000000.0);
					m_bCorrectedFrameTime = true;
					m_FrameTimeCorrection = 30;
				}
				m_LastScheduledSampleTimeFP = CurrentTime;;
			}
			else
				m_LastScheduledSampleTimeFP = Time / 10000000.0;
		}
		else
		{
			m_LastScheduledSampleTimeFP = Time / 10000000.0;
			if (Diff > m_rtTimePerFrame*8)
			{
				// Seek
				m_bSignaledStarvation = false;
				m_DetectedFrameTimePos = 0;
				m_DetectedLock = false;
			}
		}

//		TRACE("Time: %f %f %f\n", Time / 10000000.0, SetDuration / 10000000.0, m_DetectedFrameRate);
		if (!m_bCorrectedFrameTime && m_FrameTimeCorrection)
			--m_FrameTimeCorrection;

#if 0
		if (Time <= m_LastScheduledUncorrectedSampleTime && m_LastScheduledSampleTime >= 0)
			PrevTime = m_LastScheduledSampleTime;

		m_bCorrectedFrameTime = false;
		if (PrevTime != -1 && (Time >= PrevTime - ((Duration*20)/9) || Time == 0) || ForceFPS != 0.0)
		{
			if (Time - PrevTime > ((Duration*20)/9) && Time - PrevTime < Duration * 8 || Time == 0 || ((Time - PrevTime) < (Duration / 11)) || ForceFPS != 0.0)
			{
				// Error!!!!
				Time = PrevTime + Duration;
				pSample->SetSampleTime(Time);
				pSample->SetSampleDuration(Duration);
				m_bCorrectedFrameTime = true;
				TRACE("Corrected invalid sample time\n");
			}
		}
		if (Time+Duration*10 < m_LastScheduledSampleTime)
		{
			// Flush when repeating movie
			FlushSamplesInternal();
		}
#endif

#if 0
		static LONGLONG LastDuration = 0;
		LONGLONG SetDuration = m_rtTimePerFrame;
		pSample->GetSampleDuration(&SetDuration);
		if (SetDuration != LastDuration)
		{
			TRACE("Old duration: %I64d New duration: %I64d\n", LastDuration, SetDuration);
		}
		LastDuration = SetDuration;
#endif
		m_LastScheduledSampleTime = Time;

		m_ScheduledSamples.AddTail(pSample);

	}
}



void CEVRAllocatorPresenter::FlushSamples()
{
	CAutoLock				lock(this);
	CAutoLock				lock2(&m_SampleQueueLock);
	
	FlushSamplesInternal();
	m_LastScheduledSampleTime = -1;
}

void CEVRAllocatorPresenter::FlushSamplesInternal()
{
	while (m_ScheduledSamples.GetCount() > 0)
	{
		CComPtr<IMFSample>		pMFSample;

		pMFSample = m_ScheduledSamples.RemoveHead();
		MoveToFreeList (pMFSample, true);
	}

	m_LastSampleOffset			= 0;
	m_bLastSampleOffsetValid	= false;
	m_bSignaledStarvation = false;
}
