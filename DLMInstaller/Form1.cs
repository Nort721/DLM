using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;

namespace DLMInstaller
{
    public partial class Form1 : Form
    {
        const string dllMonitorPath = "DriverLoadMonitor.dll";

        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {

        }

        private void monitorToolStripMenuItem_Click(object sender, EventArgs e)
        {

        }

        static void SetDllToAppInit(string dllPath)
        {
            // Choose the registry path based on the bitness of the process
            string registryPath = Environment.Is64BitProcess ?
                @"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Windows" :
                @"HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Windows NT\CurrentVersion\Windows";

            const string registryValueName = "AppInit_DLLs";

            // Get the current value of the AppInit_DLLs registry key
            string currentValue = (string)Registry.GetValue(registryPath, registryValueName, "");

            // Check if the DLL path is already in the registry
            if (!currentValue.Contains(dllPath))
            {
                // Append your DLL path to the existing value
                string newValue = currentValue + ";" + dllPath;

                // Set the modified value back to the registry
                Registry.SetValue(registryPath, registryValueName, newValue, RegistryValueKind.String);
            }
        }

        static void RestartComputer()
        {
            // Create a new process start info
            ProcessStartInfo psi = new ProcessStartInfo
            {
                FileName = "shutdown",
                Arguments = "/r /t 0",  // /r for restart, /t for time delay (0 seconds in this case)
                CreateNoWindow = true,
                UseShellExecute = false
            };

            // Start the process
            Process.Start(psi);
        }

        static string CopyDllToAppDataRoaming(string dllPath)
        {
            try
            {
                // Get the Roaming folder path
                string roamingFolderPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "DLM");

                // Get the filename of the DLL
                string dllFileName = Path.GetFileName(dllPath);

                // Combine the destination path with the DLL filename
                string destinationPath = Path.Combine(roamingFolderPath, dllFileName);

                // Check if the folder already exists with the DLL inside
                if (File.Exists(destinationPath))
                {
                    Console.WriteLine($"DLL already exists at {destinationPath}. No need to copy.");
                    return destinationPath;
                }

                // Create the DLM folder if it doesn't exist
                if (!Directory.Exists(roamingFolderPath))
                {
                    Directory.CreateDirectory(roamingFolderPath);
                }

                // Copy the DLL to the destination folder
                File.Copy(dllPath, destinationPath);

                return destinationPath;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error: {ex.Message}");
                return null;
            }
        }

        private void installBtn_Click(object sender, EventArgs e)
        {
            // Make sure the dll is in the same folder as the installer
            if (!File.Exists(dllMonitorPath))
            {
                MessageBox.Show("Can't find " + dllMonitorPath, "Missing file", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            // Make a copy of the dll in AppData
            string copiedDllPath = CopyDllToAppDataRoaming(dllMonitorPath);

            SetDllToAppInit(copiedDllPath);
            DialogResult result = MessageBox.Show("DLM has been installed.\nTo Start monitoring you need to reboot.\nDo you want to reboot now?", "Installation successful", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);
            if (result == DialogResult.Yes)
            {
                RestartComputer();
            }
        }

        private void uninstallBtn_Click(object sender, EventArgs e)
        {

        }
    }
}
