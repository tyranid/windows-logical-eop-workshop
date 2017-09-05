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
#include "ScopedHandle.h"

using namespace std;

void RunTestCreateFile(void* handle);

void RunDemo5(void* hBinding)
{
  if (!TestLoadLibrary(hBinding, GetArgument("Specify library name").c_str()))
  {
    printf("Error calling RPC function\n");
  }
}

void RunDemo6(void* hBinding)
{
  if (!TestLoadLibraryCanonical(hBinding, GetArgument("Specify library name").c_str()))
  {
    printf("Error calling RPC function\n");
  }
}

void RunDemo7(void* hBinding)
{
  if (!TestLoadLibraryTocTou(hBinding, GetArgument("Specify library name").c_str()))
  {
    printf("Error calling RPC function\n");
  }
}

static void RunDemo8(void* hBinding)
{
  if (!TestLoadLibraryTocTouHardened(hBinding, GetArgument("Specify library name").c_str()))
  {
    printf("Error calling RPC function\n");
  }
}

static void RunDemo9(void* hBinding)
{
  if (!TestCreateProcess(hBinding))
  {
    printf("Error calling RPC function\n");
  }
}

static void RunDemo10(void*) {  
  ScopedHandle hDriver(CreateFile(L"\\\\.\\WorkshopDriver", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr));
  if (hDriver.IsInvalid())
  {
    PrintError("Error opening device driver", GetLastError());
    return;
  }

  RunTestCreateFile(hDriver.Get());
}

static void NoDemo(void*)
{
  printf("No Demo Available\n");
}

static void RunDemosInternal(void* hBinding)
{
  std::vector<MenuOption> menu = {
    { "(N/A)", NoDemo }, // 1
    { "(N/A)", NoDemo }, // 2
    { "(N/A)", NoDemo }, // 3
    { "(N/A)", NoDemo }, // 4
    { "Exploiting Path Canonicalization", RunDemo5 }, // 5
    { "Exploiting Named Streams", RunDemo6 }, // 6
    { "TOCTOU on Name", RunDemo7 }, // 7
    { "Symbolic Link TOCTOU", RunDemo8 }, // 8
    { "DosDevices Redirect", RunDemo9 }, // 9
    { "Privileged Resource Creation", RunDemo10 }, // 10
    { "(N/A)", NoDemo }, // 11
  };

  RunMenu("Select Demo", menu, hBinding);
}

void RunDemos(void*) {
  WCHAR* szStringBinding = NULL;
  handle_t hBinding = NULL;

  RpcStringBindingCompose(
    NULL,
    L"ncalrpc",
    nullptr,
    L"RpcWorkshop",
    L"Security=impersonation static false",
    &szStringBinding);

  RpcBindingFromStringBinding(
    szStringBinding,
    &hBinding);

  RpcTryExcept
  {
    RunDemosInternal(hBinding);
  }
  RpcExcept(1)
  {
    PrintError("RPC Error", RpcExceptionCode());
  }
  RpcEndExcept

  RpcStringFree(&szStringBinding);
  RpcBindingFree(&hBinding);
}
