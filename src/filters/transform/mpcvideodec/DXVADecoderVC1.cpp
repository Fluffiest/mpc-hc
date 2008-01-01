/* 
 * $Id: DXVADecoderVC1.cpp 249 2007-09-26 11:07:22Z casimir666 $
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
#include "DXVADecoderVC1.h"
#include "MPCVideoDecFilter.h"


#define VC1_NO_REF			0xFFFF;

CDXVADecoderVC1::CDXVADecoderVC1 (CMPCVideoDecFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, DXVAMode nMode, int nPicEntryNumber)
			   : CDXVADecoder (pFilter, pAMVideoAccelerator, nMode, nPicEntryNumber)
{
	Init();
}


CDXVADecoderVC1::CDXVADecoderVC1 (CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, DXVAMode nMode, int nPicEntryNumber)
			   : CDXVADecoder (pFilter, pDirectXVideoDec, nMode, nPicEntryNumber)
{
	Init();
}


CDXVADecoderVC1::~CDXVADecoderVC1(void)
{
	Flush();
}

void CDXVADecoderVC1::Init()
{
	memset (&m_PictureParams, 0, sizeof(m_PictureParams));
	memset (&m_SliceInfo,     0, sizeof(m_SliceInfo));

	m_PictureParams.wForwardRefPictureIndex			= VC1_NO_REF;
	m_PictureParams.wBackwardRefPictureIndex		= VC1_NO_REF;
	m_PictureParams.wPicHeightInMBminus1			= 15;
	m_PictureParams.bMacroblockHeightMinus1			= 15;
	m_PictureParams.bBlockWidthMinus1				= 7;
	m_PictureParams.bBlockHeightMinus1				= 7;
	m_PictureParams.bBPPminus1						= 7;
	m_PictureParams.bPicStructure					= VC1_PS_PROGRESSIVE;
	m_PictureParams.bChromaFormat					= VC1_CHROMA_420;
	m_PictureParams.bPicScanFixed					= 1;
	m_PictureParams.bPicScanMethod					= VC1_SCAN_ARBITRARY;

	switch (GetMode())
	{
	case VC1_VLD :
		AllocExecuteParams (3);
		break;
	default :
		ASSERT(FALSE);
	}
}

// === Public functions
HRESULT CDXVADecoderVC1::DecodeFrame (BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	HRESULT						hr;
	int							nSurfaceIndex;
	CComPtr<IMediaSample>		pSampleToDeliver;

	m_PictureParams.bMVprecisionAndChromaRelation	= VC1_CR_BICUBIC_QUARTER_CHROMA;		
	m_PictureParams.bPicSpatialResid8				= m_PictureParams.bPicIntra;
	m_PictureParams.bPicExtrapolation				= (m_PictureParams.bPicStructure == VC1_PS_PROGRESSIVE) ? 1 : 0;
	/*
	m_PictureParams.wForwardRefPictureIndex			= ;
	m_PictureParams.wBackwardRefPictureIndex		= ;
	m_PictureParams.wPicWidthInMBminus1				= ;
	m_PictureParams.bPicIntra						= ;
	m_PictureParams.bPicBackwardPrediction			= ;
	m_PictureParams.bBidirectionalAveragingMode		= ;		// 0x80, 0x88, 0x90, 0x98, 0xC0, 0xC8, 0xD0, 0xD8
	m_PictureParams.	= ;
	m_PictureParams.	= ;
	m_PictureParams.	= ;
	m_PictureParams.	= ;
	m_PictureParams.	= ;
	m_PictureParams.	= ;
	m_PictureParams.	= ;
	m_PictureParams.	= ;
	*/

	CHECK_HR (GetFreeSurfaceIndex (nSurfaceIndex, &pSampleToDeliver, rtStart, rtStop));
	CHECK_HR (BeginFrame(nSurfaceIndex, pSampleToDeliver));

	// Send picture params to accelerator
	m_PictureParams.wDecodedPictureIndex	= nSurfaceIndex;
	CHECK_HR (AddExecuteBuffer (DXVA2_PictureParametersBufferType, sizeof(m_PictureParams), &m_PictureParams));
	CHECK_HR (Execute());

	// Send bitstream to accelerator
	m_SliceInfo.dwSliceDataLocation	= nSize * 8;
	m_SliceInfo.wQuantizerScaleCode	= 1;			// TODO : 1->31 ???
	CHECK_HR (AddExecuteBuffer (DXVA2_BitStreamDateBufferType, nSize, pDataIn, &nSize));
	CHECK_HR (AddExecuteBuffer (DXVA2_SliceControlBufferType, sizeof (m_SliceInfo), &m_SliceInfo));

	// Decode frame
	CHECK_HR (Execute());
	CHECK_HR (EndFrame(nSurfaceIndex));

//	AddToStore (nSurfaceIndex, pSampleToDeliver, (Nalu.nal_reference_idc != 0), m_Slice.framepoc/2, rtStart, rtStop);

	return hr;
}

void CDXVADecoderVC1::SetExtraData (BYTE* pDataIn, UINT nSize)
{
}

void CDXVADecoderVC1::CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize)
{
	int		nDummy;

	// Copy bitstream buffer, with zero padding (buffer is rounded to multiple of 128)
	memcpy (pDXVABuffer, (BYTE*)pBuffer, nSize);
	nDummy  = 128 - (nSize %128);
	pDXVABuffer += nSize;
	memset (pDXVABuffer, 0, nDummy);
	nSize  += nDummy;
}

void CDXVADecoderVC1::Flush()
{
}