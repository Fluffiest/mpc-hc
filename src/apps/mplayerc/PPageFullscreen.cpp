/*
 * $Id: PPageFullScreen.cpp 1457 2010-01-11 03:13:59Z X-Dron $
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
#include "PPageFullscreen.h"

#include "Monitors.h"
#include "MultiMonitor.h"

// CPPagePlayer dialog
	
IMPLEMENT_DYNAMIC(CPPageFullscreen, CPPageBase)
CPPageFullscreen::CPPageFullscreen()
		: CPPageBase(CPPageFullscreen::IDD, CPPageFullscreen::IDD)
		, m_launchfullscreen(FALSE)
		, m_fSetFullscreenRes(FALSE)
		, m_iShowBarsWhenFullScreen(FALSE)
		, m_nShowBarsWhenFullScreenTimeOut(0)
		, m_fExitFullScreenAtTheEnd(FALSE)
{
}
	
CPPageFullscreen::~CPPageFullscreen()
{
}

void CPPageFullscreen::DoDataExchange(CDataExchange* pDX)
{
		__super::DoDataExchange(pDX);
		DDX_Check(pDX, IDC_CHECK1, m_launchfullscreen);
		DDX_Check(pDX, IDC_CHECK2, m_fSetFullscreenRes);
		DDX_CBIndex(pDX, IDC_COMBO1, m_iMonitorType);
		DDX_Control(pDX, IDC_COMBO1, m_iMonitorTypeCtrl);
		DDX_Control(pDX, IDC_COMBO2, m_dispmode24combo);
		DDX_Control(pDX, IDC_COMBO3, m_dispmode25combo);
		DDX_Control(pDX, IDC_COMBO4, m_dispmode30combo);
		DDX_Control(pDX, IDC_COMBO5, m_dispmodeOthercombo);
		DDX_Check(pDX, IDC_CHECK4, m_iShowBarsWhenFullScreen);
		DDX_Text(pDX, IDC_EDIT1, m_nShowBarsWhenFullScreenTimeOut);
		DDX_Check(pDX, IDC_CHECK5, m_fExitFullScreenAtTheEnd);
		DDX_Control(pDX, IDC_SPIN1, m_nTimeOutCtrl);
			
}

BEGIN_MESSAGE_MAP(CPPageFullscreen, CPPageBase)
		ON_CBN_SELCHANGE(IDC_COMBO1, OnUpdateFullScrCombo)
		ON_UPDATE_COMMAND_UI(IDC_COMBO2, OnUpdateDispMode24Combo)
		ON_UPDATE_COMMAND_UI(IDC_COMBO3, OnUpdateDispMode25Combo)
		ON_UPDATE_COMMAND_UI(IDC_COMBO4, OnUpdateDispMode30Combo)
		ON_UPDATE_COMMAND_UI(IDC_COMBO5, OnUpdateDispModeOtherCombo)
		ON_UPDATE_COMMAND_UI(IDC_SPIN1, OnUpdateTimeout)
		ON_UPDATE_COMMAND_UI(IDC_EDIT1, OnUpdateTimeout)
END_MESSAGE_MAP()
	
// CPPagePlayer message handlers

BOOL CPPageFullscreen::OnInitDialog()
{
		__super::OnInitDialog();

		AppSettings& s = AfxGetAppSettings();

		m_launchfullscreen = s.launchfullscreen;
		m_AutoChangeFullscrRes = s.AutoChangeFullscrRes;
		m_f_hmonitor = s.f_hmonitor;
		m_iShowBarsWhenFullScreen = s.fShowBarsWhenFullScreen;
		m_nShowBarsWhenFullScreenTimeOut = s.nShowBarsWhenFullScreenTimeOut;
		m_nTimeOutCtrl.SetRange(-1, 10);
		m_fExitFullScreenAtTheEnd = s.fExitFullScreenAtTheEnd;
			
		//-> Multi-Monitor code
		CString str;
		m_iMonitorType = 0;

		CMonitor monitor;
		CMonitors monitors;

		m_iMonitorTypeCtrl.AddString(ResStr(IDS_FULLSCREENMONITOR_CURRENT));
		m_MonitorDisplayNames.Add(_T("Current"));
		if(m_f_hmonitor == _T("Current"))
		{
			m_iMonitorType = m_iMonitorTypeCtrl.GetCount()-1;
		}

		for ( int i = 0; i < monitors.GetCount(); i++ )
		{
			monitor = monitors.GetMonitor( i );
			monitor.GetName(str);

			if(monitor.IsMonitor())
			{
				DISPLAY_DEVICE displayDevice;
				ZeroMemory(&displayDevice, sizeof(displayDevice));
				displayDevice.cb = sizeof(displayDevice);
				VERIFY(EnumDisplayDevices(str, 0,  &displayDevice, 0));

				m_iMonitorTypeCtrl.AddString(str+_T(" - ")+displayDevice.DeviceString);
				m_MonitorDisplayNames.Add(str);

				if(m_f_hmonitor == str && m_iMonitorType == 0)
				{
					m_iMonitorType = m_iMonitorTypeCtrl.GetCount()-1;
				}
			}
		}

		if(m_iMonitorTypeCtrl.GetCount() > 2)
		{
			GetDlgItem(IDC_COMBO1)->EnableWindow(TRUE);
		}
		else
		{ 
			m_iMonitorType = 0;
			GetDlgItem(IDC_COMBO1)->EnableWindow(FALSE);
		}
		//<- Multi-Monitor code
			
		ModesUpdate();

		UpdateData(FALSE);

		return TRUE;  // return TRUE unless you set the focus to a control
		// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageFullscreen::OnApply()
{
		UpdateData();

		AppSettings& s = AfxGetAppSettings();
		
		int iSel_24   =  m_dispmode24combo.GetCurSel();
		int iSel_25   =  m_dispmode25combo.GetCurSel();
		int iSel_30   =  m_dispmode30combo.GetCurSel();
		int iSel_Other = m_dispmodeOthercombo.GetCurSel();
		
		if (m_AutoChangeFullscrRes.bEnabled = !!m_fSetFullscreenRes)
		{
			if(iSel_24 >= 0 && iSel_24 < m_dms.GetCount())
				m_AutoChangeFullscrRes.dmFullscreenRes24Hz = m_dms[m_dispmode24combo.GetCurSel()];
			if(iSel_25 >= 0 && iSel_25 < m_dms.GetCount())
				m_AutoChangeFullscrRes.dmFullscreenRes25Hz = m_dms[m_dispmode25combo.GetCurSel()];
			if(iSel_30 >= 0 && iSel_30 < m_dms.GetCount())
				m_AutoChangeFullscrRes.dmFullscreenRes30Hz = m_dms[m_dispmode30combo.GetCurSel()];
			if(iSel_Other >= 0 && iSel_Other < m_dms.GetCount())
				m_AutoChangeFullscrRes.dmFullscreenResOther = m_dms[m_dispmodeOthercombo.GetCurSel()];
		}
		s.AutoChangeFullscrRes = m_AutoChangeFullscrRes;			
		s.launchfullscreen = !!m_launchfullscreen;
		s.f_hmonitor =  m_f_hmonitor;
		s.fShowBarsWhenFullScreen = !!m_iShowBarsWhenFullScreen;
		s.nShowBarsWhenFullScreenTimeOut = m_nShowBarsWhenFullScreenTimeOut;
		s.fExitFullScreenAtTheEnd = !!m_fExitFullScreenAtTheEnd;


		return __super::OnApply();
}

void CPPageFullscreen::OnUpdateDispMode24Combo(CCmdUI* pCmdUI)
{
		pCmdUI->Enable(!!IsDlgButtonChecked(IDC_CHECK2));
}

void CPPageFullscreen::OnUpdateDispMode25Combo(CCmdUI* pCmdUI)
{
		pCmdUI->Enable(!!IsDlgButtonChecked(IDC_CHECK2));
}

void CPPageFullscreen::OnUpdateDispMode30Combo(CCmdUI* pCmdUI)
{
		pCmdUI->Enable(!!IsDlgButtonChecked(IDC_CHECK2));
}

void CPPageFullscreen::OnUpdateDispModeOtherCombo(CCmdUI* pCmdUI)
{
		pCmdUI->Enable(!!IsDlgButtonChecked(IDC_CHECK2));
}

void CPPageFullscreen::OnUpdateFullScrCombo()
{
		CMonitors monitors;
		m_f_hmonitor = m_MonitorDisplayNames[m_iMonitorTypeCtrl.GetCurSel()];
		if(AfxGetAppSettings().f_hmonitor !=  m_f_hmonitor) m_AutoChangeFullscrRes.bEnabled = false;
		ModesUpdate();
		SetModified();
}

void CPPageFullscreen::OnUpdateTimeout(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(m_iShowBarsWhenFullScreen);
}
void CPPageFullscreen::ModesUpdate()
{
		CMonitors monitors;
        m_fSetFullscreenRes = m_AutoChangeFullscrRes.bEnabled;
        int iSel_24, iSel_25, iSel_30, iSel_Other;
		iSel_24 = iSel_25 = iSel_30 = iSel_Other = -1;
        dispmode dm, 
        		 dmtoset24	  = m_AutoChangeFullscrRes.dmFullscreenRes24Hz,
        		 dmtoset25	  = m_AutoChangeFullscrRes.dmFullscreenRes25Hz,
        		 dmtoset30	  = m_AutoChangeFullscrRes.dmFullscreenRes30Hz,
        		 dmtosetOther = m_AutoChangeFullscrRes.dmFullscreenResOther;        		         		 
		if(!m_AutoChangeFullscrRes.bEnabled) 
		{	
			GetCurDispMode(dmtoset24, m_f_hmonitor);
			dmtosetOther = dmtoset30 = dmtoset25 = dmtoset24;
		}
		CString str;

		ComboBox_ResetContent(m_dispmode24combo);
		ComboBox_ResetContent(m_dispmode25combo);
		ComboBox_ResetContent(m_dispmode30combo);
		ComboBox_ResetContent(m_dispmodeOthercombo);
		m_dms.RemoveAll();
	
		for(int i = 0, j = 0, ModeExist = true;  ; i++)
		{
			ModeExist = GetDispMode(i, dm, m_f_hmonitor);
			if (!ModeExist) break;   
			if(dm.bpp <= 8) continue;
			m_dms.Add(dm);
			str.Format(_T("%dx%d %dbpp %dHz"), dm.size.cx, dm.size.cy, dm.bpp, dm.freq);
			if (dm.dmDisplayFlags == DM_INTERLACED) str+=_T(" interlaced");

			m_dispmode24combo.AddString(str);
			m_dispmode25combo.AddString(str);
			m_dispmode30combo.AddString(str);
			m_dispmodeOthercombo.AddString(str);			

			if(iSel_24 < 0 && dmtoset24.fValid && dm.size == dmtoset24.size
				&& dm.bpp == dmtoset24.bpp && dm.freq == dmtoset24.freq) iSel_24 = j;
			if(iSel_25 < 0 && dmtoset25.fValid && dm.size == dmtoset25.size
				&& dm.bpp == dmtoset25.bpp && dm.freq == dmtoset25.freq) iSel_25 = j;
			if(iSel_30 < 0 && dmtoset30.fValid && dm.size == dmtoset30.size
				&& dm.bpp == dmtoset30.bpp && dm.freq == dmtoset30.freq) iSel_30 = j;
			if(iSel_Other < 0 && dmtosetOther.fValid && dm.size == dmtosetOther.size
				&& dm.bpp == dmtosetOther.bpp && dm.freq == dmtosetOther.freq) iSel_Other = j;			

			j++;
		}
		m_dispmode24combo.SetCurSel(iSel_24);
		m_dispmode25combo.SetCurSel(iSel_25);
		m_dispmode30combo.SetCurSel(iSel_30);
		m_dispmodeOthercombo.SetCurSel(iSel_Other);		
}
