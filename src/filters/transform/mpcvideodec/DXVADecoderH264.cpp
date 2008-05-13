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
#include "DXVADecoderH264.h"
#include "MPCVideoDecFilter.h"

#include "PODtypes.h"
#include "avcodec.h"

extern "C"
{
	#include "FfmpegContext.h"
}


#define SE_HEADER           0

static UINT g_UsedForReferenceFlags[] =
{
	0x00000003,
	0x0000000F,
	0x0000003F,
	0x000000FF,
	0x000003FF,
	0x00000FFF,
	0x00003FFF,
	0x0000FFFF,
	0x0003FFFF,
	0x000FFFFF,
	0x003FFFFF,
	0x00FFFFFF,
	0x03FFFFFF,
	0x0FFFFFFF,
	0x3FFFFFFF,
	0xFFFFFFFF,
};


CDXVADecoderH264::CDXVADecoderH264 (CMPCVideoDecFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, DXVAMode nMode, int nPicEntryNumber)
				: CDXVADecoder (pFilter, pAMVideoAccelerator, nMode, nPicEntryNumber)
{
	Init();
}

CDXVADecoderH264::CDXVADecoderH264 (CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, DXVAMode nMode, int nPicEntryNumber)
				: CDXVADecoder (pFilter, pDirectXVideoDec, nMode, nPicEntryNumber)
{
	Init();
}


void CDXVADecoderH264::Init()
{
	memset (&m_DXVAPicParams,	0, sizeof(m_DXVAPicParams));
	memset (&m_SliceShort,		0, sizeof(m_SliceShort));
	memset (&m_DXVAPicParams, 0, sizeof (DXVA_PicParams_H264));
	
	m_DXVAPicParams.MbsConsecutiveFlag					= 1;
	m_DXVAPicParams.Reserved16Bits						= 0;
	m_DXVAPicParams.ContinuationFlag					= 1;
	m_DXVAPicParams.Reserved8BitsA						= 0;
	m_DXVAPicParams.Reserved8BitsB						= 0;
	m_DXVAPicParams.MinLumaBipredSize8x8Flag			= 1;	// Improve accelerator performances
	m_DXVAPicParams.StatusReportFeedbackNumber			= 0;	// Use to report status

	for (int i =0; i<16; i++)
	{
		m_DXVAPicParams.RefFrameList[i].AssociatedFlag	= 1;
		m_DXVAPicParams.RefFrameList[i].bPicEntry		= 255;
		m_DXVAPicParams.RefFrameList[i].Index7Bits		= 127;
	}


	m_nCurRefFrame		= 0;
	m_nNALLength		= 4;

	switch (GetMode())
	{
	case H264_VLD :
		AllocExecuteParams (3);
		break;
	default :
		ASSERT(FALSE);
	}
}


void CDXVADecoderH264::CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize)
{
	CNalu			Nalu;
	int				nDummy;

	Nalu.SetBuffer (pBuffer, nSize, m_nNALLength);
	nSize = 0;

	while (Nalu.ReadNext())
	{
		switch (Nalu.GetType())
		{
		case NALU_TYPE_SLICE:
		case NALU_TYPE_IDR:
			// For AVC1, put startcode 0x000001
			pDXVABuffer[0]=pDXVABuffer[1]=0;pDXVABuffer[2]=1;
			
			// Copy NALU
			memcpy (pDXVABuffer+3, Nalu.GetDataBuffer(), Nalu.GetDataLength());
			
			// Add trailing bit
//			pDXVABuffer[Nalu.GetDataLength()+3] = 0x00;		// ???????

			pDXVABuffer	+= Nalu.GetDataLength() + 3;
			nSize       += Nalu.GetDataLength() + 3;

			break;
		}
	}

	// Complete with zero padding (buffer size should be a multiple of 128)
	nDummy  = 128 - (nSize %128);

	memset (pDXVABuffer, 0, nDummy);
	nSize  += nDummy;
}


void CDXVADecoderH264::Flush()
{
	ClearRefFramesList();
	m_DXVAPicParams.UsedForReferenceFlags	= 0;

	__super::Flush();
}

HRESULT CDXVADecoderH264::DecodeFrame (BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	HRESULT						hr			= S_FALSE;
	CNalu						Nalu;
	bool						bSliceFound = false;
	BYTE*						pDataSlice	= NULL;
	UINT						nSliceSize	= 0;
	int							nSurfaceIndex;
	CComPtr<IMediaSample>		pSampleToDeliver;


	Nalu.SetBuffer (pDataIn, nSize, m_nNALLength); 

	while (!bSliceFound && Nalu.ReadNext())
	{
		switch (Nalu.GetType())
		{
		case NALU_TYPE_SLICE:
		case NALU_TYPE_IDR:
			pDataSlice	= Nalu.GetDataBuffer();
			nSliceSize	= Nalu.GetDataLength();
			bSliceFound = true;
			break;

		case NALU_TYPE_PPS :
		case NALU_TYPE_SPS :
			FFH264DecodeBuffer (m_pFilter->GetAVCtx(), Nalu.GetNALBuffer(), Nalu.GetLength());			
			break;
		}
	}
	if (!bSliceFound) return S_FALSE;

	m_nMaxWaiting	= min (max (m_DXVAPicParams.num_ref_frames, 3), 8);

	// Parse slice header and set DX destination surface
	CHECK_HR (FFH264ReadSlideHeader (&m_DXVAPicParams, &m_DXVAScalingMatrix, m_pFilter->GetAVCtx(), pDataSlice, nSliceSize));
	// Wait I frame after a flush
	if (m_bFlushed && !m_DXVAPicParams.IntraPicFlag)
		return S_FALSE;

	
	CHECK_HR (GetFreeSurfaceIndex (nSurfaceIndex, &pSampleToDeliver, rtStart, rtStop));

	m_DXVAPicParams.CurrPic.Index7Bits			= nSurfaceIndex;

	CHECK_HR (BeginFrame(nSurfaceIndex, pSampleToDeliver));

	// Send picture parameters
	CHECK_HR (AddExecuteBuffer (DXVA2_PictureParametersBufferType, sizeof(m_DXVAPicParams), &m_DXVAPicParams));
	m_DXVAPicParams.StatusReportFeedbackNumber++;
	CHECK_HR (Execute());

	// Add bitstream, slice control and quantization matrix
	CHECK_HR (AddExecuteBuffer (DXVA2_BitStreamDateBufferType, nSize, pDataIn, &nSize));
	m_SliceShort.SliceBytesInBuffer = nSize;
	CHECK_HR (AddExecuteBuffer (DXVA2_SliceControlBufferType, sizeof (m_SliceShort), &m_SliceShort));
	CHECK_HR (AddExecuteBuffer (DXVA2_InverseQuantizationMatrixBufferType, sizeof (DXVA_Qmatrix_H264), (void*)&m_DXVAScalingMatrix));

	// Decode bitstream
	m_DXVAPicParams.StatusReportFeedbackNumber++;
	CHECK_HR (Execute());

	CHECK_HR (EndFrame(nSurfaceIndex));

	if (AddToStore (nSurfaceIndex, pSampleToDeliver, Nalu.IsRefFrame(), rtStart, rtStop, m_DXVAPicParams.field_pic_flag))
	{
		// Reset when new picture group detected
		if (m_DXVAPicParams.frame_num == 0) ClearRefFramesList();
		hr = DisplayNextFrame();
	}
	UpdateRefFramesList (m_DXVAPicParams.frame_num, Nalu.IsRefFrame());

	m_bFlushed = false;
	return hr;
}


void CDXVADecoderH264::SetExtraData (BYTE* pDataIn, UINT nSize)
{
	AVCodecContext*		pAVCtx = m_pFilter->GetAVCtx();
	m_nNALLength	= pAVCtx->nal_length_size;
	FFH264DecodeBuffer (pAVCtx, pDataIn, nSize);
}


void CDXVADecoderH264::ClearRefFramesList()
{
	int		i;

	for (i=0; i<m_DXVAPicParams.num_ref_frames; i++)
	{
		if (m_DXVAPicParams.RefFrameList[i].bPicEntry != 255)
			RemoveRefFrame (m_DXVAPicParams.RefFrameList[i].Index7Bits);

		m_DXVAPicParams.RefFrameList[i].AssociatedFlag	= 1;
		m_DXVAPicParams.RefFrameList[i].bPicEntry		= 255;
		m_DXVAPicParams.RefFrameList[i].Index7Bits		= 127;
		
		m_DXVAPicParams.FieldOrderCntList[i][0]			= 0;
		m_DXVAPicParams.FieldOrderCntList[i][1]			= 0;

		m_DXVAPicParams.FrameNumList[i]					= 0;
	}

	m_nCurRefFrame = 0;
}


void CDXVADecoderH264::UpdateRefFramesList (int nFrameNum, bool bRefFrame)
{
	int			i;

	if (bRefFrame)
	{
		// Shift buffers if needed
		if (!m_DXVAPicParams.RefFrameList[m_nCurRefFrame].AssociatedFlag)
		{
			if (m_DXVAPicParams.RefFrameList[0].bPicEntry != 255)
				RemoveRefFrame (m_DXVAPicParams.RefFrameList[0].Index7Bits);

			for (i=1; i<m_DXVAPicParams.num_ref_frames; i++)
			{
				m_DXVAPicParams.FrameNumList[i-1] = m_DXVAPicParams.FrameNumList[i];
				memcpy (&m_DXVAPicParams.RefFrameList[i-1], &m_DXVAPicParams.RefFrameList[i], sizeof (DXVA_PicEntry_H264));

				m_DXVAPicParams.FieldOrderCntList[i-1][0] = m_DXVAPicParams.FieldOrderCntList[i][0];
				m_DXVAPicParams.FieldOrderCntList[i-1][1] = m_DXVAPicParams.FieldOrderCntList[i][1];
			}
		}

		m_DXVAPicParams.FrameNumList[m_nCurRefFrame] = nFrameNum;

		// Update current frame parameters
		m_DXVAPicParams.RefFrameList[m_nCurRefFrame].AssociatedFlag	= 0;
		m_DXVAPicParams.RefFrameList[m_nCurRefFrame].Index7Bits		= m_DXVAPicParams.CurrPic.Index7Bits;

		m_DXVAPicParams.FieldOrderCntList[m_nCurRefFrame][0]		= m_DXVAPicParams.CurrFieldOrderCnt[0];
		m_DXVAPicParams.FieldOrderCntList[m_nCurRefFrame][1]		= m_DXVAPicParams.CurrFieldOrderCnt[1];

		m_DXVAPicParams.UsedForReferenceFlags	= g_UsedForReferenceFlags [m_nCurRefFrame];
		m_nCurRefFrame = min (m_nCurRefFrame+1, (UINT)(m_DXVAPicParams.num_ref_frames-1));
	}
}


void CNalu::SetBuffer(BYTE* pBuffer, int nSize, int nNALSize)
{
	m_pBuffer		= pBuffer;
	m_nSize			= nSize;
	m_nNALSize		= nNALSize;
	m_nCurPos		= 0;
	m_nNextRTP		= 0;

	m_nNALStartPos	= 0;
	m_nNALDataPos	= 0;
	m_nDataLen		= 0;
}

bool CNalu::MoveToNextStartcode()
{
	for (int i=m_nCurPos; i<min (m_nNextRTP, m_nSize-4); i++)
	{
		if ((*((DWORD*)(m_pBuffer+i)) & 0x00FFFFFF) == 0x00010000)
		{
			// Find next AnnexB Nal
			m_nCurPos = i;
			return true;
		}
	}

	if ((m_nNALSize != 0) && (m_nNextRTP < m_nSize))
	{
		m_nCurPos = m_nNextRTP;
		return true;
	}

	m_nCurPos = m_nSize;
	return false;
}

bool CNalu::ReadNext()
{
	int		nTemp;

	if (m_nCurPos >= m_nSize) return false;

	if ((m_nNALSize != 0) && (m_nCurPos == m_nNextRTP))
	{
		// RTP Nalu type
		m_nNALStartPos	= m_nCurPos;
		m_nNALDataPos	= m_nCurPos + m_nNALSize;
		nTemp			= 0;
		for (UINT i=0; i<m_nNALSize; i++)
		{
			nTemp = (nTemp << 8) + m_pBuffer[m_nCurPos++];
		}
		m_nNextRTP += nTemp + m_nNALSize;
		MoveToNextStartcode();
	}
	else
	{
		// AnnexB Nalu
		m_nNALStartPos	= m_nCurPos;
		m_nCurPos	   += 3;
		m_nNALDataPos	= m_nCurPos;
		MoveToNextStartcode();
	}

	forbidden_bit		= (m_pBuffer[m_nNALDataPos]>>7) & 1;
	nal_reference_idc	= (m_pBuffer[m_nNALDataPos]>>5) & 3;
	nal_unit_type		= (NALU_TYPE) (m_pBuffer[m_nNALDataPos] & 0x1f);

	return true;
}
