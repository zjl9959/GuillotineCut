using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using ClosedXML.Excel;


namespace Irp.Analyzer {
    /// <summary> result[config][instance] is statistics of instance solved with config. </summary>
    using Result = Dictionary<string, Dictionary<string, Statistic>>;


    public class LogAnalyzer {
        private static void Main(string[] args) {
            analyzeAllLogs(DefaultLogPath);
        }


        public const int TimeIndex = 0;
        public const int InstanceIndex = 2;
        public const int ConfigIndex = 3;
        public const int FeasibleIndex = 8;
        public const int CheckIndex = 9;
        public const int ObjIndex = 10;

        public const int InstanceNameColumn = 2;
        public const int HeaderRowNum = 5;
        public const int InstanceNum = 20;
        public const int GapRowNum = 1;
        public const int BestObjStartRow = 1;
        public const int AverageObjStartRow = BestObjStartRow + HeaderRowNum + InstanceNum + GapRowNum;
        public const string NormalizedScoreFormula = "=20"
            + "-EXP((R[1]C1-R[1]C)/R[1]C1)"
            + "-EXP((R[2]C1-R[2]C)/R[2]C1)"
            + "-EXP((R[3]C1-R[3]C)/R[3]C1)"
            + "-EXP((R[4]C1-R[4]C)/R[4]C1)"
            + "-EXP((R[5]C1-R[5]C)/R[5]C1)"
            + "-EXP((R[6]C1-R[6]C)/R[6]C1)"
            + "-EXP((R[7]C1-R[7]C)/R[7]C1)"
            + "-EXP((R[8]C1-R[8]C)/R[8]C1)"
            + "-EXP((R[9]C1-R[9]C)/R[9]C1)"
            + "-EXP((R[10]C1-R[10]C)/R[10]C1)"
            + "-EXP((R[11]C1-R[11]C)/R[11]C1)"
            + "-EXP((R[12]C1-R[12]C)/R[12]C1)"
            + "-EXP((R[13]C1-R[13]C)/R[13]C1)"
            + "-EXP((R[14]C1-R[14]C)/R[14]C1)"
            + "-EXP((R[15]C1-R[15]C)/R[15]C1)"
            + "-EXP((R[16]C1-R[16]C)/R[16]C1)"
            + "-EXP((R[17]C1-R[17]C)/R[17]C1)"
            + "-EXP((R[18]C1-R[18]C)/R[18]C1)"
            + "-EXP((R[19]C1-R[19]C)/R[19]C1)"
            + "-EXP((R[20]C1-R[20]C)/R[20]C1)";

        public const string DefaultLogPath = "log.csv";
        public const string OutputDir = "Doc/7 Analysis/";
        public const string OutputPath = OutputDir + "Statistics.xlsm";

        public enum LogHeaderIndex { Time, ID, Instance, Config, RandSeed, Generation, Iteration, Duration, Feasible, Check, ObjValue, Solution };
        public const string LogHeader = "Time,ID,Instance,Config,RandSeed,Generation,Iteration,Duration,Feasible,Check-Obj,ObjValue,Solution";

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

            Directory.CreateDirectory(OutputDir);

            Result result = new Result();

            foreach (var item in logPaths) {
                analyzeLog(result, item);
            }

            writeResult(result);
        }

        public static void analyzeLog(Result result, string logPath = DefaultLogPath) {
            string[] lines = File.ReadAllLines(logPath);
            for (int r = 1; r < lines.Length; ++r) { // skip header.
                string[] items = lines[r].Split(',');

                string config = items[ConfigIndex].Trim();
                string instance = items[InstanceIndex].Trim().Split('/').Last(); // extract file name.
                bool feasible = (items[FeasibleIndex] == "1");
                double obj = Convert.ToDouble(items[ObjIndex].Split('=').First()) + Convert.ToDouble(items[CheckIndex]);

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
                    statistic.startDate = items[TimeIndex].Trim().Split('_').First();
                    statistic.runCount = 1;
                    statistic.feasibleCount = feasible ? 1 : 0;
                    statistic.bestObj = feasible ? obj : double.MaxValue;
                    statistic.feasibleObjSum = feasible ? obj : 0;
                    cfgResult.Add(instance, statistic);
                }
            }
        }

        public static void writeResult(Result result) {
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

                int averageFeasibleCount = 0;
                int averageRunCount = 0;
                for (int i = 0; i < InstanceNum; ++i) {
                    string instanceName = worksheet.Cell(BestObjStartRow + HeaderRowNum + i, InstanceNameColumn).GetString();
                    if (!cfgResult.ContainsKey(instanceName)) { continue; }
                    averageFeasibleCount += cfgResult[instanceName].feasibleCount;
                    averageRunCount += cfgResult[instanceName].runCount;
                }
                averageFeasibleCount /= InstanceNum;
                averageRunCount /= InstanceNum;
                worksheet.Cell(bestObjStartRow++, resultColumn).Value = averageFeasibleCount;
                worksheet.Cell(averageObjStartRow++, resultColumn).Value = averageFeasibleCount;
                worksheet.Cell(bestObjStartRow++, resultColumn).Value = averageRunCount;
                worksheet.Cell(averageObjStartRow++, resultColumn).Value = averageRunCount;

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
                //var lines = File.ReadAllLines(path).Skip(1); // skip header.
                //foreach (var line in lines) {
                //    if (filter(line)) { result.Add(line); }
                //}
                var lines = File.ReadAllLines(path);
                for (int i = 1; i < lines.Length; ++i) {
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

        public static void configRank(string resultPath, params string[] logPaths) {
            List<string> lines = mergeFeasibleLogLines(logPaths);
            Dictionary<string, int> configCount = new Dictionary<string, int>();
            Dictionary<double, int>[] parameterCount = new Dictionary<double, int>[(int)ConfigParameterIndex.Length];
            for (int i = 0; i < parameterCount.Length; ++i) {
                parameterCount[i] = new Dictionary<double, int>();
            }

            for (int i = 1; i < lines.Count; ++i) {
                string[] items = lines[i].Split(',');
                string config = items[(int)LogHeaderIndex.Config];
                if (!configCount.ContainsKey(config)) { configCount.Add(config, 0); }
                ++configCount[config];

                string[] parameters = config.Split('[', ']')[1].Split('|');
                for (int j = (int)ConfigParameterIndex.RatioOfMinDeliveryQuantityToNodeFreeCapacity; j < (int)ConfigParameterIndex.Length; ++j) {
                    //double val = Convert.ToDouble(Regex.Match(parameters[j], @"[\d.]+").Value);
                    double val = Convert.ToDouble(parameters[j].Substring(parameters[j].IndexOfAny("0123456789+-.".ToCharArray())));
                    if (!parameterCount[j].ContainsKey(val)) { parameterCount[j].Add(val, 0); }
                    ++parameterCount[j][val];
                }
            }

            var rank = configCount.OrderByDescending((p) => p.Value);
            List<string> result = new List<string>();
            foreach (var item in rank) {
                result.Add(item.Value + "," + item.Key);
            }
            foreach (ConfigParameterIndex i in Enum.GetValues(typeof(ConfigParameterIndex))) {
                if (i == ConfigParameterIndex.Length) { break; }
                result.Add("");
                result.Add(i.ToString() + ":");
                var count = parameterCount[(int)i].OrderByDescending((p) => p.Value);
                foreach (var item in count) {
                    result.Add(item.Value + "," + item.Key);
                }
            }

            File.WriteAllLines(resultPath, result);
        }
    }


    public class Statistic {
        public string startDate;
        public int runCount;
        public int feasibleCount;
        public double bestObj;
        public double feasibleObjSum;

        public double AverageObj { get { return feasibleObjSum / feasibleCount; } }
    }

}
