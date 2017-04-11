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

using System;
using System.Runtime.InteropServices;

namespace COMServer
{
    [ComVisible(true), Guid("801445A7-C5A9-468D-9423-81F9D13FEE9B")]
    public class COMService : ICOMInterface
    {
        const int STGM_CREATE = 0x00001000;
        const int STGM_READWRITE = 0x00000002;
        const int STGM_EXCLUSIVE = 0x00000010;
        const int STGFMT_STORAGE = 0;

        [DllImport("ole32.dll", CharSet = CharSet.Unicode, PreserveSig = false)]
        static extern void StgCreateStorageEx(
            string pwcsName,
            int grfMode,
            int stgfmt,
            int grfAttrs,
            IntPtr pStgOptions,
            IntPtr pSecurityDescriptor,
            ref Guid riid,
            [MarshalAs(UnmanagedType.IUnknown)] out object ppObjectOpen
        );

        public object GetStorage()
        {
            Console.WriteLine("Calling GetStorage");
            Guid IID_IStorage = typeof(IStorage).GUID;
            object ret;
            StgCreateStorageEx("abc.stg", STGM_CREATE | STGM_READWRITE | STGM_EXCLUSIVE, 0, 0, IntPtr.Zero, IntPtr.Zero, ref IID_IStorage, out ret);
            return ret;
        }
    }
}
