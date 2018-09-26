using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using ClosedXML.Excel;


namespace Analyzer {
    /// <summary> result[config][instance] is statistics of instance solved with config. </summary>
    using Result = Dictionary<string, Dictionary<string, Statistic>>;


    public class LogAnalyzer {
        public const int InstanceNameColumn = 2;
        public const int HeaderRowNum = 5;
        public const int InstanceNum = 20;
        public const int GapRowNum = 1;
        public const int BestObjStartRow = 1;
        public const int AverageObjStartRow = BestObjStartRow + HeaderRowNum + InstanceNum + GapRowNum;
        public const string NormalizedScoreFormula = "100"
            + "-MIN(5,COUNTIF(R[1]C3:R[1]C100,\"<\"&R[1]C))"
            + "-MIN(5,COUNTIF(R[2]C3:R[2]C100,\"<\"&R[2]C))"
            + "-MIN(5,COUNTIF(R[3]C3:R[3]C100,\"<\"&R[3]C))"
            + "-MIN(5,COUNTIF(R[4]C3:R[4]C100,\"<\"&R[4]C))"
            + "-MIN(5,COUNTIF(R[5]C3:R[5]C100,\"<\"&R[5]C))"
            + "-MIN(5,COUNTIF(R[6]C3:R[6]C100,\"<\"&R[6]C))"
            + "-MIN(5,COUNTIF(R[7]C3:R[7]C100,\"<\"&R[7]C))"
            + "-MIN(5,COUNTIF(R[8]C3:R[8]C100,\"<\"&R[8]C))"
            + "-MIN(5,COUNTIF(R[9]C3:R[9]C100,\"<\"&R[9]C))"
            + "-MIN(5,COUNTIF(R[10]C3:R[10]C100,\"<\"&R[10]C))"
            + "-MIN(5,COUNTIF(R[11]C3:R[11]C100,\"<\"&R[11]C))"
            + "-MIN(5,COUNTIF(R[12]C3:R[12]C100,\"<\"&R[12]C))"
            + "-MIN(5,COUNTIF(R[13]C3:R[13]C100,\"<\"&R[13]C))"
            + "-MIN(5,COUNTIF(R[14]C3:R[14]C100,\"<\"&R[14]C))"
            + "-MIN(5,COUNTIF(R[15]C3:R[15]C100,\"<\"&R[15]C))"
            + "-MIN(5,COUNTIF(R[16]C3:R[16]C100,\"<\"&R[16]C))"
            + "-MIN(5,COUNTIF(R[17]C3:R[17]C100,\"<\"&R[17]C))"
            + "-MIN(5,COUNTIF(R[18]C3:R[18]C100,\"<\"&R[18]C))"
            + "-MIN(5,COUNTIF(R[19]C3:R[19]C100,\"<\"&R[19]C))"
            + "-MIN(5,COUNTIF(R[20]C3:R[20]C100,\"<\"&R[20]C))";

        public const string DefaultLogPath = "log.csv";
        public const string OutputDir = "";
        public const string OutputPath = OutputDir + "Statistics.xlsm";

        public enum LogHeaderIndex { Time, ID, Instance, Feasible, ObjMatch, Width, Duration, PhysMem, VirtMem, RandSeed, Config, Generation, Iteration, Util, Waste, Solution };
        public const string LogHeader = "Time,ID,Instance,Feasible,ObjMatch,Width,Duration,PhysMem,VirtMem,RandSeed,Config,Generation,Iteration,Util,Waste,Solution";

        public enum ConfigParameterIndex {
            InitAlgorithm, SearchAlgorithm, GreedyInitRetryCount,
            PossibilityOfRestartFromGlobalOptima, RatioOfMinDeliveryQuantityToNodeFreeCapacity, RatioOfMinDeliveryQuantityToTrailerCapacity,
            RefillOrDeliverySelectionCoefficient, CustomerSelectRangeExpandRate, MaxDriverTrailerPairSelectionPerturbCycle,
            Length
        };

        /// <summary> return true if the line of log fit the requirement. </summary>
        public delegate bool LogFilter(string line);


        public static void analyzeAllLogs(params string[] logPaths) {
            if (logPaths.Length <= 0) { return; }

            if ((OutputDir != null) && (OutputDir.Length > 0)) { Directory.CreateDirectory(OutputDir); }

            Result result = new Result();
            foreach (var item in logPaths) { analyzeLog(result, item); }
            writeResult(result);
        }

        public static void analyzeLog(Result result, string logPath = DefaultLogPath) {
            string[] lines = File.ReadAllLines(logPath);
            for (int r = 1; r < lines.Length; ++r) { // skip header.
                string[] items = lines[r].Split(',');

                string config = items[(int)LogHeaderIndex.Config].Trim();
                string instance = items[(int)LogHeaderIndex.Instance].Trim().Split('/').Last(); // extract file name.
                bool feasible = (items[(int)LogHeaderIndex.Feasible] == "1");
                int obj = Convert.ToInt32(items[(int)LogHeaderIndex.Waste]);

                Dictionary<string, Statistic> cfgResult;
                if (result.ContainsKey(config)) {
                    cfgResult = result[config];
                } else {
                    cfgResult = new Dictionary<string, Statistic>();
                    result.Add(config, cfgResult);
                }

                if (cfgResult.ContainsKey(instance)) {
                    Statistic statistic = cfgResult[instance];
                    ++statistic.runCount;
                    if (feasible) {
                        ++statistic.feasibleCount;
                        if (obj < statistic.bestObj) { statistic.bestObj = obj; }
                        statistic.feasibleObjSum += obj;
                    }
                } else { // add a new entry for this instance.
                    Statistic statistic = new Statistic();
                    statistic.startDate = items[(int)LogHeaderIndex.Time].Trim().Split('_').First();
                    statistic.runCount = 1;
                    statistic.feasibleCount = feasible ? 1 : 0;
                    statistic.bestObj = feasible ? obj : Int32.MaxValue;
                    statistic.feasibleObjSum = feasible ? obj : 0;
                    cfgResult.Add(instance, statistic);
                }
            }
        }

        public static void writeResult(Result result) {
            File.Copy(OutputPath, OutputPath + ".bak.xlsm", true);

            XLWorkbook workbook = new XLWorkbook(OutputPath);
            IXLWorksheet worksheet = workbook.Worksheet(1);

            int resultColumn = worksheet.ColumnsUsed().Count();
            foreach (var item in result) {
                ++resultColumn;
                var cfgResult = item.Value;
                int bestObjStartRow = BestObjStartRow;
                int averageObjStartRow = AverageObjStartRow;

                worksheet.Cell(bestObjStartRow++, resultColumn).Value = item.Key;
                worksheet.Cell(averageObjStartRow++, resultColumn).Value = item.Key;

                worksheet.Cell(bestObjStartRow++, resultColumn).Value = cfgResult.First().Value.startDate;
                worksheet.Cell(averageObjStartRow++, resultColumn).Value = cfgResult.First().Value.startDate;

                // EXTEND[szx][5]: add RunTime row (more run time can compensate bad configuration which may confuse the comparison).

                int feasibleCount = 0;
                int runCount = 0;
                for (int i = 0; i < InstanceNum; ++i) {
                    string instanceName = worksheet.Cell(BestObjStartRow + HeaderRowNum + i, InstanceNameColumn).GetString();
                    if (!cfgResult.ContainsKey(instanceName)) { continue; }
                    feasibleCount += cfgResult[instanceName].feasibleCount;
                    runCount += cfgResult[instanceName].runCount;
                }
                worksheet.Cell(bestObjStartRow++, resultColumn).Value = feasibleCount;
                worksheet.Cell(averageObjStartRow++, resultColumn).Value = feasibleCount;
                worksheet.Cell(bestObjStartRow++, resultColumn).Value = runCount;
                worksheet.Cell(averageObjStartRow++, resultColumn).Value = runCount;

                worksheet.Cell(bestObjStartRow++, resultColumn).SetFormulaR1C1(NormalizedScoreFormula);
                worksheet.Cell(averageObjStartRow++, resultColumn).SetFormulaR1C1(NormalizedScoreFormula);

                for (int i = 0; i < InstanceNum; ++i) {
                    string instanceName = worksheet.Cell(bestObjStartRow + i, InstanceNameColumn).GetString();
                    if (!cfgResult.ContainsKey(instanceName)) { continue; }
                    worksheet.Cell(bestObjStartRow + i, resultColumn).Value = cfgResult[instanceName].bestObj;
                    worksheet.Cell(averageObjStartRow + i, resultColumn).Value = cfgResult[instanceName].AverageObj;
                }
            }

            workbook.Save();
        }

        public static List<string> mergeLog(LogFilter filter, params string[] logPaths) {
            List<string> result = new List<string>();
            result.Add(LogHeader);

            foreach (var path in logPaths) {
                var lines = File.ReadAllLines(path);
                for (int i = 1; i < lines.Length; ++i) { // skip header.
                    if (filter(lines[i])) { result.Add(lines[i]); }
                }
            }

            return result;
        }

        public static List<string> mergeFeasibleLogLines(params string[] logPaths) {
            return mergeLog((line) => {
                string[] items = line.Split(',');
                if (items.Length <= (int)LogHeaderIndex.Feasible) { return false; }
                return (Convert.ToInt32(items[(int)LogHeaderIndex.Feasible]) == 1);
            }, logPaths);
        }

        public static void mergeLog(string resultPath, LogFilter filter, params string[] logPaths) {
            File.WriteAllLines(resultPath, mergeLog(filter, logPaths));
        }

        public static void mergeFeasibleLogLines(string resultPath, params string[] logPaths) {
            File.WriteAllLines(resultPath, mergeFeasibleLogLines(logPaths));
        }
    }


    public class Statistic {
        public string startDate;
        public int runCount;
        public int feasibleCount;
        public int bestObj;
        public int feasibleObjSum;

        public int AverageObj { get { return feasibleCount > 0 ? feasibleObjSum / feasibleCount : int.MaxValue; } }
    }
}
