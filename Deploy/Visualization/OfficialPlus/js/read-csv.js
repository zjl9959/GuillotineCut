var widthPlates = 3210; // prefixed plate width value.
var canvheight = document.getElementById("canvas").height - 100;
var scale = canvheight/widthPlates; // Compute pixel scale to adapt plate width to canvas width.

var ErrorIdx = 1; // variable updated in the case of unequal indexes.
var batchIdx = -1; // batch file index.
var solutionIdx = -1; // solution file index.
var nodes = []; // list of node.
var defects = []; // list of defects.
var stacks = []; // list of stacks.
var parseDefects = 0; // updated if defects file was parsed.
var parseBatch = 0; // updated if batch file was parsed.
var loadedSolution = 0; // updated if solution file was loaded.
var loadedBatch = 0; // updated if batch file was loaded.
var canvas = document.getElementById("canvas");
var context = canvas.getContext('2d');
var plateId = 0; // plate Id.
var plateNbr = 0; // number of plates.
var maxtplateId = 0; // max plate Id.
var wasted = []; // waste nodes list.
var used = []; // used nodes list.
var residual = 0; // residual identifier.
var sol_file = ''; // 记录解文件对象。

var canvas_width = canvas.width;
var canvas_height = canvas.height;

/*标记csv文件中每一列的含义*/
var solution_PLATE_ID_col = 0;
var solution_NODE_ID_col  = 1;
var solution_X_col        = 2;
var solution_Y_col        = 3;
var solution_WIDTH_col    = 4;
var solution_HEIGHT_col   = 5;
var solution_TYPE_col     = 6;
var solution_CUT_col      = 7;
var solution_PARENT_col   = 8;

var batch_ITEM_ID_col     = 0;
var batch_LENGTH_ITEM_col = 1;
var batch_WIDTH_ITEM_col  = 2;
var batch_STACK_col       = 3;
var batch_SEQ_col         = 4;

var defects_DEFECT_ID_col = 0;
var defects_PLATE_ID_col  = 1;
var defects_X_col         = 2;
var defects_Y_col         = 3;
var defects_WIDTH_col     = 4;
var defects_HEIGHT_col    = 5;

window.setInterval(updateSolution, 200);	// 设置定时器，每隔一定时间ms就重新加载一下解文件。

// This function will update plateId field and call appropriate functions to redraw canvas area.
// 定义点击previous按钮时将会发生的动作。
document.getElementById('previous').addEventListener('click', function () {
	if (plateId > 0)
		plateId--;
  else
    plateId = maxtplateId;
	document.getElementById('plateId').value = plateId + '/' + maxtplateId;
	drawSolution();
  drawDefects();
}, false);

// This function will update plateId field and call appropriate functions to redraw canvas area.
// 定义点击next按钮时将会发生的动作。
document.getElementById('next').addEventListener('click', function () {
	if (plateId < maxtplateId)
		plateId++;
  else
    plateId = 0;
	document.getElementById('plateId').value = plateId + '/' + maxtplateId;
	drawSolution();
  drawDefects();
}, false);


// This function is called when batch file is selected.
function handleBatch(files) {
  var filepath = document.getElementById('csvBatch').value; // get the filepath
  var filename = filepath.replace(/^.*[\\\/]/, ''); // remove useless character in filepath to store only filename
  var idx = filename.split('_batch.csv'); // splits the filename to get only batch index
  batchIdx = idx[0];
  // test if batch and solution files indexes are different then redraw canvas.
  if(loadedSolution) {
    if(batchIdx != solutionIdx) {
      ErrorIdx = 1;
      drawSolution();
      alert('Error: Batch file idx (' + batchIdx + ') is different of Solution file idx (' + solutionIdx +')');
      return;
    }
  }
  ErrorIdx = 0;
	parseBatch = 1;
  // Check for the various File API support.
	if(window.FileReader) {
		// FileReader are supported.
    loadedBatch = 1;
		getAsText(files[0]);
	} else {
		alert('FileReader are not supported in this browser.');
	}
}

// This function is called when defects file is selected.
function handleDefects(files) {
	parseDefects = 1;
  // Check for the various File API support.
	if(window.FileReader) {
		// FileReader are supported.
		getAsText(files[0]);
	} else {
		alert('FileReader are not supported in this browser.');
	}
}


// This function is called when solution file is selected.
function handleSolution(files) {
  plateId = 0;
  maxPlateId = 0;
  plateNbr = 0;
  var filepath = document.getElementById('csvFileInput').value;
  var filename = filepath.replace(/^.*[\\\/]/, ''); // remove useless character in filepath to store only filename
  var idx = filename.split('_solution.csv'); // splits the filename to get only solution index
  solutionIdx = idx[0];
  if(loadedBatch) {
    if(solutionIdx != batchIdx) {
      ErrorIdx = 1;
      alert('Error: Solution file idx(' + solutionIdx + ') is different of Batch file idx(' + batchIdx +')');
    }
  }
	// Check for the various File API support.
	if(window.FileReader) {
		// FileReader are supported.
    loadedSolution = 1;
	  sol_file = files[0];
		getAsText(files[0]);
	} else {
		alert('FileReader are not supported in this browser.');
	}
}

function updateSolution() {
	if(sol_file != '') {
		getAsText(sol_file);
	}
}

// This function is called by file handlers (handleSolution, handleDefects and handleBatch) functions.
function getAsText(fileToRead) {
	var reader = new FileReader();
	// Handle errors load
	reader.onload = loadhandler;
	reader.onerror = errorHandler;
	// Read file into memory as UTF-8
	reader.readAsText(fileToRead);
}


// This function is a callback function. This function choose what data the code will process.
function loadhandler(event) {
	var csv = event.target.result;
  if (parseDefects == 1)
    processDefects (csv);
  else if (parseBatch == 1)
    processBatch(csv);
  else
    processSolution(csv);
}


// This function is callback in case of error.
function errorHandler(evt) {
	if(evt.target.error.name == "NotReadableError") {
		alert("Canno't read file !");
	}
}


// This function parse batch file into stacks array.
// 将文件中的Batch信息加载到stacks变量中。
function processBatch(csv) {
  parseBatch = 0;
  stacks = [];
  var allTextdefects = csv.split(/\r\n|\n/);
  while (allTextdefects.length) {
    stacks.push(allTextdefects.shift().split(';'));
  }
  if (loadedSolution)
  {
    drawSolution ();
    drawDefects();
  }
}

// This function parse defects file into defects array.
// 将文件中的Defects信息加载到defects变量中。
function processDefects(csv) {
  parseDefects = 0;
  defects = [];
  var allTextdefects = csv.split(/\r\n|\n/);
  while (allTextdefects.length) {
    defects.push(allTextdefects.shift().split(';'));
  }
  if (loadedSolution)
  {
    drawSolution ();
    drawDefects();
  }
}

// This function parse solution file into nodes array.
// 将文件中的Solution信息加载到nodes变量中。
function processSolution(csv) {
	nodes = [];
  var allTextLines = csv.split(/\r\n|\n/);
  while (allTextLines.length) {
    nodes.push(allTextLines.shift().split(';'));
  }
  parsePlts ();
  document.getElementById('plateId').value = plateId + '/' + maxtplateId;
	drawSolution ();
  drawDefects();
}

function parsePlts () {
  maxtplateId = Math.floor(nodes[nodes.length - 2][solution_PLATE_ID_col]);
  plateNbr = maxtplateId + 1;
  for (var i = 0; i < plateNbr; i++) {
    wasted[i] = 0;
    used[i] = 0;
  }
}


/// This function will clear canvas area then redraw it according to plateId.
// 画出当前原料上的所有物品信息。
function drawSolution() {
	var something_drawn = 0;
	
	context.clearRect(0, 0, canvas_width, canvas_height);
	var offset = 50;
	var x /* x coord */, y /* y coord */, w /* width*/, h /*height*/, c /* cut */;
	
	for (var i = nodes.length - 1; i >= 0 ; i--)
	{
		x = (nodes[i][solution_X_col]*scale) + offset;
		y = (nodes[i][solution_Y_col]*scale) + offset;
		w = nodes[i][solution_WIDTH_col]*scale;
		h = nodes[i][solution_HEIGHT_col]*scale;
		y = canvas_height - y - h;
		c = nodes[i][solution_CUT_col];
		// If node's plate id equals to selected plateId and skip first file raw.
		if ((nodes[i][solution_PLATE_ID_col] == plateId) && (i > 0))
		{
			context.font="10px Verdana";
			context.fillStyle = "rgb(0, 0, 0)";
			if (c == 1 && w > 50) {
				var scale_x = parseInt(nodes[i][solution_X_col]) + parseInt(nodes[i][solution_WIDTH_col]);
				context.fillText(scale_x, x+w-10, 565);
			}
			if (c == 2 && h > 50) {
				var scale_y = parseInt(nodes[i][solution_Y_col]) + parseInt(nodes[i][solution_HEIGHT_col]);
				context.fillText(scale_y, 20, y+3);
			}
			if ((nodes[i][solution_TYPE_col] != -2)) {
				something_drawn = 1;
				context.beginPath();
				context.strokeStyle = "#000000";
				context.lineWidth = "1";
				if (nodes[i][solution_TYPE_col] == -1) {
					if ((nodes[i][solution_PLATE_ID_col] == maxtplateId) && (i == nodes.length - 1)) {
						context.fillStyle = "rgb(160, 160, 160)";
					} else {
						context.fillStyle = "rgb(188, 194, 197)";
					}
					context.rect(x, y, w, h);
					context.stroke();
					context.fill();
				}else if (nodes[i][solution_TYPE_col] ==  -3) {
					context.fillStyle = "rgb(160, 160, 160)";
					context.rect(x, y, w, h);
					context.stroke();
					context.fill();
				} else {
					context.fillStyle = "rgb(50, 175, 229)";
					context.rect(x, y, w, h);
					context.stroke();
					context.fill();
					context.font="10px Verdana";
					context.fillStyle = "rgb(0, 0, 0)";
					if (w > 40 && h > 50) {
						if (ErrorIdx == 0){
							context.fillText('id: ' + Math.floor(nodes[i][solution_TYPE_col]), x + w/2 - 12, y + h/2 - 17);
							context.fillText('w: ' + Math.floor(nodes[i][solution_WIDTH_col]), x + w/2 - 12, y + h/2 - 7);
							context.fillText('h: ' + Math.floor(nodes[i][solution_HEIGHT_col]), x + w/2 - 12, y + h/2 + 3);
							for (var j = 0; j < stacks.length-1; j++) {
								if(stacks[j][batch_ITEM_ID_col] == nodes[i][solution_TYPE_col]) {
									context.fillText('stk: ' + Math.floor(stacks[j][batch_STACK_col]), x + w/2 - 12, y + h/2 + 13);
									context.fillText('seq: ' + Math.floor(stacks[j][batch_SEQ_col]), x + w/2 - 12, y + h/2 + 23);
								}
							}
						} else {
							context.fillText('id: ' + Math.floor(nodes[i][solution_TYPE_col]), x + w/2 - 12, y + h/2 - 7);
							context.fillText('w: ' + Math.floor(nodes[i][solution_WIDTH_col]), x + w/2 - 12, y + h/2 + 3);
							context.fillText('h: ' + Math.floor(nodes[i][solution_HEIGHT_col]), x + w/2 - 12, y + h/2 + 13);
						}
					} else {
						context.fillText('id: ' + Math.floor(nodes[i][solution_TYPE_col]), x + w/2 - 12, y + h/2 + 2);
					}
				}
			} else if(something_drawn == 0){
				if (w > 50) {
					var scale_x = parseInt(nodes[i][solution_X_col]) + parseInt(nodes[i][solution_WIDTH_col]);
					context.fillText(scale_x, x+w-10, 565);
				}
				if (h > 50) {
					var scale_y = parseInt(nodes[i][solution_Y_col]) + parseInt(nodes[i][solution_HEIGHT_col]);
					context.fillText(scale_y, 20, y+3);
				}
				context.strokeStyle = "#000000";
				context.lineWidth = "1";
				context.fillStyle = "rgb(188, 194, 197)";
				context.rect(x, y, w, h);
				context.stroke();
				context.fill();
			}
		}
	}
}


/// This function will draw defects on plate according to plateId.
// 画出当前原料上的所有瑕疵。
function drawDefects(){
  var offset = 50;
  var x ,y, w, h;
  for (var i = defects.length - 1; i >= 0 ; i--)
  {
    if (defects[i][1] == plateId) {
			x =((defects[i][defects_X_col]*scale) + offset);
      y =(defects[i][defects_Y_col]*scale) + offset;
      w =((defects[i][defects_WIDTH_col]*scale));
      h =((defects[i][defects_HEIGHT_col]*scale));
      y = canvas_height - y - h;
      
	  context.beginPath();
      context.fillStyle = "rgb(255, 0, 0)";
      context.arc(x, y, 2, 0,2*Math.PI);
      context.fill();
	  
	  //context.beginPath();
	  //context.rect(x,y,w,h);
	  //context.fillStyle = "red";
	  //context.fill();
    }
  }
}
