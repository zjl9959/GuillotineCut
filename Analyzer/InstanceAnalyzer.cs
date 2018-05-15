using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;


namespace Analyzer {
    // zero-based consecutive integer identifier.
    using ID = Int32;
    // the unit of width and height.
    using Length = Int32;
    // the unit of x and y coordinates.
    using Coord = Int32;
    // the unit of elapsed computational time.
    using Duration = Int32;
    // number of neighborhood moves in local search.
    using Iteration = Int32;


    public class InstanceAnalyzer {
        public class Rect {
            public Rect() { }
            public Rect(Length width, Length height) {
                w = width;
                h = height;
            }

            Length w; // width.
            Length h; // height.
        }

        public class RectArea : Rect { // a rectangular area on the plate.
            public RectArea() { }
            public RectArea(Coord left, Coord bottom, Length width, Length height)
                : base(width, height) {
                x = left;
                y = bottom;
            }

            Coord x; // left.
            Coord y; // bottom.
        }


        public const string InstanceDir = "Instance/";
        public const string BatchSuffix= "_batch.csv";
        public const string DefectsSuffix= "_defects.csv";

        public const ID InvalidItemId = -1;


        public static void analyzeAllInstances(string outputPath) {
            for (int i = 1; i <= 20; ++i) {
                InstanceAnalyzer instAnalyzer = new InstanceAnalyzer();
                instAnalyzer.analyzeInstance("A", i);
            }
        }

        public void analyzeInstance(string dataset, int index) {
            string instanceName = dataset + index;
            string instancePrefix = InstanceDir + instanceName;

            Input input = new Input();
            input.load(instancePrefix + BatchSuffix, instancePrefix + DefectsSuffix);

            // items.
            for (int i = 0; i < input.batch.Count; ++i) {
                Input.Item item = input.batch[i];
                items[item.id] = new Rect(Math.Max(item.width, item.height), Math.Min(item.width, item.height));
                if (!stacks.ContainsKey(item.stack)) { stacks.Add(item.stack, new List<ID>()); }
                while (stacks[item.stack].Count <= item.seq) { stacks[item.stack].Add(InvalidItemId); }
                stacks[item.stack][item.seq] = item.id;
            }
            foreach (var stack in stacks) {
                stack.Value.RemoveAll((e) => (e == InvalidItemId));
            }

            // defects.
            for (int d = 0; d < input.defects.Count; ++d) {
                Input.Defect defect = input.defects[d];
                defects[defect.id] = new RectArea(defect.x, defect.y, defect.width, defect.height);
                while (plates.Count <= defect.plateId) { plates.Add(new List<ID>()); }
                plates[defect.plateId].Add(defect.id);
            }

            Console.WriteLine(instanceName);
            Console.Write("  itemNum=" + items.Count);
            Console.Write("  stackNum=" + stacks.Count);
            Console.WriteLine("  defectNum=" + defects.Count);
        }


        public Dictionary<ID, Rect> items = new Dictionary<ID, Rect>();
        public Dictionary<ID, RectArea> defects = new Dictionary<ID, RectArea>();

        public Dictionary<ID, List<ID>> stacks = new Dictionary<ID, List<ID>>(); // stacks[s][i] is the itemId of the i_th item in the stack s.
        public List<List<ID>> plates = new List<List<ID>>(); // plates[p][i] is the defectId of the i_th defect on plate p.
    }
}
