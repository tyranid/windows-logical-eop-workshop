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

#include "driver.h"

static VOID CreateNotifyRoutine(_Inout_ PEPROCESS Process,
  _In_ HANDLE ProcessId,
  _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, PrintContext)
#pragma alloc_text (PAGE, ProcessModeToString)
#pragma alloc_text (PAGE, CreateNotifyRoutine)
#pragma alloc_text (PAGE, InstallProcessCallback)
#pragma alloc_text (PAGE, UninstallProcessCallback)
#pragma alloc_text (PAGE, ToggleProcessDebug)
#endif 

const char* PsGetProcessImageFileName(PEPROCESS Process);
NTSTATUS PsQueryProcessCommandLine(PEPROCESS, WCHAR* Buffer, ULONG NumberOfBytes, INT Unknown, ULONG* BytesCopied);
static BOOLEAN g_debug_enable = FALSE;

VOID ToggleProcessDebug()
{
  g_debug_enable = !g_debug_enable;
  DBGPRINT("Process Debug is %s\r\n", g_debug_enable ? "ENABLED" : "DISABLED");
}

VOID CreateNotifyRoutine(
  _Inout_ PEPROCESS Process,
  _In_ HANDLE ProcessId,
  _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
  PACCESS_TOKEN ProcessToken = NULL;
  PACCESS_TOKEN CreatorToken = NULL;
  BOOLEAN Impersonating = TRUE;
  BOOLEAN CopyOnOpen = FALSE;
  BOOLEAN EffectiveOnly = FALSE;
  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel = SecurityAnonymous;
  BOOLEAN PrintProcessInfo = FALSE;
  PWSTR ImageFileName = NULL;
  KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
  PWSTR CreatorTokenString = NULL;
  PWSTR ProcessTokenString = NULL;
  const char* ParentProcessImageName = NULL;

  if (!CreateInfo)
  {
    return;
  }

  CreatorToken = PsReferenceImpersonationToken(PsGetCurrentThread(), &CopyOnOpen, &EffectiveOnly, &ImpersonationLevel);
  if (!CreatorToken)
  {
    Impersonating = FALSE;
    CreatorToken = PsReferencePrimaryToken(PsGetCurrentProcess());
  }
  ProcessToken = PsReferencePrimaryToken(Process);
  if (!CompareTokens(CreatorToken, ProcessToken) || PreviousMode == KernelMode)
  {
    PrintProcessInfo = TRUE;
  }

  if (PrintProcessInfo)
  {
    PEPROCESS CreatorProcess = NULL;
    PEPROCESS ParentProcess = NULL;
    if (!NT_SUCCESS(PsLookupProcessByProcessId(CreateInfo->ParentProcessId, &ParentProcess)))
      ParentProcess = NULL;
    if (CreateInfo->ParentProcessId != CreateInfo->CreatingThreadId.UniqueProcess)
    {
      if (!NT_SUCCESS(PsLookupProcessByProcessId(CreateInfo->CreatingThreadId.UniqueProcess, &CreatorProcess)))
        CreatorProcess = NULL;
    }

    ImageFileName = UnicodeStringToString(CreateInfo->ImageFileName);
    CreatorTokenString = SerializeToken(CreatorToken);
    ProcessTokenString = SerializeToken(ProcessToken);
    ParentProcessImageName = ParentProcess ? PsGetProcessImageFileName(ParentProcess) : NULL;

    if (g_debug_enable)
    {
      PrintContext(PreviousMode);
      DBGPRINT("Creating Process with Mismatched Token: %ls\r\n", ImageFileName);
      DBGPRINT("New PID: %d, ImpersonationLevel: %ls\r\n", HandleToInt(ProcessId), Impersonating ? GetTokenImpersonationLevel(CreatorToken) : L"None");
      DBGPRINT("Process Token: %ls\r\n", ProcessTokenString);
    }
  }

  //error:
  if (CreatorToken)
  {
    if (Impersonating)
    {
      PsDereferenceImpersonationToken(CreatorToken);
    }
    else
    {
      PsDereferencePrimaryToken(CreatorToken);
    }
  }
  if (ProcessToken)
  {
    PsDereferencePrimaryToken(ProcessToken);
  }
  if (ImageFileName)
  {
    ExFreePool(ImageFileName);
  }
  if (CreatorTokenString)
  {
    ExFreePool(CreatorTokenString);
  }
  if (ProcessTokenString)
  {
    ExFreePool(ProcessTokenString);
  }
}

PCSTR ProcessModeToString(KPROCESSOR_MODE mode)
{
  if (mode == KernelMode)
  {
    return "Kernel";
  }
  return "User";
}

void PrintContext(KPROCESSOR_MODE RequestorMode)
{
  PWSTR Token = SerializeEffectiveToken();
  DBGPRINT("Current Process: %p - %s\r\n", PsGetCurrentProcessId(), PsGetProcessImageFileName(PsGetCurrentProcess()));
  DBGPRINT("Current Caller %ls\r\n", Token);
  ExFreePoolWithTag(Token, DRIVER_POOL_TAG);
  DBGPRINT("Requestor Mode: %s, Thread Mode: %s\r\n", ProcessModeToString(RequestorMode), ProcessModeToString(ExGetPreviousMode()));
}

NTSTATUS InstallProcessCallback()
{
  return PsSetCreateProcessNotifyRoutineEx(CreateNotifyRoutine, FALSE);
}

void UninstallProcessCallback()
{
  PsSetCreateProcessNotifyRoutineEx(CreateNotifyRoutine, TRUE);
}