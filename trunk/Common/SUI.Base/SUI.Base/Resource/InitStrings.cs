using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using SUI.Base.Utility;

namespace SUI.Base.Resource
{
    public class InitStrings
    {
        public static string SUI_HOME = Environment.GetEnvironmentVariable("SUI_HOME", EnvironmentVariableTarget.User);
        public static string LogFile = Path.Combine(SUI_HOME, "Log\\SUIBaseLog.log");

        // Hard code here for now!
        public static string WinFormViewerResource = @"C:\Program Files\Common Files\Business Objects\2.7\Managed\xx\CrystalDecisions.Windows.Forms.Resource.dll";

        public static string WOW6432 = SUIUtil.IsX64 ? @"\Wow6432Node" : "";
    }
}
