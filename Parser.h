#pragma once

#include "./Include/json.hpp"
#include <iostream>
#include <fstream>

#include <string>

#include <bitset>
#include <sstream>
#include <Windows.h>
#include <conio.h>	//ÉLÅ[âüÇ∑óvàˆ
using std::bitset;

using json = nlohmann::json;
using std::string;

class Parser
{
public:
	Parser();
	bool Init(int argc, char* argv[]);
	bool Process();
private:
	json data, linkPathData;
	enum OUTPUT_STATE {
		OS_NONE,
		OS_INFO,
		OS_WARNING,
		OS_ERROR
	};

	bool SelectFile(string* path);
	bool End();
	//void Result();
	void ConvertData(unsigned int tiledValue, string* uePath, int* ueTileValue);
	void OutText(string str, OUTPUT_STATE outState = OS_NONE);


	std::string toBinary(unsigned int n)
	{
		std::string r;
		while (n != 0) { r = (n % 2 == 0 ? "0" : "1") + r; n /= 2; }
		return r;
	}
};

