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

void RunRpcClient(void*);
void RunDriverTests(void*);

int _tmain(int argc, _TCHAR* argv[])
{
  void* unk = nullptr;

  std::vector<MenuOption> menu = {
    { "RPC Client Tests", RunRpcClient },
    { "Driver Tests", RunDriverTests },
  };
  RunMenu("Specify Test Category", menu, unk);

  return 0;
}