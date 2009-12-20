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

#ifdef REGISTER_FILTER
#include <initguid.h>
#endif
#include <moreuuids.h>
#include "..\..\..\DSUtil\DSUtil.h"
#include <ks.h>
#include <ksmedia.h>

#include "MpcAudioRenderer.h"


// === Compatibility with Windows SDK v6.0A (define in KSMedia.h in Windows 7 SDK or later)
#ifndef STATIC_KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL

#define STATIC_KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL\
    DEFINE_WAVEFORMATEX_GUID(WAVE_FORMAT_DOLBY_AC3_SPDIF)
DEFINE_GUIDSTRUCT("00000092-0000-0010-8000-00aa00389b71", KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL);
#define KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL)

#define STATIC_KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS \
    0x0000000aL, 0x0cea, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
DEFINE_GUIDSTRUCT("0000000a-0cea-0010-8000-00aa00389b71", KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS);
#define KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS)

#define STATIC_KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP \
    0x0000000cL, 0x0cea, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
DEFINE_GUIDSTRUCT("0000000c-0cea-0010-8000-00aa00389b71", KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP);
#define KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP)

#define STATIC_KSDATAFORMAT_SUBTYPE_IEC61937_DTS\
    DEFINE_WAVEFORMATEX_GUID(WAVE_FORMAT_DTS)
DEFINE_GUIDSTRUCT("00000008-0000-0010-8000-00aa00389b71", KSDATAFORMAT_SUBTYPE_IEC61937_DTS);
#define KSDATAFORMAT_SUBTYPE_IEC61937_DTS DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_IEC61937_DTS)

#define STATIC_KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD \
    0x0000000bL, 0x0cea, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
DEFINE_GUIDSTRUCT("0000000b-0cea-0010-8000-00aa00389b71", KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD);
#define KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD)

#endif

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
 {&GUID_NULL},
};

const AMOVIESETUP_PIN sudpPins[] =
{
    {L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, countof(sudPinTypesIn), sudPinTypesIn},
};

const AMOVIESETUP_FILTER sudFilter[] =
{
	{&__uuidof(CMpcAudioRenderer), L"MPC - Audio Renderer", 0x40000001, countof(sudpPins), sudpPins,CLSID_AudioRendererCategory},
};

CFactoryTemplate g_Templates[] =
{
    {sudFilter[0].strName, &__uuidof(CMpcAudioRenderer), CreateInstance<CMpcAudioRenderer>, NULL, &sudFilter[0]},
	//{L"CMpcAudioRendererPropertyPage", &__uuidof(CMpcAudioRendererSettingsWnd), CreateInstance<CInternalPropertyPageTempl<CMpcAudioRendererSettingsWnd> >},
};

int g_cTemplates = countof(g_Templates);

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

#include "..\..\FilterApp.h"

CFilterApp theApp;

#endif

CMpcAudioRenderer::CMpcAudioRenderer(LPUNKNOWN punk, HRESULT *phr)
: CBaseRenderer(__uuidof(this), NAME("MPC - Audio Renderer"), punk, phr)
, m_pDSBuffer		(NULL )
, m_pSoundTouch (NULL )
, m_pDS				(NULL )
, m_dwDSWriteOff	(0	  )
, m_nDSBufSize		(0	  )
, m_dRate			(1.0  )
, m_pReferenceClock	(NULL )
, m_pWaveFileFormat	(NULL )
, pMMDevice (NULL)
, pAudioClient (NULL )
, pRenderClient (NULL )
, useWASAPI (true )
, nFramesInBuffer (0 )
, hnsPeriod (0 )
, hTask (NULL )
, bufferSize (0)
, isAudioClientStarted (false)
, lastBufferTime (0)
, hnsActualDuration (0)
{
	HMODULE		hLib;

	// Load Vista specifics DLLs
	hLib = LoadLibrary (L"AVRT.dll");
	if (hLib != NULL)
	{
		pfAvSetMmThreadCharacteristicsW		= (PTR_AvSetMmThreadCharacteristicsW)	GetProcAddress (hLib, "AvSetMmThreadCharacteristicsW");
		pfAvRevertMmThreadCharacteristics	= (PTR_AvRevertMmThreadCharacteristics)	GetProcAddress (hLib, "AvRevertMmThreadCharacteristics");
	}
	else
		useWASAPI = false;	// Wasapi not available below Vista

	TRACE(_T("CMpcAudioRenderer constructor"));
	if (!useWASAPI)
	{
		m_pSoundTouch	= new soundtouch::SoundTouch();
		*phr = DirectSoundCreate8 (NULL, &m_pDS, NULL);
	}
}


CMpcAudioRenderer::~CMpcAudioRenderer()
{
	Stop();

	SAFE_DELETE  (m_pSoundTouch);
	SAFE_RELEASE (m_pDSBuffer);
	SAFE_RELEASE (m_pDS);

	SAFE_RELEASE (pRenderClient);
	SAFE_RELEASE (pAudioClient);
	SAFE_RELEASE (pMMDevice);
	
	if (m_pReferenceClock)
	{
		SetSyncSource(NULL);
		SAFE_RELEASE (m_pReferenceClock);
	}

	if (m_pWaveFileFormat)
	{
		BYTE *p = (BYTE *)m_pWaveFileFormat;
		SAFE_DELETE_ARRAY(p);
	}

	if (hTask != NULL && pfAvRevertMmThreadCharacteristics != NULL)
	{
		pfAvRevertMmThreadCharacteristics(hTask);
	}
}


HRESULT CMpcAudioRenderer::CheckInputType(const CMediaType *pmt)
{
	return CheckMediaType(pmt);
}

HRESULT	CMpcAudioRenderer::CheckMediaType(const CMediaType *pmt)
{
 HRESULT hr = S_OK;
 if (pmt == NULL) return E_INVALIDARG;
 TRACE(_T("CMpcAudioRenderer::CheckMediaType"));
 WAVEFORMATEX *pwfx = (WAVEFORMATEX *) pmt->Format();

 if (pwfx == NULL) return VFW_E_TYPE_NOT_ACCEPTED;

if ((pmt->majortype		!= MEDIATYPE_Audio		) ||
  (pmt->formattype	!= FORMAT_WaveFormatEx	))
 {
  TRACE(_T("CMpcAudioRenderer::CheckMediaType Not supported"));
  return VFW_E_TYPE_NOT_ACCEPTED;
 }

 if(useWASAPI)
 {
  hr=CheckAudioClient((WAVEFORMATEX *)NULL);
  if (FAILED(hr))
  {
   TRACE(_T("CMpcAudioRenderer::CheckMediaType Error on check audio client"));
   return hr;
  }
  if (!pAudioClient) 
  {
   TRACE(_T("CMpcAudioRenderer::CheckMediaType Error, audio client not loaded"));
   return VFW_E_CANNOT_CONNECT;
  }

  if (pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, pwfx, NULL) != S_OK)
  {
   TRACE(_T("CMpcAudioRenderer::CheckMediaType WASAPI client refused the format"));
   return VFW_E_TYPE_NOT_ACCEPTED;
  }
  TRACE(_T("CMpcAudioRenderer::CheckMediaType WASAPI client accepted the format"));
 }
 else if	(pwfx->wFormatTag	!= WAVE_FORMAT_PCM)
 {
  return VFW_E_TYPE_NOT_ACCEPTED;
 }
	return S_OK;
}

void CMpcAudioRenderer::OnReceiveFirstSample(IMediaSample *pMediaSample)
{
 if (!useWASAPI)
	 ClearBuffer();
}

BOOL CMpcAudioRenderer::ScheduleSample(IMediaSample *pMediaSample)
{
	REFERENCE_TIME		StartSample;
	REFERENCE_TIME		EndSample;

 // Is someone pulling our leg
 if (pMediaSample == NULL) return FALSE;

 // Get the next sample due up for rendering.  If there aren't any ready
 // then GetNextSampleTimes returns an error.  If there is one to be done
 // then it succeeds and yields the sample times. If it is due now then
 // it returns S_OK other if it's to be done when due it returns S_FALSE
 HRESULT hr = GetSampleTimes(pMediaSample, &StartSample, &EndSample);
 if (FAILED(hr)) return FALSE;

 // If we don't have a reference clock then we cannot set up the advise
 // time so we simply set the event indicating an image to render. This
 // will cause us to run flat out without any timing or synchronisation
 if (hr == S_OK) 
	{
		EXECUTE_ASSERT(SetEvent((HANDLE) m_RenderEvent));
		return TRUE;
 }

 if (m_dRate <= 1.1)
 {
	 ASSERT(m_dwAdvise == 0);
	 ASSERT(m_pClock);
	 WaitForSingleObject((HANDLE)m_RenderEvent,0);

	 hr = m_pClock->AdviseTime( (REFERENCE_TIME) m_tStart, StartSample, (HEVENT)(HANDLE) m_RenderEvent, &m_dwAdvise);
	 if (SUCCEEDED(hr)) 	return TRUE;
 }
 else
	 hr = DoRenderSample (pMediaSample);

 // We could not schedule the next sample for rendering despite the fact
 // we have a valid sample here. This is a fair indication that either
 // the system clock is wrong or the time stamp for the sample is duff
 ASSERT(m_dwAdvise == 0);

 return FALSE;
}

HRESULT	CMpcAudioRenderer::DoRenderSample(IMediaSample *pMediaSample)
{
 if (useWASAPI)
  return DoRenderSampleWasapi(pMediaSample);
 else
	 return DoRenderSampleDirectSound(pMediaSample);
}


STDMETHODIMP CMpcAudioRenderer::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IReferenceClock)
	{
		return GetReferenceClockInterface(riid, ppv);
	}

	return CBaseRenderer::NonDelegatingQueryInterface (riid, ppv);
}

HRESULT CMpcAudioRenderer::SetMediaType(const CMediaType *pmt)
{
	if (! pmt) return E_POINTER;
 HRESULT hr = S_OK;
 int				size = 0;
 TRACE(_T("CMpcAudioRenderer::SetMediaType"));

 if (useWASAPI)
 {
  // New media type set but render client already initialized => reset it
  if (pRenderClient!=NULL)
  {
    WAVEFORMATEX	*pNewWf	= (WAVEFORMATEX *) pmt->Format();
    TRACE(_T("CMpcAudioRenderer::SetMediaType Render client already initialized. Reinitialization..."));
    CheckAudioClient(pNewWf);
  }
 }

 if (m_pWaveFileFormat)
	{
		BYTE *p = (BYTE *)m_pWaveFileFormat;
		SAFE_DELETE_ARRAY(p);
	}
 m_pWaveFileFormat=NULL;

 WAVEFORMATEX	*pwf	= (WAVEFORMATEX *) pmt->Format();
 if (pwf!=NULL)
 {
  size	= sizeof(WAVEFORMATEX) + pwf->cbSize;

	 m_pWaveFileFormat = (WAVEFORMATEX *)new BYTE[size];
	 if (! m_pWaveFileFormat)
		 return E_OUTOFMEMORY;
  
  memcpy(m_pWaveFileFormat, pwf, size);


  if (!useWASAPI && m_pSoundTouch && (pwf->nChannels <= 2))
	 {
		 m_pSoundTouch->setSampleRate (pwf->nSamplesPerSec);
		 m_pSoundTouch->setChannels (pwf->nChannels);
		 m_pSoundTouch->setTempoChange (0);
		 m_pSoundTouch->setPitchSemiTones(0);
	 }
 }

	return CBaseRenderer::SetMediaType (pmt);
}

HRESULT CMpcAudioRenderer::CompleteConnect(IPin *pReceivePin)
{
	HRESULT			hr = S_OK;
 TRACE(_T("CMpcAudioRenderer::CompleteConnect"));

 if (!useWASAPI && ! m_pDS) return E_FAIL;

	if (SUCCEEDED(hr)) hr = CBaseRenderer::CompleteConnect(pReceivePin);
	if (SUCCEEDED(hr)) hr = InitCoopLevel();

	if (!useWASAPI)
 {
  if (SUCCEEDED(hr)) hr = CreateDSBuffer();
 }
 if (SUCCEEDED(hr)) TRACE(_T("CMpcAudioRenderer::CompleteConnect Success"));
	return hr;

}

STDMETHODIMP CMpcAudioRenderer::Run(REFERENCE_TIME tStart)
{
	HRESULT		hr;

 if (m_State == State_Running) return NOERROR;

 if (useWASAPI)
 {
  hr=CheckAudioClient(m_pWaveFileFormat);
  if (FAILED(hr)) 
  {
   TRACE(_T("CMpcAudioRenderer::Run Error on check audio client"));
   return hr;
  }
  // Rather start the client at the last moment when the buffer is fed
  /*hr = pAudioClient->Start();
  if (FAILED (hr))
  {
   TRACE(_T("CMpcAudioRenderer::Run Start error"));
   return hr;
  }*/
 }
 else
 {
  if (m_pDSBuffer && 
		 m_pPosition &&
		 m_pWaveFileFormat && 
		 SUCCEEDED(m_pPosition->GetRate(&m_dRate))) 
	 {
		 if (m_dRate < 1.0)
		 {
			 hr = m_pDSBuffer->SetFrequency ((long)(m_pWaveFileFormat->nSamplesPerSec * m_dRate));
			 if (FAILED (hr)) return hr;
		 }
		 else
		 {
			 hr = m_pDSBuffer->SetFrequency ((long)m_pWaveFileFormat->nSamplesPerSec);
			 m_pSoundTouch->setRateChange((float)(m_dRate-1.0)*100);
		 }
	 }

	 ClearBuffer();
 }
	hr = CBaseRenderer::Run(tStart);

	return hr;
}

STDMETHODIMP CMpcAudioRenderer::Stop() 
{
	if (m_pDSBuffer) m_pDSBuffer->Stop();
 isAudioClientStarted=false;

 return CBaseRenderer::Stop(); 
};


STDMETHODIMP CMpcAudioRenderer::Pause()
{
	if (m_pDSBuffer) m_pDSBuffer->Stop();
 if (pAudioClient && isAudioClientStarted) pAudioClient->Stop();
 isAudioClientStarted=false;

	return CBaseRenderer::Pause(); 
};


HRESULT CMpcAudioRenderer::GetReferenceClockInterface(REFIID riid, void **ppv)
{
	HRESULT hr = S_OK;

	if (m_pReferenceClock)
		return m_pReferenceClock->NonDelegatingQueryInterface(riid, ppv);

	m_pReferenceClock = new CBaseReferenceClock (NAME("Mpc Audio Clock"), NULL, &hr);
	if (! m_pReferenceClock)
		return E_OUTOFMEMORY;

	m_pReferenceClock->AddRef();

	hr = SetSyncSource(m_pReferenceClock);
	if (FAILED(hr)) 
	{
		SetSyncSource(NULL);
		return hr;
	}

	return GetReferenceClockInterface(riid, ppv);
}



HRESULT CMpcAudioRenderer::EndOfStream(void)
{
 if (m_pDSBuffer) m_pDSBuffer->Stop();
#if !FILEWRITER
 if (pAudioClient && isAudioClientStarted) pAudioClient->Stop();
#endif
 isAudioClientStarted=false;

	return CBaseRenderer::EndOfStream();
}



#pragma region // ==== DirectSound

HRESULT CMpcAudioRenderer::CreateDSBuffer()
{
	if (! m_pWaveFileFormat) return E_POINTER;

	HRESULT					hr				= S_OK;
	LPDIRECTSOUNDBUFFER		pDSBPrimary		= NULL;
	DSBUFFERDESC			dsbd;
	DSBUFFERDESC			cDSBufferDesc;
	DSBCAPS					bufferCaps;
	DWORD					dwDSBufSize		= m_pWaveFileFormat->nAvgBytesPerSec * 4;

	ZeroMemory(&bufferCaps, sizeof(bufferCaps));
	ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));

	dsbd.dwSize        = sizeof(DSBUFFERDESC);
	dsbd.dwFlags       = DSBCAPS_PRIMARYBUFFER;
	dsbd.dwBufferBytes = 0;
	dsbd.lpwfxFormat   = NULL;
	if (SUCCEEDED (hr = m_pDS->CreateSoundBuffer( &dsbd, &pDSBPrimary, NULL )))
	{
		hr = pDSBPrimary->SetFormat(m_pWaveFileFormat);
		ATLASSERT(SUCCEEDED(hr));
		SAFE_RELEASE (pDSBPrimary);
	}


	SAFE_RELEASE (m_pDSBuffer);
	cDSBufferDesc.dwSize			= sizeof (DSBUFFERDESC);
	cDSBufferDesc.dwFlags			= DSBCAPS_GLOBALFOCUS			| 
									  DSBCAPS_GETCURRENTPOSITION2	| 
									  DSBCAPS_CTRLVOLUME 			|
									  DSBCAPS_CTRLPAN				|
									  DSBCAPS_CTRLFREQUENCY; 
	cDSBufferDesc.dwBufferBytes		= dwDSBufSize; 
	cDSBufferDesc.dwReserved		= 0; 
	cDSBufferDesc.lpwfxFormat		= m_pWaveFileFormat; 
   	cDSBufferDesc.guid3DAlgorithm	= GUID_NULL; 

	hr = m_pDS->CreateSoundBuffer (&cDSBufferDesc,  &m_pDSBuffer, NULL);

	m_nDSBufSize = 0;
	if (SUCCEEDED(hr))
	{
		bufferCaps.dwSize = sizeof(bufferCaps);
		hr = m_pDSBuffer->GetCaps(&bufferCaps);
	}
	if (SUCCEEDED (hr))
	{
		m_nDSBufSize = bufferCaps.dwBufferBytes;
		hr = ClearBuffer();
		m_pDSBuffer->SetFrequency ((long)(m_pWaveFileFormat->nSamplesPerSec * m_dRate));
	}

	return hr;
}



HRESULT CMpcAudioRenderer::ClearBuffer()
{
	HRESULT			hr				 = S_FALSE;
	VOID*			pDSLockedBuffer  = NULL;
	DWORD			dwDSLockedSize   = 0;

	if (m_pDSBuffer)
	{
		m_dwDSWriteOff = 0;
		m_pDSBuffer->SetCurrentPosition(0);

		hr = m_pDSBuffer->Lock (0, 0, &pDSLockedBuffer, &dwDSLockedSize, NULL, NULL, DSBLOCK_ENTIREBUFFER);
		if (SUCCEEDED (hr))
		{
			memset (pDSLockedBuffer, 0, dwDSLockedSize);
			hr = m_pDSBuffer->Unlock (pDSLockedBuffer, dwDSLockedSize, NULL, NULL);
		}
	}

	return hr;
}

HRESULT CMpcAudioRenderer::InitCoopLevel()
{
	HRESULT				hr				= S_OK;
	IVideoWindow*		pVideoWindow	= NULL;
	HWND				hWnd			= NULL;
	CComBSTR			bstrCaption;

	hr = m_pGraph->QueryInterface (__uuidof(IVideoWindow), (void**) &pVideoWindow);
	if (SUCCEEDED (hr))
	{
		pVideoWindow->get_Owner((OAHWND*)&hWnd);
		SAFE_RELEASE (pVideoWindow);
	}
	if (!hWnd) 
	{
		hWnd = GetTopWindow(NULL);
	}

	ATLASSERT(hWnd != NULL);
	if (!useWASAPI)
		hr = m_pDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY);
	else if (hTask == NULL)
	{
		// Ask MMCSS to temporarily boost the thread priority
		// to reduce glitches while the low-latency stream plays.
		DWORD taskIndex = 0;

		if (pfAvSetMmThreadCharacteristicsW)
		{
			hTask = pfAvSetMmThreadCharacteristicsW(TEXT("Pro Audio"), &taskIndex);
			TRACE(_T("CMpcAudioRenderer::InitCoopLevel Putting thread in higher priority for Wasapi mode (lowest latency)"));
			hr=GetLastError();
			if (hTask == NULL)
				return hr;
		}
	}

	return hr;
}

HRESULT	CMpcAudioRenderer::DoRenderSampleDirectSound(IMediaSample *pMediaSample)
{
	HRESULT			hr					= S_OK;
	DWORD			dwStatus			= 0;
	const long		lSize				= pMediaSample->GetActualDataLength();
	DWORD			dwPlayCursor		= 0;
	DWORD			dwWriteCursor		= 0;

	hr = m_pDSBuffer->GetStatus(&dwStatus);

	if (SUCCEEDED(hr) && (dwStatus & DSBSTATUS_BUFFERLOST))
		hr = m_pDSBuffer->Restore();

	if ((SUCCEEDED(hr)) && ((dwStatus & DSBSTATUS_PLAYING) != DSBSTATUS_PLAYING))
	{
		hr = m_pDSBuffer->Play( 0, 0, DSBPLAY_LOOPING);
		ATLASSERT(SUCCEEDED(hr));
	}
	
	if (SUCCEEDED(hr)) hr = m_pDSBuffer->GetCurrentPosition(&dwPlayCursor, &dwWriteCursor);

	if (SUCCEEDED(hr))
	{
		if ( (  (dwPlayCursor < dwWriteCursor)		&&
				(
					((m_dwDSWriteOff >= dwPlayCursor) && (m_dwDSWriteOff <=  dwWriteCursor))
					||
					((m_dwDSWriteOff < dwPlayCursor) && (m_dwDSWriteOff + lSize >= dwPlayCursor))
				)
			 ) 
			 ||
			 (  (dwWriteCursor < dwPlayCursor) && 
				( 
					(m_dwDSWriteOff >= dwPlayCursor) || (m_dwDSWriteOff <  dwWriteCursor)
			    ) ) )
		{
			m_dwDSWriteOff = dwWriteCursor;
		}

		if (m_dwDSWriteOff >= (DWORD)m_nDSBufSize)
		{
			m_dwDSWriteOff = 0;
		}
	}

	if (SUCCEEDED(hr)) hr = WriteSampleToDSBuffer(pMediaSample, NULL);
	
	return hr;
}

HRESULT CMpcAudioRenderer::WriteSampleToDSBuffer(IMediaSample *pMediaSample, bool *looped)
{
	if (! m_pDSBuffer) return E_POINTER;

	REFERENCE_TIME	rtStart				= 0;
	REFERENCE_TIME	rtStop				= 0;
	HRESULT			hr					= S_OK;
	bool			loop				= false;
	VOID*			pDSLockedBuffers[2] = { NULL, NULL };
	DWORD			dwDSLockedSize[2]	= {    0,    0 };
	BYTE*			pMediaBuffer		= NULL;
	long			lSize				= pMediaSample->GetActualDataLength();

	hr = pMediaSample->GetPointer(&pMediaBuffer);

	if (m_dRate > 1.0)
	{
		int		nBytePerSample = m_pWaveFileFormat->nChannels * (m_pWaveFileFormat->wBitsPerSample/8);
		m_pSoundTouch->putSamples((const short*)pMediaBuffer, lSize / nBytePerSample);
		lSize = m_pSoundTouch->receiveSamples ((short*)pMediaBuffer, lSize / nBytePerSample) * nBytePerSample;
	}

	pMediaSample->GetTime (&rtStart, &rtStop);

	if (rtStart < 0)
	{
		DWORD		dwPercent	= (DWORD) ((-rtStart * 100) / (rtStop - rtStart));
		DWORD		dwRemove	= (lSize*dwPercent/100);

		dwRemove		 = (dwRemove / m_pWaveFileFormat->nBlockAlign) * m_pWaveFileFormat->nBlockAlign;
		pMediaBuffer	+= dwRemove;
		lSize			-= dwRemove;
	}

	if (SUCCEEDED (hr))
		hr = m_pDSBuffer->Lock (m_dwDSWriteOff, lSize, &pDSLockedBuffers[0], &dwDSLockedSize[0], &pDSLockedBuffers[1], &dwDSLockedSize[1], 0 );
	if (SUCCEEDED (hr))
	{
		if (pDSLockedBuffers [0] != NULL)
		{
			memcpy(pDSLockedBuffers[0], pMediaBuffer, dwDSLockedSize[0]);
			m_dwDSWriteOff += dwDSLockedSize[0];
		}

		if (pDSLockedBuffers [1] != NULL)
		{
			memcpy(pDSLockedBuffers[1], &pMediaBuffer[dwDSLockedSize[0]], dwDSLockedSize[1]);
			m_dwDSWriteOff = dwDSLockedSize[1];
			loop = true;
		}

		hr = m_pDSBuffer->Unlock(pDSLockedBuffers[0], dwDSLockedSize[0], pDSLockedBuffers[1], dwDSLockedSize[1]);
		ATLASSERT (dwDSLockedSize [0] + dwDSLockedSize [1] == (DWORD)lSize);
	}

	if (SUCCEEDED(hr) && looped) *looped = loop;

	return hr;
}

#pragma endregion

#pragma region // ==== WASAPI

HRESULT	CMpcAudioRenderer::DoRenderSampleWasapi(IMediaSample *pMediaSample)
{
	HRESULT	hr	= S_OK;
 REFERENCE_TIME	rtStart				= 0;
	REFERENCE_TIME	rtStop				= 0;
 DWORD flags = 0;
 BYTE *pMediaBuffer		= NULL;
 BYTE *pInputBufferPointer = NULL;
 BYTE *pInputBufferEnd = NULL;
 BYTE *pData;
 bufferSize = pMediaSample->GetActualDataLength();
 const long	lSize = bufferSize;
 pMediaSample->GetTime (&rtStart, &rtStop);
  
 AM_MEDIA_TYPE *pmt;
 if (SUCCEEDED(pMediaSample->GetMediaType(&pmt)) && pmt!=NULL)
 {
   CMediaType mt(*pmt);
   if ((WAVEFORMATEXTENSIBLE*)mt.Format() != NULL)
    hr=CheckAudioClient(&(((WAVEFORMATEXTENSIBLE*)mt.Format())->Format));
   else
    hr=CheckAudioClient((WAVEFORMATEX*)mt.Format());
   if (FAILED(hr))
   {
    TRACE(_T("CMpcAudioRenderer::DoRenderSampleWasapi Error while checking audio client with input media type"));
    return hr;
   }
   DeleteMediaType(pmt);
   pmt=NULL;
 }

 // Initialization
 hr = pMediaSample->GetPointer(&pMediaBuffer);
 if (FAILED (hr)) return hr; 

 pInputBufferPointer=&pMediaBuffer[0];
 pInputBufferEnd=&pMediaBuffer[0]+lSize;

 WORD frameSize = m_pWaveFileFormat->nBlockAlign;

 
 // Sleep for half the buffer duration since last buffer feed
 DWORD currentTime=GetTickCount();
 if (lastBufferTime!=0 && hnsActualDuration!= 0 && lastBufferTime<currentTime && (currentTime-lastBufferTime)<hnsActualDuration)
 {
  hnsActualDuration=hnsActualDuration-(currentTime-lastBufferTime);
  Sleep(hnsActualDuration);
 }

	// Each loop fills one of the two buffers.
 while (pInputBufferPointer < pInputBufferEnd)
 {  
  UINT32 numFramesPadding=0;
  pAudioClient->GetCurrentPadding(&numFramesPadding);
  UINT32 numFramesAvailable = nFramesInBuffer - numFramesPadding;

  UINT32 nAvailableBytes=numFramesAvailable*frameSize;
  UINT32 nBytesToWrite=nAvailableBytes;
  // More room than enough in the output buffer
  if (nAvailableBytes > pInputBufferEnd - pInputBufferPointer)
  {
   nBytesToWrite=pInputBufferEnd - pInputBufferPointer;
   numFramesAvailable=(UINT32)((float)nBytesToWrite/frameSize);
  }

  // Grab the next empty buffer from the audio device.
  hr = pRenderClient->GetBuffer(numFramesAvailable, &pData);
  if (FAILED (hr))
  {
   TRACE(_T("CMpcAudioRenderer::DoRenderSampleWasapi GetBuffer failed with size %ld : (error %lx)"),nFramesInBuffer,hr);
    return hr;
  }

  // Load the buffer with data from the audio source.
  if (pData != NULL)
		{

		 memcpy(&pData[0], pInputBufferPointer, nBytesToWrite);
		 pInputBufferPointer += nBytesToWrite;
		}
  else
   TRACE(_T("CMpcAudioRenderer::DoRenderSampleWasapi Output buffer is NULL"));

  hr = pRenderClient->ReleaseBuffer(numFramesAvailable, 0); // no flags
  if (FAILED (hr)) 
  {
   TRACE(_T("CMpcAudioRenderer::DoRenderSampleWasapi ReleaseBuffer failed with size %ld (error %lx)"),nFramesInBuffer,hr);
   return hr;
  }

  if (!isAudioClientStarted)
  {
   TRACE(_T("CMpcAudioRenderer::DoRenderSampleWasapi Starting audio client"));
   pAudioClient->Start();
   isAudioClientStarted=true;
  }

  if (pInputBufferPointer >= pInputBufferEnd)
  {
   lastBufferTime=GetTickCount();
   // This is the duration of the filled buffer
   hnsActualDuration=(double)REFTIMES_PER_SEC * numFramesAvailable / m_pWaveFileFormat->nSamplesPerSec;
   // Sleep time is half this duration
   hnsActualDuration=(DWORD)(hnsActualDuration/REFTIMES_PER_MILLISEC/2);
   break;
  }

  // Buffer not completely filled, sleep for half buffer capacity duration
  hnsActualDuration=(double)REFTIMES_PER_SEC * nFramesInBuffer / m_pWaveFileFormat->nSamplesPerSec;
  // Sleep time is half this duration
  hnsActualDuration=(DWORD)(hnsActualDuration/REFTIMES_PER_MILLISEC/2);
  Sleep(hnsActualDuration);
 }
	return hr;
}

HRESULT CMpcAudioRenderer::CheckAudioClient(WAVEFORMATEX *pWaveFormatEx)
{
 HRESULT hr = S_OK;
 CAutoLock cAutoLock(&m_csCheck);
 TRACE(_T("CMpcAudioRenderer::CheckAudioClient"));
 if (pMMDevice == NULL) hr=GetDefaultAudioDevice(&pMMDevice);

 // If no WAVEFORMATEX structure provided and client already exists, return it
 if (pAudioClient != NULL && pWaveFormatEx == NULL) return hr;

 // Just create the audio client if no WAVEFORMATEX provided
 if (pAudioClient == NULL && pWaveFormatEx==NULL)
 {
  if (SUCCEEDED (hr)) hr=CreateAudioClient(pMMDevice, &pAudioClient);
  return hr;
 }
 
 // Compare the exisiting WAVEFORMATEX with the one provided
 WAVEFORMATEX *pNewWaveFormatEx = NULL;
 if (CheckFormatChanged(pWaveFormatEx, &pNewWaveFormatEx))
 {
  // Format has changed, audio client has to be reinitialized
  TRACE(_T("CMpcAudioRenderer::CheckAudioClient Format changed, reinitialize the audio client"));
  if (m_pWaveFileFormat)
	 {
		 BYTE *p = (BYTE *)m_pWaveFileFormat;
		 SAFE_DELETE_ARRAY(p);
	 }
  m_pWaveFileFormat=pNewWaveFormatEx;
  hr = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, pWaveFormatEx, NULL);
  if (SUCCEEDED(hr))
  { 
   if (pAudioClient!=NULL && isAudioClientStarted) pAudioClient->Stop();
   isAudioClientStarted=false;
   SAFE_RELEASE(pRenderClient);
   SAFE_RELEASE(pAudioClient);
   if (SUCCEEDED (hr)) hr=CreateAudioClient(pMMDevice, &pAudioClient);
  }
  else
  {
   TRACE(_T("CMpcAudioRenderer::CheckAudioClient New format not supported, accept it anyway"));
   return S_OK;
  }
 }
 else if (pRenderClient == NULL)
 {
  TRACE(_T("CMpcAudioRenderer::CheckAudioClient First initialization of the audio renderer"));
 }
 else
  return hr;

 
 SAFE_RELEASE(pRenderClient);
 if (SUCCEEDED (hr)) hr=InitAudioClient(pWaveFormatEx, pAudioClient, &pRenderClient);
  return hr;
}

/* Retrieves the default audio device from the Core Audio API
 To be used for WASAPI mode
 TODO : choose a device in the renderer configuration dialogs
*/
HRESULT CMpcAudioRenderer::GetDefaultAudioDevice(IMMDevice **ppMMDevice)
{
	HRESULT hr;
	CComPtr<IMMDeviceEnumerator> enumerator;
 TRACE(_T("CMpcAudioRenderer::GetDefaultAudioDevice"));

	hr = enumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator));
	hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole,
							 ppMMDevice);
		return hr;
}

bool CMpcAudioRenderer::CheckFormatChanged(WAVEFORMATEX *pWaveFormatEx, WAVEFORMATEX **ppNewWaveFormatEx)
{
 bool formatChanged=false;
 if (m_pWaveFileFormat==NULL)
  formatChanged=true;
 else if (pWaveFormatEx->wFormatTag != m_pWaveFileFormat->wFormatTag
  || pWaveFormatEx->nChannels != m_pWaveFileFormat->nChannels
  || pWaveFormatEx->wBitsPerSample != m_pWaveFileFormat->wBitsPerSample) // TODO : improve the checks
  formatChanged=true;


 if (!formatChanged) return false;

 int size	= sizeof(WAVEFORMATEX) + pWaveFormatEx->cbSize; // Always true, even for WAVEFORMATEXTENSIBLE and WAVEFORMATEXTENSIBLE_IEC61937
 *ppNewWaveFormatEx = (WAVEFORMATEX *)new BYTE[size];
	if (! *ppNewWaveFormatEx)
		return false;
 memcpy(*ppNewWaveFormatEx, pWaveFormatEx, size);
 return true;
}

HRESULT CMpcAudioRenderer::GetBufferSize(WAVEFORMATEX *pWaveFormatEx, REFERENCE_TIME *pHnsBufferPeriod)
{ 
 if (pWaveFormatEx==NULL) return S_OK;
 if (pWaveFormatEx->cbSize <22) //WAVEFORMATEX
  return S_OK;

 WAVEFORMATEXTENSIBLE *wfext=(WAVEFORMATEXTENSIBLE*)pWaveFormatEx;

 if (bufferSize==0)
  if (wfext->SubFormat==KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP)
   bufferSize=61440;
  else if (wfext->SubFormat==KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD)
   bufferSize=32768;
  else if (wfext->SubFormat==KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS)
   bufferSize=24576;
  else if (wfext->Format.wFormatTag==WAVE_FORMAT_DOLBY_AC3_SPDIF)
   bufferSize=6144;
 else return S_OK;
 
 *pHnsBufferPeriod = (REFERENCE_TIME)( (REFERENCE_TIME)bufferSize * 10000 * 8 / ((REFERENCE_TIME)pWaveFormatEx->nChannels * pWaveFormatEx->wBitsPerSample *
  1.0 * pWaveFormatEx->nSamplesPerSec) /*+ 0.5*/);
 *pHnsBufferPeriod *= 1000;

 TRACE(_T("CMpcAudioRenderer::GetBufferSize set a %lld period for a %ld buffer size"),*pHnsBufferPeriod,bufferSize);

 return S_OK;
}

HRESULT CMpcAudioRenderer::InitAudioClient(WAVEFORMATEX *pWaveFormatEx, IAudioClient *pAudioClient, IAudioRenderClient **ppRenderClient)
{
 TRACE(_T("CMpcAudioRenderer::InitAudioClient"));
 HRESULT hr=S_OK;
 // Initialize the stream to play at the minimum latency.
 //if (SUCCEEDED (hr)) hr = pAudioClient->GetDevicePeriod(NULL, &hnsPeriod);
 hnsPeriod=500000; //50 ms is the best according to James @Slysoft

 hr = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, pWaveFormatEx, NULL);
 if (FAILED(hr))
  TRACE(_T("CMpcAudioRenderer::InitAudioClient not supported (0x%08x)"), hr);
 else
  TRACE(_T("CMpcAudioRenderer::InitAudioClient format supported"));

 GetBufferSize(pWaveFormatEx, &hnsPeriod);

 if (SUCCEEDED (hr)) hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE,0/*AUDCLNT_STREAMFLAGS_EVENTCALLBACK*/,
         hnsPeriod,hnsPeriod,pWaveFormatEx,NULL);
 if (FAILED (hr) && hr != AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED)
 {
  TRACE(_T("CMpcAudioRenderer::InitAudioClient failed (0x%08x)"), hr);
  return hr;
 }

 if (AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED == hr) 
 {
  // if the buffer size was not aligned, need to do the alignment dance
  TRACE(_T("CMpcAudioRenderer::InitAudioClient Buffer size not aligned. Realigning"));

  // get the buffer size, which will be aligned
  hr = pAudioClient->GetBufferSize(&nFramesInBuffer);

  // throw away this IAudioClient
  pAudioClient->Release();pAudioClient=NULL;

  // calculate the new aligned periodicity
  hnsPeriod = // hns =
   (REFERENCE_TIME)(
   10000.0 * // (hns / ms) *
   1000 * // (ms / s) *
   nFramesInBuffer / // frames /
   pWaveFormatEx->nSamplesPerSec  // (frames / s)
   + 0.5 // rounding
   );

  if (SUCCEEDED (hr)) hr=CreateAudioClient(pMMDevice, &pAudioClient);
  TRACE(_T("CMpcAudioRenderer::InitAudioClient Trying again with periodicity of %I64u hundred-nanoseconds, or %u frames.\n"), hnsPeriod, nFramesInBuffer);
  if (SUCCEEDED (hr)) 
   hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE,0/*AUDCLNT_STREAMFLAGS_EVENTCALLBACK*/,
         hnsPeriod, hnsPeriod, pWaveFormatEx, NULL);
  if (FAILED(hr))
  {
   TRACE(_T("CMpcAudioRenderer::InitAudioClient Failed to reinitialize the audio client"));
   return hr;
  }
 } // if (AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED == hr) 

 // get the buffer size, which is aligned
 if (SUCCEEDED (hr)) hr = pAudioClient->GetBufferSize(&nFramesInBuffer);

 // calculate the new period
 if (SUCCEEDED (hr)) hr = pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)(ppRenderClient));

 if (FAILED (hr))
  TRACE(_T("CMpcAudioRenderer::InitAudioClient service initialization failed (0x%08x)"), hr);
 else
  TRACE(_T("CMpcAudioRenderer::InitAudioClient service initialization success"));

 return hr;
}


HRESULT CMpcAudioRenderer::CreateAudioClient(IMMDevice *pMMDevice, IAudioClient **ppAudioClient)
{
 HRESULT hr = S_OK;
 hnsPeriod = 0;

 TRACE(_T("CMpcAudioRenderer::CreateAudioClient"));

 if (*ppAudioClient)
 {
  if (isAudioClientStarted) (*ppAudioClient)->Stop();
  SAFE_RELEASE(*ppAudioClient);
  isAudioClientStarted=false;
 }

 if (pMMDevice==NULL)
 {
  TRACE(_T("CMpcAudioRenderer::CreateAudioClient failed, device not loaded"));
  return E_FAIL;
 }
 
 hr = pMMDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, reinterpret_cast<void**>(ppAudioClient));
 if (FAILED(hr))
  TRACE(_T("CMpcAudioRenderer::CreateAudioClient activation failed (0x%08x)"), hr);
 else
  TRACE(_T("CMpcAudioRenderer::CreateAudioClient success"));
 return hr;
}

