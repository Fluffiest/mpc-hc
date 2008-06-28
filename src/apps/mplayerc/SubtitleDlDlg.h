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

#pragma once

#include "ISDb.h"
#include "afxwin.h"


// CSubtitleDlDlg dialog

class CSubtitleDlDlg : public CResizableDialog
{
//	DECLARE_DYNAMIC(CSubtitleDlDlg)

private:
	CList<isdb_movie> m_movies;

	enum {COL_FILENAME, COL_LANGUAGE, COL_FORMAT, COL_DISC, COL_TITLES};

	CImageList m_onoff;
	int GetChecked(int iItem);
	void SetChecked(int iItem, int iChecked);

public:
	CSubtitleDlDlg(CList<isdb_movie>& movies, CWnd* pParent = NULL);   // standard constructor
	virtual ~CSubtitleDlDlg();

	bool m_fReplaceSubs;
	CList<isdb_subtitle> m_selsubs;

// Dialog Data
	enum { IDD = IDD_SUBTITLEDL_DLG };
	CListCtrl m_list;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnNMClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnUpdateOk(CCmdUI* pCmdUI);
};
