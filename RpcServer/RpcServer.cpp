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
#include "server_h.h"
#include <Shlwapi.h>
#include <ShlObj.h>
#include <string>
#include <sstream>
#include <Softpub.h>
#include <wincrypt.h>
#include <WinTrust.h>

#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "wintrust.lib")

class ScopedHandle
{
  HANDLE _h;
public:
  ScopedHandle(HANDLE h)
  {
    _h = h;
  }

  ScopedHandle(const ScopedHandle&) = delete;

  ~ScopedHandle()
  {
    if (!IsInvalid())
    {
      CloseHandle(_h);
      _h = nullptr;
    }
  }

  bool IsInvalid() const
  {
    return _h == nullptr || _h == INVALID_HANDLE_VALUE;
  }

  HANDLE Get() const
  {
    return _h;
  }
};

bool IsWow64()
{
  BOOL is_wow64 = FALSE;
  if (IsWow64Process(GetCurrentProcess(), &is_wow64) && is_wow64)
  {
    return true;
  }
  return false;
}

extern "C" void* __RPC_USER MIDL_user_allocate(size_t size)
{
	return new char[size];
}

extern "C" void __RPC_USER MIDL_user_free(void* p)
{
	delete[] p;
}

std::wstring GetErrorMessage(int error)
{
	WCHAR buf[1024];
	if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error, 0, buf, _countof(buf), nullptr) > 0)
	{
		return buf;
	}
	else
	{
		std::wstringstream ss;
		ss << "Unknown Error: " << error;
		return ss.str();
	}
}

std::wstring GetPathFromEnv(REFKNOWNFOLDERID folder)
{
	PWSTR value = nullptr;

	if (SUCCEEDED(SHGetKnownFolderPath(folder, KF_FLAG_DONT_VERIFY, nullptr, &value)))
	{
		std::wstring ret = value;
		CoTaskMemFree(value);

		return ret;
	}
	else
	{
		printf("Error\n");
		return L"";
	}
}

#pragma comment (lib, "wintrust")

bool VerifyEmbeddedSignature(LPCWSTR pwszSourceFile)
{
  LONG lStatus;

  WINTRUST_FILE_INFO FileData;
  memset(&FileData, 0, sizeof(FileData));
  FileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
  FileData.pcwszFilePath = pwszSourceFile;
  FileData.hFile = NULL;
  FileData.pgKnownSubject = NULL;

  GUID WVTPolicyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
  WINTRUST_DATA WinTrustData;

  memset(&WinTrustData, 0, sizeof(WinTrustData));

  WinTrustData.cbStruct = sizeof(WinTrustData);
  WinTrustData.pPolicyCallbackData = NULL;
  WinTrustData.pSIPClientData = NULL;
  WinTrustData.dwUIChoice = WTD_UI_NONE;
  WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
  WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;
  WinTrustData.dwStateAction = WTD_STATEACTION_VERIFY;
  WinTrustData.hWVTStateData = NULL;
  WinTrustData.pwszURLReference = NULL;
  WinTrustData.dwUIContext = 0;
  WinTrustData.pFile = &FileData;
  lStatus = WinVerifyTrust(
    NULL,
    &WVTPolicyGUID,
    &WinTrustData);

  bool ret = false;

  switch (lStatus)
  {
  case ERROR_SUCCESS:
    ret = true;
    break;
  default:
    printf("Error verifying file: %ls\n", GetErrorMessage(GetLastError()).c_str());
    break;
  }

  WinTrustData.dwStateAction = WTD_STATEACTION_CLOSE;
  lStatus = WinVerifyTrust(
    NULL,
    &WVTPolicyGUID,
    &WinTrustData);

  return ret;
}

extern "C" boolean TestCreateProcess(
	handle_t hBinding)
{
	printf("TestCreateProcess called\n");
  RPC_STATUS status = RpcImpersonateClient(hBinding);
  if (status == 0)
	{
		STARTUPINFO startInfo = { 0 };
		PROCESS_INFORMATION procInfo = { 0 };

		startInfo.cb = sizeof(startInfo);
		WCHAR cmdline[] = L"c:\\windows\\notepad.exe";

		if (CreateProcess(cmdline, cmdline, nullptr, nullptr,
			FALSE, 0, nullptr, nullptr, &startInfo, &procInfo))
		{
			printf("Created Process: %d\n", procInfo.dwProcessId);
			CloseHandle(procInfo.hThread);
			CloseHandle(procInfo.hProcess);
      RpcRevertToSelf();
			return true;
		}
		else
		{
			printf("Error creating process: %d\n", GetLastError());
      RpcRevertToSelf();
		}
	}
	else
	{
		printf("Error impersonating user: %d\n", status);
	}
	return false;
}

extern "C" boolean TestLoadLibrary(handle_t hBinding, const wchar_t* name)
{
	printf("TestLoadLibrary called\n");
	std::wstring full_path = GetPathFromEnv(FOLDERID_Windows) + L"\\" + name;
	printf("Loading path: %ls\n", full_path.c_str());
	HMODULE hModule = LoadLibrary(full_path.c_str());
	if (hModule != nullptr)
	{
		printf("Loaded module: %p\n", hModule);
		FreeLibrary(hModule);
		return true;
	}
	else
	{
		printf("Error loading module: %ls\n", GetErrorMessage(GetLastError()).c_str());
	}

	return false;
}

extern "C" boolean TestLoadLibraryCanonical(handle_t hBinding, const wchar_t* name)
{
	printf("TestLoadLibraryCanonical called\n");
	if (wcschr(name, '\\') || wcschr(name, '/'))
	{
		printf("Error, name contains path separators\n");
		return false;
	}

	std::wstring full_path = GetPathFromEnv(FOLDERID_Windows) + L"\\" + name;
  return TestLoadLibrary(hBinding, name);
}

extern "C" int TestDuplicateHandle(handle_t hBinding, int handle)
{
  printf("TestDuplicateHandle called\n");
  unsigned long pid;
  RPC_STATUS status = I_RpcBindingInqLocalClientPID(hBinding, &pid);
  if (status != ERROR_SUCCESS)
  {
    printf("Error getting local PID: %ls\n", GetErrorMessage(status).c_str());
    return 0;
  }

  ScopedHandle process(OpenProcess(PROCESS_DUP_HANDLE, FALSE, pid));
  if (process.IsInvalid())
  {
    printf("Error getting opening process: %ls\n", GetErrorMessage(GetLastError()).c_str());
    return 0;
  }

  HANDLE ret;
  if (!DuplicateHandle(process.Get(), (HANDLE)handle, process.Get(), &ret, 0, FALSE, DUPLICATE_SAME_ACCESS))
  {
    printf("Error getting duplicating handle: %ls\n", GetErrorMessage(GetLastError()).c_str());
    return 0;
  }

  return (int)ret;
}

bool CheckFileIsInSystem(const ScopedHandle& handle)
{
  WCHAR path[MAX_PATH];
  if (GetFinalPathNameByHandleW(handle.Get(), path, MAX_PATH, 0) == 0)
  {
    printf("Error checking executable file path: %ls\n", GetErrorMessage(GetLastError()).c_str());
    return false;
  }

  printf("Referenced File is %ls\n", path);
  PathRemoveFileSpec(path);
  printf("Directory is %ls\n", path);

  std::wstring system = GetPathFromEnv(IsWow64() ? FOLDERID_SystemX86 : FOLDERID_System);
  system = L"\\\\?\\" + system;
  return _wcsicmp(path, system.c_str()) == 0;
}

extern "C" boolean TestLoadLibraryTocTou(handle_t hBinding, const wchar_t* lib_path)
{
  if (VerifyEmbeddedSignature(lib_path))
  {
    HMODULE hModule = LoadLibrary(lib_path);
    if (hModule != nullptr)
    {
      printf("Loaded module: %p\n", hModule);
      FreeLibrary(hModule);
      return true;
    }
    else
    {
      printf("Error loading module: %ls\n", GetErrorMessage(GetLastError()).c_str());
    }
  }
  else
  {
    printf("Module not in system directory\n");
  }

  return false;
}

extern "C" boolean TestLoadLibraryTocTouHardened(handle_t hBinding, const wchar_t* lib_path)
{
  LPWSTR ext = PathFindExtensionW(lib_path);
  printf("Extension is: %ls\n", ext);
  if (ext == nullptr || _wcsicmp(ext, L".dll") != 0)
  {
    printf("Invalid DLL extension %ls\n", ext);
    return false;
  }

  // Lock DLL file over calls.
  ScopedHandle handle(CreateFile(lib_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
  if (handle.IsInvalid())
  {
    printf("Error opening dll file: %ls\n", GetErrorMessage(GetLastError()).c_str());
    return false;
  }

  if (!CheckFileIsInSystem(handle))
  {
    printf("File not in system directory\n");
    return false;
  }

  HMODULE hModule = LoadLibrary(lib_path);
  if (hModule != nullptr)
  {
    printf("Loaded module: %p\n", hModule);
    FreeLibrary(hModule);
    return true;
  }
  else
  {
    printf("Error loading module: %ls\n", GetErrorMessage(GetLastError()).c_str());
  }
  
  return false;
}

RPC_STATUS CALLBACK SecurityCallback(RPC_IF_HANDLE /* hInterface */, void* pBindingHandle)
{
  RPC_WSTR StringBinding = nullptr;
  RPC_STATUS status = RpcBindingToStringBindingW(pBindingHandle, &StringBinding);
  if (status == 0)
  {
    printf("Called on binding '%ls'\n", StringBinding);
  }
  if (StringBinding)
    RpcStringFreeW(&StringBinding);
	return RPC_S_OK; // Always allow anyone.
}

int _tmain(int argc, _TCHAR* argv[])
{
	RPC_STATUS status;
	
	status = RpcServerUseProtseqEp(
		L"ncalrpc",
		RPC_C_PROTSEQ_MAX_REQS_DEFAULT, 
    L"RpcWorkshop",
	  nullptr);

	if (status)
	{
		printf("Error using protocol sequence: %d\n", status); 
		exit(status);
	}

  status = RpcServerUseProtseqEp(
    L"ncacn_np",
    RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
    L"\\pipe\\RpcWorkshop",
    nullptr);

  if (status)
  {
    printf("Error using protocol sequence: %d\n", status);
    exit(status);
  }

  status = RpcServerUseProtseqEp(
    L"ncacn_ip_tcp",
    RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
    nullptr,
    nullptr);

  if (status)
  {
    printf("Error using protocol sequence: %d\n", status);
    exit(status);
  }

	RPC_BINDING_VECTOR* vector;

	status = RpcServerInqBindings(&vector);

	if (status)
	{
		printf("Error getting bindings: %d\n", status);
		exit(status);
	}

	status = RpcEpRegister(RpcServer_v1_0_s_ifspec, vector, nullptr, L"Hello");
	if (status)
	{
		printf("Error registering bindings: %d\n", status);
		exit(status);
	}

	status = RpcServerRegisterIfEx(
		RpcServer_v1_0_s_ifspec,
		NULL,
		NULL,
		RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH,
		RPC_C_LISTEN_MAX_CALLS_DEFAULT,
		SecurityCallback
		);	

	if (status)
	{
		printf("Error registering interface: %d\n", status); 
		exit(status);
	}
	
	status = RpcServerListen(
		1,
		RPC_C_LISTEN_MAX_CALLS_DEFAULT, 
		FALSE); 

	if (status)
		exit(status);

	return 0;
}

