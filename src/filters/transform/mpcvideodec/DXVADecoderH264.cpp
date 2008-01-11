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
	NALU			Nalu;
	BYTE*			pDataSlice	= pBuffer;
	UINT			nSliceSize	= nSize;
	int				nDummy;

	nSize = 0;
	do
	{
		ReadNalu (&Nalu, pDataSlice, nSliceSize, m_nNALLength);

		switch (Nalu.nal_unit_type)
		{
			case NALU_TYPE_PPS :
			case NALU_TYPE_SPS :
				// Do not copy thoses units, accelerator don't like it
				break;
			default :
				// Put startcode 00000001
				pDXVABuffer[0]=pDXVABuffer[1]=pDXVABuffer[2]=0; pDXVABuffer[3]=1;
				
				// Copy NALU
				memcpy (pDXVABuffer+4, (BYTE*)pDataSlice+m_nNALLength, Nalu.len-m_nNALLength);
				pDXVABuffer	+= Nalu.len;
				nSize       += Nalu.len;
				break;
		}

		pDataSlice	+= Nalu.len;
		nSliceSize	-= Nalu.len;
	} while (nSliceSize > 0);

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
	BYTE*						pDataSlice	= pDataIn;
	UINT						nSliceSize	= nSize;
	NALU						Nalu;
	int							nSurfaceIndex;
	CComPtr<IMediaSample>		pSampleToDeliver;

	bool		bSliceFound = false;

	while (!bSliceFound)
	{
		ReadNalu (&Nalu, pDataSlice, nSliceSize, m_nNALLength);
		switch (Nalu.nal_unit_type)
		{
		case NALU_TYPE_SLICE:
		case NALU_TYPE_IDR:
			bSliceFound = true;
			break;

		case NALU_TYPE_PPS :
		case NALU_TYPE_SPS :
			FFH264UpdatePictureParam (false, &m_DXVAPicParams, &m_DXVAScalingMatrix, m_pFilter->GetAVCtx(), pDataSlice, nSliceSize);
			break;
		}

		if (!bSliceFound)
		{
			if (Nalu.len > nSliceSize) 
				return E_INVALIDARG;
			pDataSlice	+= Nalu.len;
			nSliceSize	-= Nalu.len;
		}
	}

	// Parse slice header and set DX destination surface
	CHECK_HR (FFH264ReadSlideHeader (&m_DXVAPicParams, m_pFilter->GetAVCtx(), pDataSlice+ m_nNALLength, nSliceSize -  m_nNALLength));
	CHECK_HR (GetFreeSurfaceIndex (nSurfaceIndex, &pSampleToDeliver, rtStart, rtStop));
	m_DXVAPicParams.CurrPic.bPicEntry			= nSurfaceIndex;
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

	UpdateRefFramesList (m_DXVAPicParams.frame_num, (Nalu.nal_reference_idc != 0));
	AddToStore (nSurfaceIndex, pSampleToDeliver, (Nalu.nal_reference_idc != 0), /*m_Slice.framepoc/2,*/ rtStart, rtStop);

	return DisplayNextFrame();
}


void CDXVADecoderH264::SetExtraData (BYTE* pDataIn, UINT nSize)
{
	AVCodecContext*		pAVCtx = m_pFilter->GetAVCtx();
	m_nNALLength	= pAVCtx->nal_length_size;
	FFH264UpdatePictureParam (true, &m_DXVAPicParams, &m_DXVAScalingMatrix, pAVCtx, pDataIn, nSize);
}


void CDXVADecoderH264::ClearRefFramesList()
{
	int		i;

	for (i=0; i<m_DXVAPicParams.num_ref_frames; i++)
	{
		if (m_DXVAPicParams.RefFrameList[i].bPicEntry != 255)
			RemoveRefFrame (m_DXVAPicParams.RefFrameList[i].bPicEntry);

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

	// Reset when new picture group detected
	if (nFrameNum == 0) ClearRefFramesList();

	if (bRefFrame)
	{
		// Shift buffers if needed
		if (!m_DXVAPicParams.RefFrameList[m_nCurRefFrame].AssociatedFlag)
		{
			if (m_DXVAPicParams.RefFrameList[0].bPicEntry != 255)
				RemoveRefFrame (m_DXVAPicParams.RefFrameList[0].bPicEntry);

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
		m_DXVAPicParams.RefFrameList[m_nCurRefFrame].bPicEntry		= m_DXVAPicParams.CurrPic.bPicEntry;
		m_DXVAPicParams.RefFrameList[m_nCurRefFrame].Index7Bits		= m_DXVAPicParams.CurrPic.Index7Bits;

		m_DXVAPicParams.FieldOrderCntList[m_nCurRefFrame][0]		= m_DXVAPicParams.CurrFieldOrderCnt[0];
		m_DXVAPicParams.FieldOrderCntList[m_nCurRefFrame][1]		= m_DXVAPicParams.CurrFieldOrderCnt[1];

		m_DXVAPicParams.UsedForReferenceFlags	= g_UsedForReferenceFlags [m_nCurRefFrame];
		m_nCurRefFrame = min (m_nCurRefFrame+1, (UINT)(m_DXVAPicParams.num_ref_frames-1));
	}
}


void CDXVADecoderH264::ReadNalu (NALU* pNalu, BYTE* pBuffer, UINT nBufferLength, UINT NbBytesForSize)
{
	pNalu->data		= pBuffer;

	pNalu->data_len = 0;
	for (UINT i=0; i<NbBytesForSize; i++)
	{
		pNalu->data_len = (pNalu->data_len << 8) + *pNalu->data;
		pNalu->data++;
	}

	pNalu->len					= pNalu->data_len + NbBytesForSize;
	pNalu->forbidden_bit		= (*pNalu->data>>7) & 1;
	pNalu->nal_reference_idc	= (*pNalu->data>>5) & 3;
	pNalu->nal_unit_type		= (*pNalu->data)	  & 0x1f;
	pNalu->data++;
	pNalu->data_len--;
}
