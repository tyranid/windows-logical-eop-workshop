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

NTSTATUS RegistryCallbackFunc(
  _In_     PVOID CallbackContext,
  _In_opt_ PVOID Argument1,
  _In_opt_ PVOID Argument2
);

struct RegistryObjectContext
{
  BOOLEAN KernelMode;
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, RegistryCallbackFunc)
#pragma alloc_text (PAGE, InstallRegistryCallback)
#pragma alloc_text (PAGE, UninstallRegistryCallback)
#pragma alloc_text (PAGE, ToggleRegistryDebug)
#endif

static LARGE_INTEGER RegistryCookie = { 0 };
static BOOLEAN g_debug_enable = FALSE;

VOID ToggleRegistryDebug()
{
  g_debug_enable = !g_debug_enable;
  DBGPRINT("Registry Debug is %s\r\n", g_debug_enable ? "ENABLED" : "DISABLED");
}

NTSTATUS RegistryCallbackFunc(
  _In_     PVOID CallbackContext,
  _In_opt_ PVOID Argument1,
  _In_opt_ PVOID Argument2
)
{
  UNREFERENCED_PARAMETER(CallbackContext);
  
  REG_NOTIFY_CLASS notify_class = (REG_NOTIFY_CLASS)(ULONG_PTR)Argument1;
  switch (notify_class)
  {
  case RegNtCallbackObjectContextCleanup:
  {
    PREG_CALLBACK_CONTEXT_CLEANUP_INFORMATION op = Argument2;
    if (op->ObjectContext)
      ExFreePoolWithTag(op->ObjectContext, DRIVER_POOL_TAG);
  }
  break;
  case RegNtPostDeleteKey:
  {
    PREG_POST_OPERATION_INFORMATION postop = Argument2;
    PREG_DELETE_KEY_INFORMATION data = postop->PreInformation;

    if (data->ObjectContext)
    {
      struct RegistryObjectContext* ctx = data->ObjectContext;
      if (ctx->KernelMode)
      {
        PWSTR name = NULL;
        PWSTR Token = SerializeEffectiveToken();
        PCUNICODE_STRING ObjectName = NULL;
        if (postop->Status == STATUS_SUCCESS && NT_SUCCESS(CmCallbackGetKeyObjectIDEx(&RegistryCookie, postop->Object, NULL, &ObjectName, 0)))
        {
          name = UnicodeStringToString(ObjectName);
        }

        if (g_debug_enable)
        {
          PrintContext(KernelMode);
          DBGPRINT("Registry Kernel Mode Delete: %ls\n", name);
        }
        if (ObjectName)
        {
          CmCallbackReleaseKeyObjectIDEx(ObjectName);
        }
        if (name)
        {
          ExFreePoolWithTag(name, DRIVER_POOL_TAG);
        }
        if (Token)
        {
          ExFreePoolWithTag(Token, DRIVER_POOL_TAG);
        }
      }
    }

  }
  break;

  case RegNtPostOpenKeyEx:
  {
    PREG_POST_OPERATION_INFORMATION postop = Argument2;
    PREG_OPEN_KEY_INFORMATION_V1 data = postop->PreInformation;
    if (data->CheckAccessMode == KernelMode)
    {    
      if (postop->Status != STATUS_REPARSE)
      {
        struct RegistryObjectContext* ctx = 
          ExAllocatePoolWithTag(PagedPool, sizeof(struct RegistryObjectContext), DRIVER_POOL_TAG);
        ctx->KernelMode = TRUE;
        CmSetCallbackObjectContext(postop->Object, &RegistryCookie, ctx, NULL);
      }
    }
  }
  break;
  case RegNtPostCreateKeyEx:
  {
    PREG_POST_OPERATION_INFORMATION postop = Argument2;
    PREG_CREATE_KEY_INFORMATION_V1 data = postop->PreInformation;
    if (data->CheckAccessMode == KernelMode)
    {
      PWSTR Token = SerializeEffectiveToken();
      if (postop->Status != STATUS_REPARSE)
      {
        PWSTR name = NULL;
        PCUNICODE_STRING ObjectName = NULL;

        const wchar_t* disposition = *data->Disposition == REG_CREATED_NEW_KEY ? L"New Key" : L"Opened Key";
        if (postop->Status == STATUS_SUCCESS && NT_SUCCESS(CmCallbackGetKeyObjectIDEx(&RegistryCookie, postop->Object, NULL, &ObjectName, 0)))
        {
          name = UnicodeStringToString(ObjectName);
        }
        else
        {
          name = UnicodeStringToString(data->CompleteName);
        }

        struct RegistryObjectContext* ctx = ExAllocatePoolWithTag(PagedPool, sizeof(struct RegistryObjectContext), DRIVER_POOL_TAG);
        ctx->KernelMode = TRUE;
        CmSetCallbackObjectContext(postop->Object, &RegistryCookie, ctx, NULL);

        if (g_debug_enable)
        {
          PrintContext(data->CheckAccessMode);
          DBGPRINT("Registry Kernel Mode Create: %ls\n", name);
          DBGPRINT("Status: %08X, Disposition: %ls\n", postop->Status, disposition);
          DBGPRINT("Caller: %ls\n", Token);
          DBGPRINT("OBJ_OPENLINK: %s, Volatile: %s\n", BtoS((data->Attributes & OBJ_OPENLINK) == OBJ_OPENLINK), BtoS((data->Options & REG_OPTION_VOLATILE) == REG_OPTION_VOLATILE));
        }

        if (ObjectName)
        {
          CmCallbackReleaseKeyObjectIDEx(ObjectName);
        }

        if (name)
        {
          ExFreePoolWithTag(name, DRIVER_POOL_TAG);
        }
      }
      ExFreePoolWithTag(Token, DRIVER_POOL_TAG);
    }
  }
  break;
  default:
    break;
  }

  return STATUS_SUCCESS;
}

NTSTATUS InstallRegistryCallback()
{
  return CmRegisterCallback(RegistryCallbackFunc, NULL, &RegistryCookie);
}

void UninstallRegistryCallback()
{
  CmUnRegisterCallback(RegistryCookie);
}