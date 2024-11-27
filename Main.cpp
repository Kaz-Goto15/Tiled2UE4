#include "Parser.h"

int main(int argc, char* argv[]){
	Parser* parser = new Parser();
	//if(parser->Init())
	return parser->Process(argc, argv);
}