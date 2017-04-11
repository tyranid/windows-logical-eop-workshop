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

SECURITY_IMPERSONATION_LEVEL SeTokenImpersonationLevel(PACCESS_TOKEN Token);

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, CompareTokens)
#pragma alloc_text (PAGE, GetTokenUserSidString)
#pragma alloc_text (PAGE, GetTokenElevated)
#pragma alloc_text (PAGE, GetTokenAppContainer)
#pragma alloc_text (PAGE, GetTokenIntegrityLevel)
#pragma alloc_text (PAGE, GetTokenType)
#pragma alloc_text (PAGE, GetTokenImpersonationLevel)
#pragma alloc_text (PAGE, SerializeToken)
#pragma alloc_text (PAGE, SerializeEffectiveToken)
#pragma alloc_text (PAGE, SerializeEffectiveTokenFromThread)
#pragma alloc_text (PAGE, TokenImpersonationLevelToString)
#endif

PWSTR SerializeToken(PACCESS_TOKEN Token)
{
  // Serialize the token information into a CSV string of form:
  // SID,AuthID,IL,Elevated,AppContainer,Restricted,WriteRestricted,SessionId,TokenType,TokenImpersonationLevel
  UNICODE_STRING us = { 0 };
  UNICODE_STRING sid = { 0 };
  LUID AuthId = { 0 };
  ULONG SessionId = 0;
  PWSTR ret = NULL;
  NTSTATUS status = STATUS_SUCCESS;

  us.Buffer = ExAllocatePoolWithTag(PagedPool, 4096, DRIVER_POOL_TAG);
  if (!us.Buffer)
  {
    goto error;
  }
  us.MaximumLength = 4096;

  CHECK_STATUS(GetTokenUserSidString(Token, &sid));
  CHECK_STATUS(RtlAppendUnicodeStringToString(&us, &sid));
  CHECK_STATUS(RtlAppendUnicodeToString(&us, L","));
  CHECK_STATUS(SeQueryAuthenticationIdToken(Token, &AuthId));
  CHECK_STATUS(AppendInt64ToString(&us, LuidToLong(&AuthId)));
  CHECK_STATUS(RtlAppendUnicodeToString(&us, L","));
  CHECK_STATUS(AppendInt64ToString(&us, GetTokenIntegrityLevel(Token)));
  CHECK_STATUS(RtlAppendUnicodeToString(&us, L","));
  CHECK_STATUS(AppendBooleanToString(&us, GetTokenElevated(Token)));
  CHECK_STATUS(RtlAppendUnicodeToString(&us, L","));
  CHECK_STATUS(AppendBooleanToString(&us, GetTokenAppContainer(Token)));
  CHECK_STATUS(RtlAppendUnicodeToString(&us, L","));
  CHECK_STATUS(AppendBooleanToString(&us, SeTokenIsRestricted(Token)));
  CHECK_STATUS(RtlAppendUnicodeToString(&us, L","));
  CHECK_STATUS(AppendBooleanToString(&us, SeTokenIsWriteRestricted(Token)));
  CHECK_STATUS(RtlAppendUnicodeToString(&us, L","));
  CHECK_STATUS(SeQuerySessionIdToken(Token, &SessionId));
  CHECK_STATUS(AppendInt64ToString(&us, SessionId));

  ret = UnicodeStringToString(&us);

error:

  if (sid.Buffer)
  {
    RtlFreeUnicodeString(&sid);
  }

  return ret;
}

PWSTR SerializeEffectiveTokenFromThread(PETHREAD Thread)
{
  PWSTR result = NULL;
  BOOLEAN Impersonating = TRUE;
  BOOLEAN CopyOnOpen = FALSE;
  BOOLEAN EffectiveOnly = FALSE;
  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel = SecurityAnonymous;
  PACCESS_TOKEN Token = PsReferenceImpersonationToken(PsGetCurrentThread(),
    &CopyOnOpen, &EffectiveOnly, &ImpersonationLevel);
  if (!Token)
  {
    Impersonating = FALSE;
    Token = PsReferencePrimaryToken(IoThreadToProcess(Thread));
  }

  result = SerializeToken(Token);
  if (Impersonating)
  {
    PsDereferenceImpersonationToken(Token);
  }
  else
  {
    PsDereferencePrimaryToken(Token);
  }

  return result;
}

PWSTR SerializeEffectiveToken()
{
  return SerializeEffectiveTokenFromThread(PsGetCurrentThread());
}

BOOLEAN IsUserToken(PWSTR Token)
{
  UNICODE_STRING TokenString;
  UNICODE_STRING UserPrefix;
  RtlInitUnicodeString(&TokenString, Token);
  RtlInitUnicodeString(&UserPrefix, L"S-1-5-21");
  return RtlPrefixUnicodeString(&UserPrefix, &TokenString, TRUE);
}

NTSTATUS GetTokenUserSidString(PACCESS_TOKEN Token, PUNICODE_STRING UserSid)
{
  PTOKEN_USER User = NULL;
  NTSTATUS status = SeQueryInformationToken(Token, TokenUser, &User);
  if (!NT_SUCCESS(status))
    return status;
  status = RtlConvertSidToUnicodeString(UserSid, User->User.Sid, TRUE);
  if (User)
  {
    ExFreePool(User);
  }
  return status;
}

BOOLEAN GetTokenElevated(PACCESS_TOKEN Token)
{
  PTOKEN_ELEVATION Elevation = NULL;
  BOOLEAN ret = FALSE;
  if (NT_SUCCESS(SeQueryInformationToken(Token, TokenElevation, &Elevation)))
  {
    ret = !!Elevation->TokenIsElevated;
    ExFreePool(Elevation);
  }
  return ret;
}

BOOLEAN GetTokenAppContainer(PACCESS_TOKEN Token)
{
  ULONG ac = 0;
  BOOLEAN ret = FALSE;
  if (NT_SUCCESS(SeQueryInformationToken(Token, TokenIsAppContainer, (PVOID*)&ac)))
  {
    ret = !!ac;
  }
  return ret;
}

ULONG GetTokenIntegrityLevel(PACCESS_TOKEN Token)
{
  ULONG il = 0;
  SeQueryInformationToken(Token, TokenIntegrityLevel, (PVOID*)&il);
  return il;
}

PCWSTR GetTokenType(PACCESS_TOKEN Token)
{
  TOKEN_TYPE type = SeTokenType(Token);
  return type == TokenPrimary ? L"Primary" : L"Impersonation";
}

PCWSTR TokenImpersonationLevelToString(SECURITY_IMPERSONATION_LEVEL Level)
{
  switch (Level)
  {
  case SecurityAnonymous:
    return L"Anonymous";
  case SecurityIdentification:
    return L"Identification";
  case SecurityImpersonation:
    return L"Impersonation";
  case SecurityDelegation:
    return L"Delegation";
  default:
    return L"None";
  }
}

PCWSTR GetTokenImpersonationLevel(PACCESS_TOKEN Token)
{
  return TokenImpersonationLevelToString(SeTokenImpersonationLevel(Token));
}

BOOLEAN CompareTokens(PACCESS_TOKEN CreatorToken, PACCESS_TOKEN ProcessToken)
{
  LUID CreatorAuthId, ProcessAuthId;
  PTOKEN_USER CreatorUser = NULL;
  PTOKEN_USER ProcessUser = NULL;
  ULONG CreatorSessionId, ProcessSessionId;
  BOOLEAN Result = FALSE;
  NTSTATUS status = STATUS_SUCCESS;

  CHECK_STATUS(SeQueryInformationToken(CreatorToken, TokenUser, &CreatorUser));
  CHECK_STATUS(SeQueryInformationToken(ProcessToken, TokenUser, &ProcessUser));
  if (!RtlEqualSid(CreatorUser->User.Sid, ProcessUser->User.Sid))
  {
    goto error;
  }

  CHECK_STATUS(SeQueryAuthenticationIdToken(CreatorToken, &CreatorAuthId));
  CHECK_STATUS(SeQueryAuthenticationIdToken(ProcessToken, &ProcessAuthId));
  if (!RtlEqualLuid(&CreatorAuthId, &ProcessAuthId))
  {
    goto error;
  }

  if (GetTokenElevated(CreatorToken) != GetTokenElevated(ProcessToken))
  {
    goto error;
  }

  if (SeTokenIsRestricted(CreatorToken) != SeTokenIsRestricted(ProcessToken))
  {
    goto error;
  }

  if (SeTokenIsWriteRestricted(CreatorToken) != SeTokenIsWriteRestricted(ProcessToken))
  {
    goto error;
  }

  if (GetTokenAppContainer(CreatorToken) != GetTokenAppContainer(ProcessToken))
  {
    goto error;
  }

  if (GetTokenIntegrityLevel(CreatorToken) != GetTokenIntegrityLevel(ProcessToken))
  {
    goto error;
  }

  CHECK_STATUS(SeQuerySessionIdToken(CreatorToken, &CreatorSessionId));
  CHECK_STATUS(SeQuerySessionIdToken(ProcessToken, &ProcessSessionId));
  if (CreatorSessionId != ProcessSessionId)
  {
    goto error;
  }

  Result = TRUE;

error:

  if (CreatorUser)
  {
    ExFreePool(CreatorUser);
  }
  if (ProcessUser)
  {
    ExFreePool(ProcessUser);
  }

  return Result;
}
