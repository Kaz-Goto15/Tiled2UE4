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

class Parser
{
public:
	//コンストラクタ
	Parser();
	//メイン処理機構
	bool Process(int argc, char* argv[]);

private:
	json linkPathData;

	json data;	//読込jsonデータの格納変数

	enum OUTPUT_STATE {
		OS_NONE,
		OS_INFO,
		OS_WARNING,
		OS_ERROR
	};

	//初期化
	string exeDir;
	bool Init(char* exePath);

	//変換データ系
	vector<string> parsePaths;	//変換データのパスを格納する配列
	bool StoreParseFile(int argc, char* argv[], vector<string>* paths);	//読込-基本
	void SelectFile(vector<string>* paths);								//読込-ウィンドウ選択
	bool Read(string _path, json* _data);								//マップデータを読み込みデータを格納する
	void Parse(string _path, json data);								//変換

	bool End();
	void Result();
	void ConvertData(unsigned int tiledValue, string* uePath, int* ueTileValue);
	void OutText(string str, OUTPUT_STATE outState = OS_NONE);


	std::string toBinary(unsigned int n)
	{
		std::string r;
		while (n != 0) { r = (n % 2 == 0 ? "0" : "1") + r; n /= 2; }
		return r;
	}
};

