#define _CRT_SECURE_NO_WARNINGS
#include "jsonTest.h"

#include <iostream>
#include <cmath>



using namespace std;

jsonTest::jsonTest()
{
}

bool jsonTest::Process(char* exePath)
{	//実行ファイルのディレクトリを取得
	std::filesystem::path path = exePath;
	outDir = path.parent_path().string() + "\\output\\";
	//TiledとUE4のリンクファイルを読込
	string lpPath = path.parent_path().string() + "\\linkPath.json";
	if (filesystem::exists(lpPath)) {
		std::ifstream lp(lpPath);

		try { linkPathData = json::parse(lp); }
		catch (json::parse_error e) {
			OutText("リンクパスファイルの読込時に問題が発生しました：" + (string)e.what(), OS_ERROR);
			return false;
		}
	}
	else {
		OutText("リンクパスファイルが見つかりません。", OS_ERROR);
		return 0;
	}

	if (!filesystem::is_directory(outDir)) {
		OutText("出力フォルダがありません。ディレクトリを自動追加します。", OS_WARNING);
		if (!filesystem::create_directory(outDir)) {
			OutText("出力フォルダの作成に失敗しました。管理者権限が必要な場所で実行しているか、容量が足りない可能性があります。", OS_ERROR);
			return 0;
		};
	}

	cout << linkPathData;
	return false;
}

void jsonTest::OutText(string str, OUTPUT_STATE outState) {
	switch (outState)
	{
	case OS_NONE:		cout << " ";						break;
	case OS_INFO:		cout << "[INFO] ";					break;
	case OS_WARNING:	cout << "\033[33m" << "[WARNING] ";	break;
	case OS_ERROR:		cout << "\033[31m" << "[ERROR] ";	break;
	}

	cout << str << "\033[0m" << endl;
}