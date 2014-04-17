using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Forms;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace transm.gui3
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public string SimulationFolder = "";

        public MainWindow()
        {
            InitializeComponent();
        }

        private void OpenDirectory(object sender, MouseButtonEventArgs e)
        {
            FolderBrowserDialog profilePath = new FolderBrowserDialog();

            DialogResult result = profilePath.ShowDialog();
            if (result == System.Windows.Forms.DialogResult.OK)
            {
                SimulationFolder = profilePath.SelectedPath;
            }
        }

        private void RunSimulation(object sender, MouseButtonEventArgs e)
        {
            var allFiles = Directory.EnumerateFiles(SimulationFolder).ToList();
            var allXmls = allFiles.Where((a,b) => System.IO.Path.GetExtension(a) == ".xml").ToList();
            Directory.SetCurrentDirectory(SimulationFolder);

            foreach(var xml in allXmls)
            {
                var xmlFile = System.IO.Path.Combine(SimulationFolder, xml);
                using(SimManaged m = new SimManaged(xmlFile))
                {
                    m.Initialize();
                    foreach (var message in m.Messages)
                    {
                        SimOutput.AppendText(message);
                    }

                    Thread t = new Thread(() =>
                    {
                        while (m.Step())
                        {
                            foreach (var message in m.Messages)
                            {
                                SimOutput.AppendText(message);
                            }
                        }
                    });
                    t.Start();
                }
            }
        }
    }
}
