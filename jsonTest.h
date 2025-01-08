#pragma once

#include "./Include/json.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

#include <bitset>
#include <sstream>
#include <Windows.h>
#include <conio.h>	//キー押す要因
using std::bitset;

using std::vector;
using json = nlohmann::json;
using std::string;

class jsonTest
{
public:
	//コンストラクタ
	jsonTest();
	//メイン処理機構
	bool Process(char* exePath);

private:

	enum OUTPUT_STATE {
		OS_NONE,
		OS_INFO,
		OS_WARNING,
		OS_ERROR
	};
	void OutText(string str, OUTPUT_STATE outState = OS_NONE);
	string outDir;

	json linkPathData;
};

