using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;


namespace Analyzer {
    class Program {
        static void Main(string[] args) {
            if (args.Length > 0) {
                switch (args[0]) {
                case "-i":
                    InstanceAnalyzer.analyzeAllInstances("Instances.xlsx");
                    return;
                case "-l":
                    LogAnalyzer.analyzeAllLogs(args.Skip(1).ToArray());
                    return;
                }
            }

            LogAnalyzer.analyzeAllLogs(LogAnalyzer.DefaultLogPath);
        }
    }
}
