#pragma once

const char* BtoS(BOOLEAN b);
PWSTR UnicodeStringToString(PCUNICODE_STRING String);
ULONG64 LuidToLong(PLUID luid);
unsigned int HandleToInt(HANDLE h);
NTSTATUS AppendInt64ToString(PUNICODE_STRING String, ULONGLONG Value);
NTSTATUS AppendPointerToString(PUNICODE_STRING String, void* pointer);
NTSTATUS AppendBooleanToString(PUNICODE_STRING String, BOOLEAN b);
NTSTATUS AllocateUnicodeString(PUNICODE_STRING String);
void FreeUnicodeString(PUNICODE_STRING String);
