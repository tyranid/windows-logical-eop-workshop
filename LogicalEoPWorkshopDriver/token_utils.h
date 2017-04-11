#pragma once

PWSTR SerializeToken(PACCESS_TOKEN Token);
PWSTR SerializeEffectiveTokenFromThread(PETHREAD Thread);
PWSTR SerializeEffectiveToken();
BOOLEAN IsUserToken(PWSTR Token);
NTSTATUS GetTokenUserSidString(PACCESS_TOKEN Token, PUNICODE_STRING UserSid);
BOOLEAN GetTokenElevated(PACCESS_TOKEN Token);
BOOLEAN GetTokenAppContainer(PACCESS_TOKEN Token);
ULONG GetTokenIntegrityLevel(PACCESS_TOKEN Token);
PCWSTR GetTokenType(PACCESS_TOKEN Token);
PCWSTR TokenImpersonationLevelToString(SECURITY_IMPERSONATION_LEVEL Level);
PCWSTR GetTokenImpersonationLevel(PACCESS_TOKEN Token);
BOOLEAN CompareTokens(PACCESS_TOKEN CreatorToken, PACCESS_TOKEN ProcessToken);