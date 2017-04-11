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
#include "device_control_ioctl.h"

NTSTATUS HandleIoControl(PIRP Irp);
NTSTATUS CreateFile(PUNICODE_STRING Path, BOOLEAN Secure, BOOLEAN Directory);
NTSTATUS CreateFileIo(PUNICODE_STRING Path, BOOLEAN Secure, 
    BOOLEAN ForceAccessCheck, BOOLEAN Directory);
KSTART_ROUTINE ImpersonationThread;
NTSTATUS CreateKey(PUNICODE_STRING Path, HANDLE Root, BOOLEAN Secure);
NTSTATUS IsCallerElevated(BOOLEAN secure);
NTSTATUS BadImpersonation();
NTSTATUS DoImpersonation(PACCESS_TOKEN token);
NTSTATUS OpenKey(PHANDLE Handle, LPCWSTR path);
NTSTATUS CreateKeyRelative(LPCWSTR BasePath, PUNICODE_STRING Path);
NTSTATUS RunIoTest();

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, Device_DeviceIoControlFunction)
#pragma alloc_text (PAGE, Device_FileSystemControlFunction)
#pragma alloc_text (PAGE, HandleIoControl)
#pragma alloc_text (PAGE, CreateFile)
#pragma alloc_text (PAGE, CreateFileIo)
#pragma alloc_text (PAGE, ImpersonationThread)
#pragma alloc_text (PAGE, CreateKey)
#pragma alloc_text (PAGE, IsCallerElevated)
#pragma alloc_text (PAGE, BadImpersonation)
#pragma alloc_text (PAGE, DoImpersonation)
#pragma alloc_text (PAGE, OpenKey)
#pragma alloc_text (PAGE, CreateKeyRelative)
#pragma alloc_text (PAGE, RunIoTest)
#endif

NTSTATUS CreateFile(PUNICODE_STRING Path, BOOLEAN Secure, BOOLEAN Directory)
{
  OBJECT_ATTRIBUTES obj_attr = { 0 };
  HANDLE Handle = NULL;
  IO_STATUS_BLOCK io_status = { 0 };
  NTSTATUS status = STATUS_SUCCESS;
  ULONG CreateOptions = Directory ? FILE_DIRECTORY_FILE : FILE_NON_DIRECTORY_FILE;
  ULONG AttributeFlags = OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE;
  if (Secure)
  {
    AttributeFlags |= OBJ_FORCE_ACCESS_CHECK;
  }

  InitializeObjectAttributes(&obj_attr, Path, AttributeFlags, NULL, NULL);
  
  CHECK_STATUS(ZwCreateFile(&Handle, MAXIMUM_ALLOWED, &obj_attr, &io_status,
    NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_DELETE,
    FILE_OPEN_IF, CreateOptions, NULL, 0));
  
error:
  if (Handle)
  {
    ZwClose(Handle);
  }
  return status;
}

NTSTATUS CreateFileIo(PUNICODE_STRING Path, BOOLEAN Secure, BOOLEAN ForceAccessCheck, BOOLEAN Directory)
{
	OBJECT_ATTRIBUTES obj_attr = { 0 };
	HANDLE Handle = NULL;
	IO_STATUS_BLOCK io_status = { 0 };
	NTSTATUS status = STATUS_SUCCESS;
	ULONG CreateOptions = Directory ? FILE_DIRECTORY_FILE : FILE_NON_DIRECTORY_FILE;
	ULONG AttributeFlags = OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE;
	if (Secure)
	{
		AttributeFlags |= OBJ_FORCE_ACCESS_CHECK;
	}

	InitializeObjectAttributes(&obj_attr, Path, AttributeFlags, NULL, NULL);

  DBGPRINT("IO Test: Path: %wZ - Secure: %s - ForceAccessCheck: %s\n",
    Path, BtoS(Secure), BtoS(ForceAccessCheck));
	CHECK_STATUS(IoCreateFileEx(&Handle, MAXIMUM_ALLOWED, &obj_attr, &io_status,
		NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_DELETE,
		FILE_OPEN_IF, CreateOptions, NULL, 0, CreateFileTypeNone, 
		NULL, IO_NO_PARAMETER_CHECKING | (ForceAccessCheck ? IO_FORCE_ACCESS_CHECK : 0), NULL));

error:
	if (Handle)
	{
		ZwClose(Handle);
	}
	return status;
}

NTSTATUS CreateKey(PUNICODE_STRING Path, HANDLE Root, BOOLEAN Secure)
{
  OBJECT_ATTRIBUTES obj_attr = { 0 };
  HANDLE Handle = NULL;
  NTSTATUS status = STATUS_SUCCESS;
  ULONG Disposition = 0;
  ULONG AttributeFlags = OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE;
  if (Secure)
  {
    AttributeFlags |= OBJ_FORCE_ACCESS_CHECK;
  }

  InitializeObjectAttributes(&obj_attr, Path, AttributeFlags, Root, NULL);

  CHECK_STATUS(ZwCreateKey(&Handle, MAXIMUM_ALLOWED, &obj_attr, 
    0, NULL, REG_OPTION_NON_VOLATILE, &Disposition));

error:
  if (Handle)
  {
    ZwClose(Handle);
  }
  return status;
}

NTSTATUS OpenKey(PHANDLE Handle, LPCWSTR path)
{
  OBJECT_ATTRIBUTES obj_attr = { 0 };
  UNICODE_STRING Path;
  ULONG AttributeFlags = OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE;  
  RtlInitUnicodeString(&Path, path);

  InitializeObjectAttributes(&obj_attr, &Path, AttributeFlags, NULL, NULL);

  return ZwOpenKey(Handle, KEY_CREATE_SUB_KEY, &obj_attr);
}

NTSTATUS IsCallerElevated(BOOLEAN secure)
{
  SECURITY_SUBJECT_CONTEXT subject_context = { 0 };
  NTSTATUS status = STATUS_ACCESS_DENIED;
  SeCaptureSubjectContext(&subject_context);
  PACCESS_TOKEN token = SeQuerySubjectContextToken(&subject_context);

  DBGPRINT("ImpersonationLevel: %ls\r\n", subject_context.ClientToken 
    ? GetTokenImpersonationLevel(subject_context.ClientToken) : L"None");

  if (secure)
  {
    if (subject_context.ClientToken && subject_context.ImpersonationLevel < SecurityImpersonation)
    {
      status = STATUS_BAD_IMPERSONATION_LEVEL;
      goto error;
    }
  }
  
  status = GetTokenElevated(token) ? STATUS_SUCCESS : STATUS_ACCESS_DENIED;
error:
  SeReleaseSubjectContext(&subject_context);
  return status;
}

struct ImpersonationData
{
  PACCESS_TOKEN Token;
  NTSTATUS Status;
};

NTSTATUS DoImpersonation(PACCESS_TOKEN token)
{
  NTSTATUS status = STATUS_ACCESS_DENIED;
  UNICODE_STRING file;

  CHECK_STATUS(PsImpersonateClient(KeGetCurrentThread(), token, FALSE, FALSE, SecurityImpersonation));
  DBGPRINT("Impersonated token\n");
  RtlInitUnicodeString(&file, L"\\SystemRoot\\demo.txt");
  CHECK_STATUS(CreateFile(&file, TRUE, FALSE));
  DBGPRINT("Create file\n");
error:
  PsRevertToSelf();
  return status;
}

VOID
ImpersonationThread(
  PVOID  StartContext
)
{
  struct ImpersonationData* imp_data = StartContext;
  imp_data->Status = DoImpersonation(imp_data->Token);
}

NTSTATUS BadImpersonation()
{
  SECURITY_SUBJECT_CONTEXT subject_context = { 0 };
  struct ImpersonationData* imp_data = NULL;
  NTSTATUS status = STATUS_ACCESS_DENIED;
  SeCaptureSubjectContext(&subject_context);
  imp_data = ExAllocatePoolWithTag(PagedPool, sizeof(struct ImpersonationData), DRIVER_POOL_TAG);
  memset(imp_data, 0, sizeof(struct ImpersonationData));
  imp_data->Token = SeQuerySubjectContextToken(&subject_context);
  HANDLE thread = NULL;

  DBGPRINT("ImpersonationLevel: %ls\r\n", subject_context.ClientToken
    ? GetTokenImpersonationLevel(subject_context.ClientToken) : L"None");
  CHECK_STATUS(PsCreateSystemThread(&thread, THREAD_ALL_ACCESS, NULL, NULL, NULL, ImpersonationThread, imp_data));
  CHECK_STATUS(ZwWaitForSingleObject(thread, FALSE, NULL));
  
error:
  if (thread)
  {
    ZwClose(thread);
  }
  SeReleaseSubjectContext(&subject_context);

  if (imp_data)
  {
    status = imp_data->Status;
    ExFreePoolWithTag(imp_data, DRIVER_POOL_TAG);
  }

  return status;
}

NTSTATUS CreateKeyRelative(LPCWSTR BasePath, PUNICODE_STRING Path)
{
  HANDLE key = NULL;
  NTSTATUS status = STATUS_SUCCESS;

  CHECK_STATUS(OpenKey(&key, BasePath));
  CHECK_STATUS(CreateKey(Path, key, FALSE));

error:
  if (key)
  {
    ZwClose(key);
  }

  return status;
}

NTSTATUS RunIoTest()
{
  UNICODE_STRING device_name = { 0 };
  RtlInitUnicodeString(&device_name, L"\\Device\\" DEVICE_NAME);
  
  CreateFileIo(&device_name, FALSE, FALSE, FALSE);
  CreateFileIo(&device_name, TRUE, FALSE, FALSE);
  CreateFileIo(&device_name, FALSE, TRUE, FALSE);
  CreateFileIo(&device_name, TRUE, TRUE, FALSE);
  return STATUS_SUCCESS;
}

NTSTATUS HandleIoControl(PIRP Irp)
{
  IO_STACK_LOCATION* io_stack = IoGetCurrentIrpStackLocation(Irp);
  ULONG ctl_code = io_stack->Parameters.DeviceIoControl.IoControlCode;
  enum ControlCode code = (ctl_code >> 2) & 0x7FF;
  UNICODE_STRING path = { 0 };
  
  if ((ctl_code & ~(0x7FF << 2)) != IOCTL_BASE)
  {
    return STATUS_INVALID_PARAMETER;
  }

  path.Buffer = Irp->AssociatedIrp.SystemBuffer;
  path.MaximumLength = (USHORT)io_stack->Parameters.DeviceIoControl.InputBufferLength;
  path.Length = (USHORT)io_stack->Parameters.DeviceIoControl.InputBufferLength;

  switch (code)
  {
  case ControlCreateFile:
    return CreateFile(&path, FALSE, FALSE);
  case ControlCreateFileSecure:
    return CreateFile(&path, TRUE, FALSE);
  case ControlCreateDir:
    return CreateFile(&path, FALSE, TRUE);
  case ControlCreateDirSecure:
    return CreateFile(&path, TRUE, TRUE);
  case ControlCreateKey:
    return CreateKey(&path, NULL, FALSE);
  case ControlCreateKeySecure:
    return CreateKey(&path, NULL, TRUE);
  case ControlCreateKeyRelative:
    return CreateKeyRelative(L"\\Registry\\Machine\\Software", &path);
  case ControlCallerIsElevated:
    return IsCallerElevated(FALSE);
  case ControlCallerIsElevatedSecure:
    return IsCallerElevated(TRUE);
  case ControlBadImpersonation:
    return BadImpersonation();
  case ControlToggleProcessDebug:
    ToggleProcessDebug();
    return STATUS_SUCCESS;
  case ControlToggleIrpDebug:
    ToggleIrpDebugEnabled();
    return STATUS_SUCCESS;
  case ControlToggleRegistryDebug:
    ToggleRegistryDebug();
    return STATUS_SUCCESS;
  case ControlRunIoTest:
    return RunIoTest();
  }

  return STATUS_NOT_SUPPORTED;
}

NTSTATUS Device_DeviceIoControlFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
  UNREFERENCED_PARAMETER(DeviceObject);
  if (GetIrpDebugEnabled())
  {
    DBGPRINT("DeviceIoControl Called: %08X\n", IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.IoControlCode);
    PrintContext(Irp->RequestorMode);
  }
  return CompleteIrp(Irp, HandleIoControl(Irp));
}

NTSTATUS Device_FileSystemControlFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
  UNREFERENCED_PARAMETER(DeviceObject);
  if (GetIrpDebugEnabled())
  {
    DBGPRINT("FileSystemControl Called: %08X\n", IoGetCurrentIrpStackLocation(Irp)->Parameters.FileSystemControl.FsControlCode);
    PrintContext(Irp->RequestorMode);
  }
  return CompleteIrp(Irp, STATUS_NOT_SUPPORTED);
}
