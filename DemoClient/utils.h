#pragma once

std::wstring GetArgument(const char* prompt);
int GetArgumentNumeric(const char* prompt);
std::wstring GetErrorMessage(int error);
void PrintError(const char* error_title, int error);

struct MenuOption
{
  const char* title;
  void(*RunOp)(void* context);
};

void RunMenu(const char* name, const std::vector<MenuOption>& menu, void* context);