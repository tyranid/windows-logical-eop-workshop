#pragma once

NTSTATUS Device_DeviceIoControlFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS Device_FileSystemControlFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp);
