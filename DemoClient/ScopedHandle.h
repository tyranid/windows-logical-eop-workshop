//    This file is part of windows-logical-eop-workshop
//    Copyright (C) James Forshaw 2017
//
//    windows-logical-eop-workshop is free software: you can redistribute
//    it and/or modify it under the terms of the GNU General Public License 
//    as published by the Free Software Foundation, either version 3 of the 
//    License, or (at your option) any later version.
//
//    windows-logical-eop-workshop is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with windows-logical-eop-workshop.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <Windows.h>

class ScopedHandle
{
  HANDLE _h;
public:
  ScopedHandle(HANDLE h)
  {
    _h = h;
  }

  ScopedHandle(const ScopedHandle&) = delete;

  ScopedHandle() : _h(nullptr)
  {
  }

  ~ScopedHandle()
  {
    if (!IsInvalid())
    {
      CloseHandle(_h);
      _h = nullptr;
    }
  }

  bool IsInvalid()
  {
    return _h == nullptr || _h == INVALID_HANDLE_VALUE;
  }

  HANDLE Get()
  {
    return _h;
  }

  HANDLE* Ptr()
  {
    return &_h;
  }
};