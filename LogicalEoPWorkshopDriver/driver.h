#pragma once

#define INITGUID

#include <ntifs.h>
#include <Fltkernel.h>
#include <strsafe.h>
#include <Wdmsec.h>
#include "token_utils.h"
#include "utils.h"
#include "registry_monitor.h"
#include "process_monitor.h"
#include "device_control.h"

EXTERN_C_START
DRIVER_INITIALIZE DriverEntry;
EXTERN_C_END

#define DRIVER_POOL_TAG 'WLPX'
#define DEVICE_NAME L"WorkshopDriver"
#define DEVICE_INSECURE_NAME L"WorkshopDriverInsecure"
#define DEVICE_NOTRAVERSAL_NAME L"WorkshopDriverNoTraversal"
#define DBGPRINT(x, ...) DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, __FUNCTION__ ": " x, ##__VA_ARGS__)
#define CHECK_STATUS(s) { status = (s); if (!NT_SUCCESS(status)) { goto error; } }

NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status);
BOOLEAN GetIrpDebugEnabled();
void SetIrpDebugEnabled(BOOLEAN value);
void ToggleIrpDebugEnabled();
void LockDriver();
void UnlockDriver();