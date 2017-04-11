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
#include "DummyExe.h"

#include <sstream>
#include <memory>

bool QueryTokenBool(HANDLE token, TOKEN_INFORMATION_CLASS token_info)
{
  DWORD value;
  DWORD ret_length;

  if (GetTokenInformation(token, token_info, &value, sizeof(value), &ret_length))
  {
    return value != 0;
  }
  return false;
}

std::wstring QueryTokenIL(HANDLE token)
{
  std::size_t size = sizeof(TOKEN_MANDATORY_LABEL) + SECURITY_MAX_SID_SIZE;
  std::unique_ptr<BYTE> buffer = std::unique_ptr<BYTE>(new BYTE[size]);
  DWORD ret_length;
  if (GetTokenInformation(token, TokenIntegrityLevel, buffer.get(), static_cast<DWORD>(size), &ret_length))
  {
    PTOKEN_MANDATORY_LABEL label = reinterpret_cast<PTOKEN_MANDATORY_LABEL>(buffer.get());
    DWORD il = *GetSidSubAuthority(label->Label.Sid, 0);
    switch (il)
    {
    case SECURITY_MANDATORY_SYSTEM_RID:
      return L"System";
    case SECURITY_MANDATORY_HIGH_RID:
      return L"High";
    case SECURITY_MANDATORY_MEDIUM_RID:
      return L"Medium";
    case SECURITY_MANDATORY_LOW_RID:
      return L"Low";
    case SECURITY_MANDATORY_UNTRUSTED_RID:
      return L"Untrusted";
    default:
      std::wstringstream ss;
      ss << L"0x" << std::hex << il;
      return ss.str();
    }
  }
  return L"Medium";
}

void Hello()
{
  HANDLE token;
  bool ui_access = false;
  bool elevated = false;
  std::wstring il = L"Medium";
  std::wstringstream ss;

  if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
  {
    ui_access = QueryTokenBool(token, TokenUIAccess);
    elevated = QueryTokenBool(token, TokenElevation);
    il = QueryTokenIL(token);
    CloseHandle(token);
  }

  ss << L"Hello From Process " << GetCurrentProcessId() << std::endl
    << "Integrity: " << il << std::endl
    << "UIAccess: " << std::boolalpha << ui_access << std::endl
    << "Elevated: " << elevated;
  MessageBox(nullptr, ss.str().c_str(), L"Hello", MB_OK | MB_ICONINFORMATION);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
  Hello();
  return 0;
}
