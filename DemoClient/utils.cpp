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

#include "stdafx.h"

using namespace std;

std::wstring GetArgument(const char* prompt)
{
  cout << prompt << ": ";
  std::wstring ret;
  getline(wcin, ret);
  return ret;
}

int GetArgumentNumeric(const char* prompt)
{
  while (true)
  {
    std::wstring str = GetArgument(prompt);
    wchar_t* ep = nullptr;
    int ret = wcstol(str.c_str(), &ep, 0);
    if (ret != 0 || *ep == 0 || isspace(*ep))
    {
      return ret;
    }
    wcout << "Invalid numeric argument: " << str << endl;
  }
}

std::wstring GetErrorMessage(int error)
{
  WCHAR buf[1024];
  if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error, 0, buf, _countof(buf), nullptr) > 0)
  {
    return buf;
  }
  else
  {
    wstringstream ss;
    ss << "Unknown Error: " << error;
    return ss.str();
  }
}

void PrintError(const char* error_title, int error)
{
  wcout << error_title << ": " << GetErrorMessage(error) << endl;
}

void RunMenu(const char* name, const std::vector<MenuOption>& menu, void* context)
{
  int run_op = -1;
  
  while (run_op != 0)
  {
    cout << "[" << name << "]" << endl;
    for (size_t i = 0; i < menu.size(); ++i)
    {
      cout << (i + 1) << " - " << menu[i].title << endl;
    }
    cout << "0 - " << "Exit Menu" << endl;

    run_op = GetArgumentNumeric("Specify operation number");
    cout << endl;
    if (run_op > 0 && run_op <= static_cast<int>(menu.size()))
    {
      if (menu[run_op - 1].RunOp)
      {
        menu[run_op - 1].RunOp(context);
      }
    }
  }
}
