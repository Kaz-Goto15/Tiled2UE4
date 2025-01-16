#pragma once

#include "./Include/json.hpp"
#include <iostream>

#include <filesystem>
#include <vector>
#include <tchar.h>
#include <bitset>
#include <sstream>
#include <Windows.h>
#include <conio.h>	//キー押す要因
#include <locale>
#include <string>
using std::bitset;

using std::vector;
using json = nlohmann::json;
using std::string;
using std::wstring;

using namespace std;
class Parser
{
public:
	//コンストラクタ
	Parser();
	//メイン処理機構
	bool Process(int argc, wchar_t* argv[]);

private:
	wstring parentDir;
	json linkPathJson;

	json data;	//読込jsonデータの格納変数
	vector<int> useLinkDataIndexes;

	//初期化
	wstring outDir;
	bool Init(wchar_t* exePath);

	//変換データ系
	vector<wstring> parsePaths;	//変換データのパスを格納する配列
	bool StoreParseFile(int argc, wchar_t* argv[], vector<wstring>* paths);	//読込-基本

	struct STR_FILTER {
		wstring descr = L"";
		wstring ext = L"*";
	};
	void SelectFile(vector<wstring>* storePaths, vector<STR_FILTER> filters, wstring dir = L"", bool enAllFile = true);		//複数選択、複数フィルタ
	void SelectFile(vector<wstring>* storePaths, STR_FILTER filter, wstring dir = L"", bool enAllFile = true);				//複数選択、単一フィルタ
	void SelectFile(wstring* storePath, vector<STR_FILTER> filters, wstring dir = L"", bool enAllFile = true);											//単一選択、複数フィルタ
	void SelectFile(wstring* storePath, STR_FILTER filter, wstring dir = L"", bool enAllFile = true);													//単一選択、単一フィルタ
	vector<wstring> SelectFile_proc(vector<STR_FILTER> filters, wstring dir, bool enAllFile, bool isSingleFile);	//処理

	bool Read(wstring _path, json* _data);								//マップデータを読み込みデータを格納する
	void Parse(wstring _path, json data);								//変換

	bool End();
	void Result();
	void ConvertData(unsigned int tiledValue, wstring* uePath, int* ueTileValue);


	wstring GetStem(wstring path);


	string ExtractTexture(string filePath_tileset);
	string ExtractImportData(string filePath_);

	void AddLinkDataW(wstring tiled_sourcePath, wstring ue4_path);

	//変換関数
	void StoreWStr(wstring* wstr, json* j);
	wchar_t* GetWC(const char* c);
	string ConvStr(wstring& src);
	// ===================== 汎用入出力関数 =====================
	//キー入力(isgraph)
	char GetKey(wstring descr = L"");

	//Y/N入力
	bool InputBool(wstring descr = L"");

	//JSON出力
	void OutputJson(wstring filePath, json content);

	// ===================== 標準入出力関数 =====================
	enum OUTPUT_STATE {
		OS_NONE,
		OS_INFO,
		OS_WARNING,
		OS_ERROR
	};
	//状態付き出力
	//void OutText(string str, OUTPUT_STATE outState = OS_NONE);
	void OutText(wstring wstr, OUTPUT_STATE outState = OS_NONE);

	//処理中断・警告
	bool BreakNIsContinue(wstring warnStr);

	//改行
	void Br() { std::wcout << std::endl; }

	//string配列を全出力
	void PrintStrList(vector<wstring>* descrList, int startNum = 0);
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
	string PGetCurrentDirectoryA();
	wstring PGetCurrentDirectoryW();
};

