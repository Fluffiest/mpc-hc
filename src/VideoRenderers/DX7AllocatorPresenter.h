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

#pragma once

#include "AllocatorCommon7.h"
#include <ddraw.h>
#include <d3d.h>

namespace DSObjects
{

	class CDX7AllocatorPresenter
		: public ISubPicAllocatorPresenterImpl
	{
	protected:
		CSize	m_ScreenSize;

		CComPtr<IDirectDraw7>	m_pDD;
		CComQIPtr<IDirect3D7, &IID_IDirect3D7>	m_pD3D;
		CComPtr<IDirect3DDevice7>	m_pD3DDev;

		CComPtr<IDirectDrawSurface7>	m_pPrimary;
		CComPtr<IDirectDrawSurface7>	m_pBackBuffer;
		CComPtr<IDirectDrawSurface7>	m_pVideoTexture;
		CComPtr<IDirectDrawSurface7>	m_pVideoSurface;

		virtual HRESULT CreateDevice();
		virtual HRESULT AllocSurfaces();
		virtual void DeleteSurfaces();

		void SendResetRequest();

	public:
		CDX7AllocatorPresenter(HWND hWnd, HRESULT& hr);

		// ISubPicAllocatorPresenter
		STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);
		STDMETHODIMP_(bool) Paint(bool fAll);
		STDMETHODIMP GetDIB(BYTE* lpDib, DWORD* size);
		STDMETHODIMP_(bool) ResetDevice();
	};

}
