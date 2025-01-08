#include "Parser.h"
#include "jsonTest.h"
int main(int argc, char* argv[]){
	
	Parser* parser = new Parser();
	//if(parser->Init())
	return parser->Process(argc, argv);
	
	//jsonTest* jt = new jsonTest();
	//jt->Process(argv[0]);
}