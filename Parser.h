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
#include <locale>
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
	string parentDir;
	json linkPathJson;

	json data;	//読込jsonデータの格納変数
	vector<int> useLinkDataIndexes;

	//初期化
	string outDir;
	bool Init(char* exePath);

	//変換データ系
	vector<string> parsePaths;	//変換データのパスを格納する配列
	bool StoreParseFile(int argc, char* argv[], vector<string>* paths);	//読込-基本

	struct STR_FILTER {
		string descr = "";
		string ext = "*";
	};
	void SelectFile(vector<string>* storePaths, vector<STR_FILTER> filters, string dir = "", bool enAllFile = true);		//複数選択、複数フィルタ
	void SelectFile(vector<string>* storePaths, STR_FILTER filter, string dir = "", bool enAllFile = true);				//複数選択、単一フィルタ
	void SelectFile(string* storePath, vector<STR_FILTER> filters, string dir = "", bool enAllFile = true);											//単一選択、複数フィルタ
	void SelectFile(string* storePath, STR_FILTER filter, string dir = "", bool enAllFile = true);													//単一選択、単一フィルタ
	vector<string> SelectFile_proc(vector<STR_FILTER> filters, string dir, bool enAllFile, bool isSingleFile);	//処理

	bool Read(string _path, json* _data);								//マップデータを読み込みデータを格納する
	void Parse(string _path, json data);								//変換

	bool End();
	void Result();
	void ConvertData(unsigned int tiledValue, string* uePath, int* ueTileValue);


	string GetStem(string path);


	string ExtractTexture(string filePath_tileset);
	string ExtractImportData(string filePath_);

	// ===================== 汎用入出力関数 =====================
	//キー入力(isgraph)
	char GetKey(string descr = "");

	//Y/N入力
	bool InputBool(string descr = "");

	//JSON出力
	void OutputJson(string filePath, json content);

	// ===================== 標準入出力関数 =====================
	enum OUTPUT_STATE {
		OS_NONE,
		OS_INFO,
		OS_WARNING,
		OS_ERROR
	};
	//状態付き出力
	void OutText(string str, OUTPUT_STATE outState = OS_NONE);

	//処理中断・警告
	bool BreakNIsContinue(string warnStr);

	//改行
	void Br() { std::cout << std::endl; }

	//string配列を全出力
	void PrintStrList(vector<string>* descrList, int startNum = 0);
	// ===================== 汎用関数 =====================

	//値が範囲内か
	template <class T>
	bool Between(T value, T min, T max) { return (min <= value && value <= max); }

	//SQLのIn句と同じ
	template <class T>
	bool In(T val, vector<T> search) {
		for (auto& word : search) {
			if (val == word)return true;
		}
		return false;
	}

	//SQLのLIKE句と同じ
	bool Like(string val, string search);

	//半分にする 型をそのまんま返すためintなどは自動切り捨て
	template <class T>
	T Half(T value) { return (value / 2.0f); }

	//2倍にする
	template <class T>
	T Twice(T value) { return (value * 2); }

	//偶数かを見る
	bool IsEven(int value) { return (value % 2 == 0); }

	//0d to 0b バイナリ変換
	string toBinary(unsigned int n);

	//カレントディレクトリの取得
	string PGetCurrentDirectory();
};

