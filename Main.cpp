#include "Parser.h"
#include "jsonTest.h"
#include <io.h>     // _setmode
#include <fcntl.h>  // _O_U16TEXT
int wmain(int argc, wchar_t* argv[]){
	_setmode(_fileno(stdout), _O_U16TEXT);
	Parser* parser = new Parser();
	//if(parser->Init())
	return parser->Process(argc, argv);
	
	//jsonTest* jt = new jsonTest();
	//jt->Process(argv[0]);
}