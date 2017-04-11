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

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, BtoS)
#pragma alloc_text (PAGE, UnicodeStringToString)
#pragma alloc_text (PAGE, LuidToLong)
#pragma alloc_text (PAGE, HandleToInt)
#pragma alloc_text (PAGE, AppendInt64ToString)
#pragma alloc_text (PAGE, AppendPointerToString)
#pragma alloc_text (PAGE, AppendBooleanToString)
#pragma alloc_text (PAGE, AllocateUnicodeString)
#endif

const char* BtoS(BOOLEAN b)
{
  return b ? "True" : "False";
}

PWSTR UnicodeStringToString(PCUNICODE_STRING String)
{
  PWSTR ret = ExAllocatePoolWithTag(PagedPool, String->Length + sizeof(WCHAR), DRIVER_POOL_TAG);
  if (ret)
  {
    memcpy(ret, String->Buffer, String->Length);
    ret[String->Length / sizeof(WCHAR)] = 0;
  }
  return ret;
}

ULONG64 LuidToLong(PLUID luid)
{
  ULONG64 ret = 0;
  memcpy(&ret, luid, sizeof(ret));
  return ret;
}

unsigned int HandleToInt(HANDLE h)
{
  return (unsigned int)(ULONG_PTR)h;
}

NTSTATUS AppendInt64ToString(PUNICODE_STRING String, ULONGLONG Value)
{
  WCHAR Buffer[16];
  UNICODE_STRING us = { 0 };
  NTSTATUS status;
  us.MaximumLength = sizeof(Buffer);
  us.Length = 0;
  us.Buffer = Buffer;

  status = RtlInt64ToUnicodeString(Value, 16, &us);
  if (!NT_SUCCESS(status))
    return status;

  return RtlAppendUnicodeStringToString(String, &us);
}

NTSTATUS AppendPointerToString(PUNICODE_STRING String, void* pointer)
{
  return AppendInt64ToString(String, (ULONG_PTR)pointer);
}

NTSTATUS AppendBooleanToString(PUNICODE_STRING String, BOOLEAN b)
{
  return RtlAppendUnicodeToString(String, b ? L"True" : L"False");
}

NTSTATUS AllocateUnicodeString(PUNICODE_STRING String)
{
  USHORT MaximumLength = MAXUSHORT - 1;
  String->Buffer = ExAllocatePoolWithTag(PagedPool, MaximumLength, DRIVER_POOL_TAG);
  if (String->Buffer == NULL)
  {
    return STATUS_INSUFFICIENT_RESOURCES;
  }
  String->Length = 0;
  String->MaximumLength = MaximumLength;
  return STATUS_SUCCESS;
}

void FreeUnicodeString(PUNICODE_STRING String)
{
  if (String->Buffer)
  {
    ExFreePoolWithTag(String->Buffer, DRIVER_POOL_TAG);
    String->Buffer = NULL;
  }
}