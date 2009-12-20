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

#pragma once

#include <streams.h>
#include <dsound.h>

#include <MMReg.h>  //must be before other Wasapi headers
#include <strsafe.h>
#include <mmdeviceapi.h>
#include <Avrt.h>
#include <audioclient.h>
#include <Endpointvolume.h>


#include "SoundTouch\Include\SoundTouch.h"

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

// if you get a compilation error on AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED,
// uncomment the #define below
#define AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED      AUDCLNT_ERR(0x019)

[uuid("601D2A2B-9CDE-40bd-8650-0485E3522727")]
class CMpcAudioRenderer : public CBaseRenderer
{
public:
	CMpcAudioRenderer(LPUNKNOWN punk, HRESULT *phr);
	virtual ~CMpcAudioRenderer();

    static const AMOVIESETUP_FILTER sudASFilter;

	HRESULT					CheckInputType			(const CMediaType* mtIn);
	virtual HRESULT			CheckMediaType			(const CMediaType *pmt);
	virtual HRESULT			DoRenderSample			(IMediaSample *pMediaSample);
	virtual void			OnReceiveFirstSample	(IMediaSample *pMediaSample);
	BOOL					ScheduleSample			(IMediaSample *pMediaSample);
	virtual HRESULT			SetMediaType			(const CMediaType *pmt);
    virtual HRESULT			CompleteConnect			(IPin *pReceivePin);

	HRESULT EndOfStream(void);


    DECLARE_IUNKNOWN

	STDMETHODIMP			NonDelegatingQueryInterface(REFIID riid, void **ppv);

	// === IMediaFilter
	STDMETHOD(Run)				(REFERENCE_TIME tStart);
	STDMETHOD(Stop)				();
	STDMETHOD(Pause)			();

 // CMpcAudioRenderer
private:

	HRESULT					DoRenderSampleDirectSound(IMediaSample *pMediaSample);

	HRESULT					InitCoopLevel();
	HRESULT					ClearBuffer();
	HRESULT					CreateDSBuffer();
	HRESULT					GetReferenceClockInterface(REFIID riid, void **ppv);
	HRESULT					WriteSampleToDSBuffer(IMediaSample *pMediaSample, bool *looped);

 LPDIRECTSOUND8			m_pDS;
	LPDIRECTSOUNDBUFFER		m_pDSBuffer;
	DWORD					m_dwDSWriteOff;
	WAVEFORMATEX			*m_pWaveFileFormat;
	int						m_nDSBufSize;
	CBaseReferenceClock*	m_pReferenceClock;
	double					m_dRate;
 soundtouch::SoundTouch*	m_pSoundTouch;

 // CMpcAudioRenderer WASAPI methods
 HRESULT     GetDefaultAudioDevice(IMMDevice **ppMMDevice);
 HRESULT     CreateAudioClient(IMMDevice *pMMDevice, IAudioClient **ppAudioClient);
 HRESULT     InitAudioClient(WAVEFORMATEX *pWaveFormatEx, IAudioClient *pAudioClient, IAudioRenderClient **ppRenderClient);
 HRESULT     CheckAudioClient(WAVEFORMATEX *pWaveFormatEx);
 bool        CheckFormatChanged(WAVEFORMATEX *pWaveFormatEx, WAVEFORMATEX **ppNewWaveFormatEx);
 HRESULT	    DoRenderSampleWasapi(IMediaSample *pMediaSample);
 HRESULT     GetBufferSize(WAVEFORMATEX *pWaveFormatEx, REFERENCE_TIME *pHnsBufferPeriod);

 
 // WASAPI variables
 bool               useWASAPI;
 IMMDevice          *pMMDevice;
 IAudioClient       *pAudioClient;
 IAudioRenderClient *pRenderClient;
 UINT32             nFramesInBuffer;
 REFERENCE_TIME     hnsPeriod, hnsActualDuration;
 HANDLE             hTask;
 CCritSec           m_csCheck;
 UINT32             bufferSize;
 bool               isAudioClientStarted;
 DWORD              lastBufferTime;

	 // AVRT.dll (Vista or greater
	typedef HANDLE  (__stdcall *PTR_AvSetMmThreadCharacteristicsW)(LPCWSTR TaskName, LPDWORD TaskIndex);
	typedef BOOL	(__stdcall *PTR_AvRevertMmThreadCharacteristics)(HANDLE AvrtHandle);

	PTR_AvSetMmThreadCharacteristicsW		pfAvSetMmThreadCharacteristicsW;
	PTR_AvRevertMmThreadCharacteristics		pfAvRevertMmThreadCharacteristics;

};

