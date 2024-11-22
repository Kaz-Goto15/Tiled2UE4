#include "Parser.h"

int main(int argc, char* argv[]){
	Parser* parser = new Parser();
	parser->Init(argc, argv);
	return parser->Process();
}