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
#include "utils.h"

#include <intrin.h>

VOID Driver_Unload(PDRIVER_OBJECT DriverObject);
NTSTATUS Device_UnSupportedFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS Device_CreateFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS Device_CloseFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS Device_CleanupFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS CreateDevice(PDRIVER_OBJECT DriverObject, LPCWSTR Name, DWORD Flags, PCUNICODE_STRING Sddl);

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (INIT, CreateDevice)
#pragma alloc_text (PAGE, Device_UnSupportedFunction)
#pragma alloc_text (PAGE, Device_CreateFunction)
#pragma alloc_text (PAGE, Device_CloseFunction)
#pragma alloc_text (PAGE, Device_CleanupFunction)
#pragma alloc_text (PAGE, Device_DeviceIoControlFunction)
#pragma alloc_text (PAGE, Device_FileSystemControlFunction)
#pragma alloc_text (PAGE, Driver_Unload)
#pragma alloc_text (PAGE, GetIrpDebugEnabled)
#pragma alloc_text (PAGE, SetIrpDebugEnabled)
#pragma alloc_text (PAGE, ToggleIrpDebugEnabled)
#pragma alloc_text (PAGE, LockDriver)
#pragma alloc_text (PAGE, UnlockDriver)
#endif

static LPWSTR FunctionNames[] = {
  L"IRP_MJ_CREATE",
  L"IRP_MJ_CREATE_NAMED_PIPE",
  L"IRP_MJ_CLOSE",
  L"IRP_MJ_READ",
  L"IRP_MJ_WRITE",
  L"IRP_MJ_QUERY_INFORMATION",
  L"IRP_MJ_SET_INFORMATION",
  L"IRP_MJ_QUERY_EA",
  L"IRP_MJ_SET_EA",
  L"IRP_MJ_FLUSH_BUFFERS",
  L"IRP_MJ_QUERY_VOLUME_INFORMATION",
  L"IRP_MJ_SET_VOLUME_INFORMATION",
  L"IRP_MJ_DIRECTORY_CONTROL",
  L"IRP_MJ_FILE_SYSTEM_CONTROL",
  L"IRP_MJ_DEVICE_CONTROL",
  L"IRP_MJ_INTERNAL_DEVICE_CONTROL",
  L"IRP_MJ_SHUTDOWN",
  L"IRP_MJ_LOCK_CONTROL",
  L"IRP_MJ_CLEANUP",
  L"IRP_MJ_CREATE_MAILSLOT",
  L"IRP_MJ_QUERY_SECURITY",
  L"IRP_MJ_SET_SECURITY",
  L"IRP_MJ_POWER",
  L"IRP_MJ_SYSTEM_CONTROL",
  L"IRP_MJ_DEVICE_CHANGE",
  L"IRP_MJ_QUERY_QUOTA",
  L"IRP_MJ_SET_QUOTA",
  L"IRP_MJ_PNP",
};

static BOOLEAN g_installed_process_monitor = FALSE;
static BOOLEAN g_irp_debug_enabled = FALSE;
static PFAST_MUTEX g_driver_lock = NULL;

BOOLEAN GetIrpDebugEnabled()
{
  return g_irp_debug_enabled;
}

void SetIrpDebugEnabled(BOOLEAN value)
{
  g_irp_debug_enabled = value;
}

void ToggleIrpDebugEnabled()
{
  g_irp_debug_enabled = !g_irp_debug_enabled;
  DBGPRINT("IRP Debug is %s\r\n", g_irp_debug_enabled ? "ENABLED" : "DISABLED");
}

void LockDriver()
{
  ExAcquireFastMutex(g_driver_lock);
}

void UnlockDriver()
{
  ExReleaseFastMutex(g_driver_lock);
}

NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status)
{
  Irp->IoStatus.Information = 0;
  Irp->IoStatus.Status = status;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  return status;
}

NTSTATUS Device_UnSupportedFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
  UNREFERENCED_PARAMETER(DeviceObject);
  if (Irp->RequestorMode == KernelMode)
  {
    if (GetIrpDebugEnabled())
    {
      PIO_STACK_LOCATION stack_loc = IoGetCurrentIrpStackLocation(Irp);
      if (stack_loc->MajorFunction < (sizeof(FunctionNames) / sizeof(FunctionNames[0])))
        DBGPRINT("Device_UnSupportedFunction Called %ls\r\n", FunctionNames[stack_loc->MajorFunction]);
      else
        DBGPRINT("Device_UnSupportedFunction Called %d\r\n", stack_loc->MajorFunction);
      PrintContext(Irp->RequestorMode);
    }
  }
  return CompleteIrp(Irp, STATUS_NOT_SUPPORTED);
}

void DeleteSymbolicLink(LPCWSTR Name)
{
  UNICODE_STRING usDosDeviceName = { 0 };
  NTSTATUS status = STATUS_SUCCESS;

  CHECK_STATUS(AllocateUnicodeString(&usDosDeviceName));
  CHECK_STATUS(RtlAppendUnicodeToString(&usDosDeviceName, L"\\DosDevices\\"));
  CHECK_STATUS(RtlAppendUnicodeToString(&usDosDeviceName, Name));
  CHECK_STATUS(IoDeleteSymbolicLink(&usDosDeviceName));
error:
  FreeUnicodeString(&usDosDeviceName);
}

VOID Driver_Unload(PDRIVER_OBJECT DriverObject)
{
  PDEVICE_OBJECT pDevice = DriverObject->DeviceObject;
  DBGPRINT("Device_Unload Called \r\n");
  DeleteSymbolicLink(DEVICE_NAME);
  DeleteSymbolicLink(DEVICE_INSECURE_NAME);
  DeleteSymbolicLink(DEVICE_NOTRAVERSAL_NAME);
  while(pDevice)
  {
    PDEVICE_OBJECT pNext = pDevice->NextDevice;
    DBGPRINT("Deleting Device %p\n", pDevice);
    IoDeleteDevice(pDevice);
    pDevice = pNext;
  }

  UninstallRegistryCallback();
  if (g_installed_process_monitor)
  {
    UninstallProcessCallback();
  }
}

NTSTATUS CreateDevice(PDRIVER_OBJECT DriverObject, LPCWSTR Name, DWORD Flags, PCUNICODE_STRING Sddl)
{
  UNICODE_STRING DeviceName = { 0 };
  UNICODE_STRING DosDeviceName = { 0 };
  PDEVICE_OBJECT DeviceObject = NULL;
  NTSTATUS status = STATUS_SUCCESS;

  CHECK_STATUS(AllocateUnicodeString(&DeviceName));
  CHECK_STATUS(RtlAppendUnicodeToString(&DeviceName, L"\\Device\\"));
  CHECK_STATUS(RtlAppendUnicodeToString(&DeviceName, Name));
  CHECK_STATUS(AllocateUnicodeString(&DosDeviceName));
  CHECK_STATUS(RtlAppendUnicodeToString(&DosDeviceName, L"\\DosDevices\\"));
  CHECK_STATUS(RtlAppendUnicodeToString(&DosDeviceName, Name));

  if (Sddl)
  {
    CHECK_STATUS(IoCreateDeviceSecure(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN,
      Flags, FALSE, Sddl, NULL, &DeviceObject));
  }
  else
  {
    CHECK_STATUS(IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN,
      Flags, FALSE, &DeviceObject));
  }

  CHECK_STATUS(IoCreateSymbolicLink(&DosDeviceName, &DeviceName));
  DeviceObject->Flags |= DO_BUFFERED_IO;
  
error:

  if (!NT_SUCCESS(status)) {
    DBGPRINT("Error creating device %ls: %08X\n", Name, status);
  }
  else {
    DBGPRINT("Created device %ls - %p\r\n", Name, DeviceObject);
  }

  if (DeviceObject && !NT_SUCCESS(status))
  {
    IoDeleteDevice(DeviceObject);
  }

  FreeUnicodeString(&DeviceName);
  FreeUnicodeString(&DosDeviceName);

  return status;
}

NTSTATUS
DriverEntry(
  _In_ PDRIVER_OBJECT  DriverObject,
  _In_ PUNICODE_STRING RegistryPath
)
{
  UNREFERENCED_PARAMETER(RegistryPath);
  UNREFERENCED_PARAMETER(DriverObject);
  DBGPRINT("In DriverEntry\n");
  NTSTATUS status = STATUS_SUCCESS;
  g_driver_lock = ExAllocatePoolWithTag(NonPagedPool, sizeof(FAST_MUTEX), DRIVER_POOL_TAG);
  ExInitializeFastMutex(g_driver_lock);

  //__debugbreak();

  status = InstallRegistryCallback();
  if (!NT_SUCCESS(status)) {
    DBGPRINT("Error setting Registry callback: %08X\n", status);
    return status;
  }

  status = InstallProcessCallback();
  if (!NT_SUCCESS(status)) {
    DBGPRINT("Error setting Process callback: %08X\n", status);    
  }
  else {
    g_installed_process_monitor = TRUE;
  }
  
  int iIndex = 0;
  DBGPRINT("Driver Started\r\n");

  CHECK_STATUS(CreateDevice(DriverObject, DEVICE_NAME, FILE_DEVICE_SECURE_OPEN | FILE_DEVICE_ALLOW_APPCONTAINER_TRAVERSAL, NULL));
  CHECK_STATUS(CreateDevice(DriverObject, DEVICE_INSECURE_NAME, FILE_DEVICE_ALLOW_APPCONTAINER_TRAVERSAL, &SDDL_DEVOBJ_SYS_ALL_ADM_ALL));
  CHECK_STATUS(CreateDevice(DriverObject, DEVICE_NOTRAVERSAL_NAME, 0, NULL));

  for (iIndex = 0; iIndex < IRP_MJ_MAXIMUM_FUNCTION; iIndex++)
    DriverObject->MajorFunction[iIndex] = Device_UnSupportedFunction;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = Device_CreateFunction;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = Device_CloseFunction;
  DriverObject->MajorFunction[IRP_MJ_CLEANUP] = Device_CleanupFunction;
  DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = Device_FileSystemControlFunction;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Device_DeviceIoControlFunction;
  DriverObject->DriverUnload = Driver_Unload;

error:
  if (!NT_SUCCESS(status))
  {
    Driver_Unload(DriverObject);
  }

  return status;
}

NTSTATUS Device_CreateFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
  UNREFERENCED_PARAMETER(DeviceObject);
  IO_STACK_LOCATION* stack_loc = IoGetCurrentIrpStackLocation(Irp);

  DBGPRINT("Create Called \r\n");
  PrintContext(Irp->RequestorMode);
  DBGPRINT("Remaining Name: %wZ\r\n", &stack_loc->FileObject->FileName);
  DBGPRINT("EA Length: %d\n", stack_loc->Parameters.Create.EaLength);
  DBGPRINT("IRP Flags: %08X\n", stack_loc->Flags);
  
  return CompleteIrp(Irp, STATUS_SUCCESS);
}

NTSTATUS Device_CloseFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
  UNREFERENCED_PARAMETER(DeviceObject);
  if (GetIrpDebugEnabled())
  {
    DBGPRINT("Close Called \r\n");
    PrintContext(Irp->RequestorMode);
  }
  return CompleteIrp(Irp, STATUS_SUCCESS);
}

NTSTATUS Device_CleanupFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
  UNREFERENCED_PARAMETER(DeviceObject);
  if (GetIrpDebugEnabled())
  {
    DBGPRINT("Cleanup Called \r\n");
    PrintContext(Irp->RequestorMode);
  }
  return CompleteIrp(Irp, STATUS_SUCCESS);
}
