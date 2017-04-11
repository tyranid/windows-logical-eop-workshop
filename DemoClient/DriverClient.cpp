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

#include "stdafx.h"
#include <iostream>
#include <device_control_ioctl.h>

using namespace std;

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

void RunTestGeneral(HANDLE handle, ControlCode code)
{
  wstring path = GetArgument("Specify native path");
  DWORD bytes_returned;
  if (!DeviceIoControl(handle, MAKE_IOCTL(code),
    const_cast<wchar_t*>(path.c_str()), path.size() * sizeof(wchar_t), nullptr, 0, &bytes_returned, nullptr))
  {
    PrintError("Error issuing device control", GetLastError());
  }
}

void RunTestNoData(HANDLE handle, ControlCode code)
{
  DWORD bytes_returned;
  if (!DeviceIoControl(handle, MAKE_IOCTL(code),
    nullptr, 0, nullptr, 0, &bytes_returned, nullptr))
  {
    PrintError("Error issuing device control", GetLastError());
  }
}

void RunTestCallerToken(HANDLE handle, ControlCode code)
{
  wstring option = GetArgument("Impersonate Admin Token? (Y/N)");
  if (option.size() > 0)
  {
    bool impersonate = tolower(option[0]) == 'y';

    if (impersonate)
    {
      ScopedHandle proc_token;
      if (!OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, proc_token.Ptr()))
      {
        PrintError("Couldn't open process token", GetLastError());
        return;
      }

      ScopedHandle imp_token;
      DWORD return_length = 0;
      if (!GetTokenInformation(proc_token.Get(), TokenLinkedToken, imp_token.Ptr(), sizeof(HANDLE), &return_length))
      {
        PrintError("Couldn't get linked token", GetLastError());
        return;
      }

      if (!ImpersonateLoggedOnUser(imp_token.Get()))
      {
        PrintError("Couldn't impersonate token", GetLastError());
        return;
      }
    }

    DWORD bytes_returned;
    BOOL result = DeviceIoControl(handle, MAKE_IOCTL(code),
      nullptr, 0, nullptr, 0, &bytes_returned, nullptr);

    if (impersonate)
    {
      RevertToSelf();
    }

    if (!result)
    {
      PrintError("Error issuing device control", GetLastError());
    }
  }
}

void RunTestCreateFile(void* handle)
{
  RunTestGeneral(handle, ControlCreateFile);
}

void RunTestCreateFileSecure(void* handle)
{
  RunTestGeneral(handle, ControlCreateFileSecure);
}

void RunTestCreateDir(void* handle)
{
  RunTestGeneral(handle, ControlCreateDir);
}

void RunTestCreateDirSecure(void* handle)
{
  RunTestGeneral(handle, ControlCreateDirSecure);
}

void RunTestCreateKey(void* handle)
{
  RunTestGeneral(handle, ControlCreateKey);
}

void RunTestCreateKeySecure(void* handle)
{
  RunTestGeneral(handle, ControlCreateKeySecure);
}

void RunTestCreateKeyRelative(void* handle)
{
  RunTestGeneral(handle, ControlCreateKeyRelative);
}

void RunTestCallerIsElevated(void* handle)
{
  RunTestCallerToken(handle, ControlCallerIsElevated);
}

void RunTestCallerIsElevatedSecure(void* handle)
{
  RunTestCallerToken(handle, ControlCallerIsElevatedSecure);
}

void RunTestBadImpersonation(void* handle)
{
  RunTestCallerToken(handle, ControlBadImpersonation);
}

void RunToggleProcessDebug(void* handle)
{
  RunTestNoData(handle, ControlToggleProcessDebug);
}

void RunToggleIrpDebug(void* handle)
{
  RunTestNoData(handle, ControlToggleIrpDebug);
}

void RunToggleRegistryDebug(void* handle)
{
  RunTestNoData(handle, ControlToggleRegistryDebug);
}

void RunIoTest(void* handle)
{
  RunTestNoData(handle, ControlRunIoTest);
}

void RunDriverFileTests(void* handle)
{
  std::vector<MenuOption> menu = {
    { "Create File", RunTestCreateFile },
    { "Create File Secure", RunTestCreateFileSecure },
    { "Create Dir", RunTestCreateDir },
    { "Create Dir Secure", RunTestCreateDirSecure },
  };

  RunMenu("Driver File Tests", menu, handle);
}

void RunDriverKeyTests(void* handle)
{
  std::vector<MenuOption> menu = {
    { "Create Key", RunTestCreateKey },
    { "Create Key Secure", RunTestCreateKeySecure },
    { "Create Key Relative", RunTestCreateKeyRelative },
  };

  RunMenu("Driver Key Tests", menu, handle);
}

void RunDriverTokenTests(void* handle)
{
  std::vector<MenuOption> menu = {
    { "Check Token Elevated", RunTestCallerIsElevated },
    { "Check Token Elevated Secure", RunTestCallerIsElevatedSecure },
    { "Bad Impersonation", RunTestBadImpersonation },
  };

  RunMenu("Driver Token Tests", menu, handle);
}

void RunDriverDebugOptions(void* handle)
{
  std::vector<MenuOption> menu = {
    { "Toggle Process Debug", RunToggleProcessDebug },
    { "Toggle IRP Debug", RunToggleIrpDebug },
    { "Toggle Registry Debug", RunToggleRegistryDebug },
	  { "Run IO Test", RunIoTest },
  };

  RunMenu("Driver Debug Options", menu, handle);
}

void RunDriverTests(void*)
{
  ScopedHandle hDriver(CreateFile(L"\\\\.\\WorkshopDriver", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr));
  if (hDriver.IsInvalid())
  {
    PrintError("Error opening device driver", GetLastError());
    return;
  }

  std::vector<MenuOption> menu = {
    { "Driver File Tests", RunDriverFileTests },
    { "Driver Key Tests", RunDriverKeyTests },
    { "Driver Token Tests", RunDriverTokenTests },
    { "Driver Debug Options", RunDriverDebugOptions },
  };

  RunMenu("Driver Tests", menu, hDriver.Get());
}
