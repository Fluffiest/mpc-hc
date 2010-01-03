/*
* $Id: SyncAllocatorPresenter.h 1292 2009-10-03 23:20:26Z ar-jar $
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

// {F9F62627-E3EF-4a2e-B6C9-5D4C0DC3326B}
DEFINE_GUID(CLSID_SyncAllocatorPresenter, 0xf9f62627, 0xe3ef, 0x4a2e, 0xb6, 0xc9, 0x5d, 0x4c, 0xd, 0xc3, 0x32, 0x6b);

HRESULT CreateSyncRenderer(const CLSID& clsid, HWND hWnd, ISubPicAllocatorPresenter** ppAP);