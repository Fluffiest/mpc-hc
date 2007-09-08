/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "mplayerc.h"
#include "PixelShaderCompiler.h"

CPixelShaderCompiler::CPixelShaderCompiler(IDirect3DDevice9* pD3DDev, bool fStaySilent)
	: m_pD3DDev(pD3DDev)
	, m_pD3DXCompileShader(NULL)
	, m_pD3DXDisassembleShader(NULL)
{
	HINSTANCE		hDll;
	hDll = AfxGetMyApp()->GetD3X9Dll();

	if(hDll)
	{
		m_pD3DXCompileShader = (D3DXCompileShaderPtr)GetProcAddress(hDll, "D3DXCompileShader");
		m_pD3DXDisassembleShader = (D3DXDisassembleShaderPtr)GetProcAddress(hDll, "D3DXDisassembleShader");
	}

	if(!fStaySilent)
	{
		if(!hDll)
		{
			AfxMessageBox(ResStr(IDS_PIXELSHADERCOMPILER_0), MB_OK);
		}
		else if(!m_pD3DXCompileShader || !m_pD3DXDisassembleShader) 
		{
			AfxMessageBox(ResStr(IDS_PIXELSHADERCOMPILER_1), MB_OK);
		}
	}
}

CPixelShaderCompiler::~CPixelShaderCompiler()
{
}

HRESULT CPixelShaderCompiler::CompileShader(
    LPCSTR pSrcData,
    LPCSTR pFunctionName,
    LPCSTR pProfile,
    DWORD Flags,
    IDirect3DPixelShader9** ppPixelShader,
	CString* disasm,
	CString* errmsg)
{
	if(!m_pD3DXCompileShader || !m_pD3DXDisassembleShader)
		return E_FAIL;

	HRESULT hr;

	CComPtr<ID3DXBuffer> pShader, pDisAsm, pErrorMsgs;
	hr = m_pD3DXCompileShader(pSrcData, strlen(pSrcData), NULL, NULL, pFunctionName, pProfile, Flags, &pShader, &pErrorMsgs, NULL);

	if(FAILED(hr))
	{
		if(errmsg)
		{
			CStringA msg = "Unexpected compiler error";

			if(pErrorMsgs)
			{
				int len = pErrorMsgs->GetBufferSize();
				memcpy(msg.GetBufferSetLength(len), pErrorMsgs->GetBufferPointer(), len);
			}

			*errmsg = msg;
		}

		return hr;
	}

	if(ppPixelShader)
	{
		if(!m_pD3DDev) return E_FAIL;
		hr = m_pD3DDev->CreatePixelShader((DWORD*)pShader->GetBufferPointer(), ppPixelShader);
		if(FAILED(hr)) return hr;
	}

	if(disasm)
	{
		hr = m_pD3DXDisassembleShader((DWORD*)pShader->GetBufferPointer(), FALSE, NULL, &pDisAsm);
		if(SUCCEEDED(hr) && pDisAsm) *disasm = CStringA((const char*)pDisAsm->GetBufferPointer());
	}

	return S_OK;
}
