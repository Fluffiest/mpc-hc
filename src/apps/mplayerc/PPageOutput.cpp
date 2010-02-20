/*
 * $Id$
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2010 see AUTHORS
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
#include "MainFrm.h"
#include "PPageOutput.h"
#include <moreuuids.h>

#include "Monitors.h"

// CPPageOutput dialog

IMPLEMENT_DYNAMIC(CPPageOutput, CPPageBase)
CPPageOutput::CPPageOutput()
	: CPPageBase(CPPageOutput::IDD, CPPageOutput::IDD)
	, m_iDSVideoRendererType(0)
	, m_iRMVideoRendererType(0)
	, m_iQTVideoRendererType(0)
	, m_iAPSurfaceUsage(0)
	, m_iAudioRendererType(0)
//	, m_fVMRSyncFix(FALSE)
	, m_iDX9Resizer(0)
	, m_fVMR9MixerMode(FALSE)
	, m_fVMR9MixerYUV(FALSE)
	, m_fVMR9AlterativeVSync(FALSE)
	, m_fResetDevice(FALSE)
	, m_iEvrBuffers(L"5")
	, m_fD3DFullscreen(FALSE)
{
}

CPPageOutput::~CPPageOutput()
{
}

void CPPageOutput::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_DSSYSDEF, m_iDSVideoRendererType);
	DDX_Radio(pDX, IDC_RMSYSDEF, m_iRMVideoRendererType);
	DDX_Radio(pDX, IDC_QTSYSDEF, m_iQTVideoRendererType);
//	DDX_Radio(pDX, IDC_REGULARSURF, m_iAPSurfaceUsage);
	DDX_CBIndex(pDX, IDC_DX_SURFACE, m_iAPSurfaceUsage);
	DDX_CBIndex(pDX, IDC_COMBO1, m_iAudioRendererType);
	DDX_Control(pDX, IDC_COMBO1, m_iAudioRendererTypeCtrl);
//	DDX_Check(pDX, IDC_CHECK1, m_fVMRSyncFix);
	DDX_CBIndex(pDX, IDC_DX9RESIZER_COMBO, m_iDX9Resizer);
	DDX_Check(pDX, IDC_DSVMR9LOADMIXER, m_fVMR9MixerMode);
	DDX_Check(pDX, IDC_DSVMR9YUVMIXER, m_fVMR9MixerYUV);
	DDX_Check(pDX, IDC_DSVMR9ALTERNATIVEVSYNC, m_fVMR9AlterativeVSync);
	DDX_Check(pDX, IDC_RESETDEVICE, m_fResetDevice);
	DDX_Check(pDX, IDC_FULLSCREEN_MONITOR_CHECK, m_fD3DFullscreen);
	
	DDX_CBString(pDX, IDC_EVR_BUFFERS, m_iEvrBuffers);
}

BEGIN_MESSAGE_MAP(CPPageOutput, CPPageBase)
	ON_UPDATE_COMMAND_UI(IDC_DSVMR9YUVMIXER, OnUpdateMixerYUV)	
	ON_CBN_SELCHANGE(IDC_DX_SURFACE, &CPPageOutput::OnSurfaceChange)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_DSSYSDEF, IDC_DSSYNC, &CPPageOutput::OnDSRendererChange)
	ON_BN_CLICKED(IDC_FULLSCREEN_MONITOR_CHECK, OnFullscreenCheck)
END_MESSAGE_MAP()

void CPPageOutput::DisableRadioButton(UINT nID, UINT nDefID)
{
	if(IsDlgButtonChecked(nID))
	{
		CheckDlgButton(nID, BST_UNCHECKED);
		CheckDlgButton(nDefID, BST_CHECKED);
	}

	GetDlgItem(nID)->EnableWindow(FALSE);
}

// CPPageOutput message handlers

BOOL CPPageOutput::OnInitDialog()
{
	__super::OnInitDialog();

	AppSettings& s = AfxGetAppSettings();

	m_iDSVideoRendererType	= s.iDSVideoRendererType;
	m_iRMVideoRendererType	= s.iRMVideoRendererType;
	m_iQTVideoRendererType	= s.iQTVideoRendererType;
	m_iAPSurfaceUsage		= s.iAPSurfaceUsage;
//	m_fVMRSyncFix			= s.fVMRSyncFix;
	m_iDX9Resizer			= s.iDX9Resizer;
	m_fVMR9MixerMode		= s.fVMR9MixerMode;
	m_fVMR9MixerYUV			= s.fVMR9MixerYUV;
	m_fVMR9AlterativeVSync	= s.m_RenderSettings.fVMR9AlterativeVSync;
	m_fD3DFullscreen		= s.fD3DFullscreen;
	m_iEvrBuffers.Format(L"%d", s.iEvrBuffers);

	m_fResetDevice = s.fResetDevice;
	m_AudioRendererDisplayNames.Add(_T(""));
	m_iAudioRendererTypeCtrl.AddString(_T("1: System Default"));
	m_iAudioRendererType = 0;

	int i=2;
	CString Cbstr;

	BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker)
	{
		LPOLESTR olestr = NULL;
		if(FAILED(pMoniker->GetDisplayName(0, 0, &olestr)))
			continue;

		CStringW str(olestr);
		CoTaskMemFree(olestr);

		m_AudioRendererDisplayNames.Add(CString(str));

		CComPtr<IPropertyBag> pPB;
		if(SUCCEEDED(pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB)))
		{
			CComVariant var;
			pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL);

			CString fstr(var.bstrVal);

			var.Clear();
			if(SUCCEEDED(pPB->Read(CComBSTR(_T("FilterData")), &var, NULL)))
			{			
				BSTR* pbstr;
				if(SUCCEEDED(SafeArrayAccessData(var.parray, (void**)&pbstr)))
				{
					fstr.Format(_T("%s (%08x)"), CString(fstr), *((DWORD*)pbstr + 1));
					SafeArrayUnaccessData(var.parray);
				}
			}
			Cbstr.Format(_T("%d: %s"), i, fstr);
		}
		else
		{	
			Cbstr.Format(_T("%d: %s"), i, CString(str));
		}
		m_iAudioRendererTypeCtrl.AddString(Cbstr);

		if(s.AudioRendererDisplayName == str && m_iAudioRendererType == 0)
		{
			m_iAudioRendererType = m_iAudioRendererTypeCtrl.GetCount()-1;
		}
		i++;
	}
	EndEnumSysDev

	Cbstr.Format(_T("%d: %s"), i++, AUDRNDT_NULL_COMP);
	m_AudioRendererDisplayNames.Add(AUDRNDT_NULL_COMP);
	m_iAudioRendererTypeCtrl.AddString(Cbstr);
	if(s.AudioRendererDisplayName == AUDRNDT_NULL_COMP && m_iAudioRendererType == 0)
		m_iAudioRendererType = m_iAudioRendererTypeCtrl.GetCount()-1;

	Cbstr.Format(_T("%d: %s"), i++, AUDRNDT_NULL_UNCOMP);
	m_AudioRendererDisplayNames.Add(AUDRNDT_NULL_UNCOMP);
	m_iAudioRendererTypeCtrl.AddString(Cbstr);
	if(s.AudioRendererDisplayName == AUDRNDT_NULL_UNCOMP && m_iAudioRendererType == 0)
		m_iAudioRendererType = m_iAudioRendererTypeCtrl.GetCount()-1;

	Cbstr.Format(_T("%d: %s"), i++, AUDRNDT_MPC);
	m_AudioRendererDisplayNames.Add(AUDRNDT_MPC);
	m_iAudioRendererTypeCtrl.AddString(Cbstr);
	if(s.AudioRendererDisplayName == AUDRNDT_MPC && m_iAudioRendererType == 0)
		m_iAudioRendererType = m_iAudioRendererTypeCtrl.GetCount()-1;


	CorrectComboListWidth(m_iAudioRendererTypeCtrl, GetFont());

	UpdateData(FALSE);

	if(!IsCLSIDRegistered(CLSID_VideoMixingRenderer))
	{
		DisableRadioButton(IDC_DSVMR7WIN, IDC_DSSYSDEF);
		DisableRadioButton(IDC_DSVMR7REN, IDC_DSSYSDEF);
	}

	if(!IsCLSIDRegistered(CLSID_VideoMixingRenderer9))
	{
		DisableRadioButton(IDC_DSVMR9WIN, IDC_DSSYSDEF);
		DisableRadioButton(IDC_DSVMR9REN, IDC_DSSYSDEF);
		DisableRadioButton(IDC_RMDX9, IDC_RMSYSDEF);
		DisableRadioButton(IDC_QTDX9, IDC_QTSYSDEF);
	}

	if(!IsCLSIDRegistered(CLSID_EnhancedVideoRenderer))
	{
		DisableRadioButton(IDC_EVR, IDC_DSSYSDEF);
		DisableRadioButton(IDC_EVR_CUSTOM, IDC_DSSYSDEF);
	}

	if(!IsCLSIDRegistered(CLSID_DXR))
	{
		DisableRadioButton(IDC_DSDXR, IDC_DSSYSDEF);
	}

	if(!IsCLSIDRegistered(CLSID_madVR))
	{
		DisableRadioButton(IDC_DSMADVR, IDC_DSSYSDEF);
	}

	// YUV mixing is not compatible with Vista
	if (AfxGetMyApp()->IsVistaOrAbove())
	{
		GetDlgItem(IDC_DSVMR9YUVMIXER)->ShowWindow (SW_HIDE);
	}

	OnDSRendererChange (m_iDSVideoRendererType + IDC_DSSYSDEF);

	CreateToolTip();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageOutput::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	s.iDSVideoRendererType		= m_iDSVideoRendererType;
	s.iRMVideoRendererType		= m_iRMVideoRendererType;
	s.iQTVideoRendererType		= m_iQTVideoRendererType;
	s.iAPSurfaceUsage			= m_iAPSurfaceUsage;
//	s.fVMRSyncFix				= !!m_fVMRSyncFix;
	s.AudioRendererDisplayName	= m_AudioRendererDisplayNames[m_iAudioRendererType];
	s.iDX9Resizer				= m_iDX9Resizer;
	s.fVMR9MixerMode			= !!m_fVMR9MixerMode;
	s.fVMR9MixerYUV				= !!m_fVMR9MixerYUV;
	s.m_RenderSettings.fVMR9AlterativeVSync		= m_fVMR9AlterativeVSync != 0;
	s.fD3DFullscreen			= m_fD3DFullscreen ? true : false;

	s.fResetDevice = !!m_fResetDevice;

	if (!m_iEvrBuffers.IsEmpty())
	{
		int Temp = 5;
		swscanf(m_iEvrBuffers.GetBuffer(), L"%d", &Temp);
		s.iEvrBuffers = Temp;
	}
	else
		s.iEvrBuffers = 5;

	return __super::OnApply();
}

void CPPageOutput::OnUpdateMixerYUV(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!!IsDlgButtonChecked(IDC_DSVMR9LOADMIXER) && IsDlgButtonChecked(IDC_DSVMR9REN));
}
void CPPageOutput::OnSurfaceChange()
{
	SetModified();
}

void CPPageOutput::OnDSRendererChange(UINT nIDbutton)
{
	GetDlgItem(IDC_DX_SURFACE)->EnableWindow(FALSE);
	GetDlgItem(IDC_DX9RESIZER_COMBO)->EnableWindow(FALSE);
	GetDlgItem(IDC_FULLSCREEN_MONITOR_CHECK)->EnableWindow(FALSE);
	GetDlgItem(IDC_DSVMR9LOADMIXER)->EnableWindow(FALSE);
	GetDlgItem(IDC_DSVMR9YUVMIXER)->EnableWindow(FALSE);
	GetDlgItem(IDC_DSVMR9ALTERNATIVEVSYNC)->EnableWindow(FALSE);
	GetDlgItem(IDC_RESETDEVICE)->EnableWindow(FALSE);
//	GetDlgItem(IDC_CHECK1)->EnableWindow(FALSE);
	GetDlgItem(IDC_EVR_BUFFERS)->EnableWindow((nIDbutton - IDC_DSSYSDEF) == 11);
	GetDlgItem(IDC_EVR_BUFFERS_TXT)->EnableWindow((nIDbutton - IDC_DSSYSDEF) == 11);

	switch (nIDbutton - IDC_DSSYSDEF)
	{
	case 6 :	// VMR9 renderless
		GetDlgItem(IDC_DSVMR9LOADMIXER)->EnableWindow(TRUE);
		GetDlgItem(IDC_DSVMR9YUVMIXER)->EnableWindow(TRUE);
		GetDlgItem(IDC_DSVMR9ALTERNATIVEVSYNC)->EnableWindow(TRUE);		
		GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);
	case 11 :	// EVR custom presenter
		GetDlgItem(IDC_DX9RESIZER_COMBO)->EnableWindow(TRUE);
		GetDlgItem(IDC_FULLSCREEN_MONITOR_CHECK)->EnableWindow(TRUE);
//		GetDlgItem(IDC_CHECK1)->EnableWindow(TRUE);		// Lock back buffer
		GetDlgItem(IDC_DSVMR9ALTERNATIVEVSYNC)->EnableWindow(TRUE);
		GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);

		// Force 3D surface with EVR Custom
		if (nIDbutton - IDC_DSSYSDEF == 11)
		{
			GetDlgItem(IDC_DX_SURFACE)->EnableWindow(FALSE);
			((CComboBox*)GetDlgItem(IDC_DX_SURFACE))->SetCurSel(2);
		}
		else
			GetDlgItem(IDC_DX_SURFACE)->EnableWindow(TRUE);
		break;
	case 13 :	// Sync Renderer
		GetDlgItem(IDC_EVR_BUFFERS)->EnableWindow(TRUE);
		GetDlgItem(IDC_EVR_BUFFERS_TXT)->EnableWindow(TRUE);
		GetDlgItem(IDC_DX9RESIZER_COMBO)->EnableWindow(TRUE);
		GetDlgItem(IDC_FULLSCREEN_MONITOR_CHECK)->EnableWindow(TRUE);
		GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);
		GetDlgItem(IDC_DX_SURFACE)->EnableWindow(FALSE);
		((CComboBox*)GetDlgItem(IDC_DX_SURFACE))->SetCurSel(2);
		break;
	case 5 :	// VMR7 renderless
		GetDlgItem(IDC_DX_SURFACE)->EnableWindow(TRUE);
		break;
	}

	SetModified();
}

void CPPageOutput::OnFullscreenCheck()
{
	UpdateData();
	if (m_fD3DFullscreen &&
		(MessageBox(ResStr(IDS_D3DFS_WARNING), NULL, MB_YESNO) == IDNO))
	{
		m_fD3DFullscreen = false;
		UpdateData(FALSE);
	}
	SetModified();
}