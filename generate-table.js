var fs = require("fs");
var content = fs.readFileSync("table.json");
var jsonContent = JSON.parse(content);

var streamh = fs.createWriteStream("table.h");
streamh.once('open', function(fd){
	streamh.write("const int TABLE_LENGTH = "+jsonContent.result.descs.length+";\n");
	streamh.write("extern const char* g_unicodeTable[TABLE_LENGTH];\n");
	streamh.write("extern const char* MISSING_CODEPOINT_STRING;\n");
	streamh.end();
});

var streamc = fs.createWriteStream("table.cpp");
streamc.once('open', function(fd){
	streamc.write("#include \"table.h\"\n");
	streamc.write("const char* MISSING_CODEPOINT_STRING = \"<missing codepoint>\";\n");
	streamc.write("const char* g_unicodeTable[TABLE_LENGTH]={\n");
	jsonContent.result.descs.forEach(function(e){
		if (e != "" && e != "<Not a Character>"){
			streamc.write("\""+e+"\",\n");
		} else {
			streamc.write("MISSING_CODEPOINT_STRING,\n");
		}
	});
	streamc.write("};\n");
	streamc.end();
});
