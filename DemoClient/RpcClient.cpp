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
#include <server_h.h>

#pragma comment(lib, "rpcrt4.lib")

using namespace std;

extern "C" void* __RPC_USER MIDL_user_allocate(size_t size)
{
	return new char[size];
}

extern "C" void __RPC_USER MIDL_user_free(void* p)
{
	delete[] p;
}

void RunTestCreateProcess(void* hBinding)
{
  if (!TestCreateProcess(hBinding))
  {
    printf("Error calling RPC function\n");
  }
}

void RunTestLoadLibrary(void* hBinding)
{
  if (!TestLoadLibrary(hBinding, GetArgument("Specify library name").c_str()))
  {
    printf("Error calling RPC function\n");
  }
}

void RunTestLoadLibraryCanonical(void* hBinding)
{
  if (!TestLoadLibraryCanonical(hBinding, GetArgument("Specify library name").c_str()))
  {
    printf("Error calling RPC function\n");
  }
}

void RunTestDuplicateHandle(void* hBinding)
{
  int handle = TestDuplicateHandle(hBinding, GetArgumentNumeric("Specify handle to duplicate"));
  if (handle <= 0)
  {
    printf("Error calling RPC function\n");
  }
  else
  {
    printf("Handle duplicated as %d/0x%x\n", handle, handle);
  }
}

void RunTestLoadLibraryTocTou(void* hBinding)
{
  if (!TestLoadLibraryTocTou(hBinding, GetArgument("Specify library name").c_str()))
  {
    printf("Error calling RPC function\n");
  }
}

void RunTestLoadLibraryTocTouHardened(void* hBinding)
{
  if (!TestLoadLibraryTocTouHardened(hBinding, GetArgument("Specify library name").c_str()))
  {
    printf("Error calling RPC function\n");
  }
}

void RunOps(handle_t hBinding)
{
  std::vector<MenuOption> menu = {
    { "Test Create Process", RunTestCreateProcess },
    { "Test Load Library", RunTestLoadLibrary },
    { "Test Load Library with Path Check", RunTestLoadLibraryCanonical },
    { "Test Load Library TOCTOU", RunTestLoadLibraryTocTou },
    { "Test Load Library TOCTOU Hardened", RunTestLoadLibraryTocTouHardened },
    { "Duplicate handle", RunTestDuplicateHandle },
  };

  RunMenu("RPC Tests", menu, hBinding);
}

void RunRpcClient(LPWSTR protocol, LPWSTR endpoint)
{
	WCHAR* szStringBinding = NULL;
	handle_t hBinding = NULL;

  RpcStringBindingCompose(
    NULL,
    protocol,
    nullptr,
    endpoint,
    L"Security=impersonation static false",
    &szStringBinding);
  
	RpcBindingFromStringBinding(
		szStringBinding,
		&hBinding);

	RpcTryExcept
	{
    RunOps(hBinding);
	}
	RpcExcept(1)
	{
		PrintError("RPC Error", RpcExceptionCode());
	}
	RpcEndExcept

	RpcStringFree(&szStringBinding);
	RpcBindingFree(&hBinding);
}

void RunRpcClientTcp(void*)
{
  RunRpcClient(L"ncacn_ip_tcp", nullptr);
}

void RunRpcClientAlpc(void*)
{
  RunRpcClient(L"ncalrpc", L"RpcWorkshop");
}

void RunRpcClientNamedPipe(void*)
{
  RunRpcClient(L"ncacn_np", L"\\pipe\\RpcWorkshop");
}

void RunRpcClient(void*)
{
  std::vector<MenuOption> menu = {

      { "ALPC", RunRpcClientAlpc },
      { "Named Pipe", RunRpcClientNamedPipe },
      { "TCP", RunRpcClientTcp },
  };

  RunMenu("Select RPC Endpoint", menu, nullptr);
}