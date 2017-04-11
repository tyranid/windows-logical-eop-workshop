#pragma once

void PrintContext(KPROCESSOR_MODE RequestorMode);
PCSTR ProcessModeToString(KPROCESSOR_MODE mode);
NTSTATUS InstallProcessCallback();
void UninstallProcessCallback();
VOID ToggleProcessDebug();