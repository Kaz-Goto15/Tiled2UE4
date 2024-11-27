#include "Parser.h"

#include <iostream>
#include <cmath>


using namespace std;

Parser::Parser()
{
}

bool Parser::Init(char* exePath) {
	//実行ファイルのディレクトリを取得
	std::filesystem::path path = exePath;
	exeDir = path.parent_path().string();
	//TiledとUE4のリンクファイルを読込
	string lpPath = exeDir + "\\linkPath.json";
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

	if (!filesystem::is_directory(exeDir + "\\output")) {
		OutText("出力フォルダがありません。ディレクトリを自動追加します。", OS_WARNING);
		if (!filesystem::create_directory(exeDir + "\\output")) {
			OutText("出力フォルダの作成に失敗しました。管理者権限が必要な場所で実行しているか、容量が足りない可能性があります。", OS_ERROR);
			return 0;
		};
	}
}

bool Parser::Process(int argc, char* argv[])
{
	//初期化
	OutText("パーサのセットアップを開始", OS_INFO);
	if (Init(argv[0])) {
		OutText("パーサのセットアップが完了", OS_INFO);
	}
	else {
		OutText("セットアップ時に問題が発生しました。", OS_ERROR);
		return false;
	}

	//ファイル選択
	if (!StoreParseFile(argc, argv, &parsePaths))return false;

	//読込+処理
	for (int i = 0; i < parsePaths.size(); i++) {
		OutText(parsePaths[i] + " を変換しています...(" + to_string(i + 1) + "/" + to_string(parsePaths.size()) + ")", OS_INFO);

		//読込
		if (!Read(parsePaths[i], &data))continue;

		//処理
		Parse(parsePaths[i], data);
	}
	//読込+処理

	Result();
	return 0;
}

bool Parser::StoreParseFile(int argc, char* argv[], vector<string>* paths)
{
	// 引数が1つ以上ある場合（0番目は実行ファイル自身のパス）
	if (argc > 1) {
		OutText("D&Dされたファイルを読み込みます。", OS_INFO);
		OutText("読込データ：");
		for (int i = 1; i < argc; i++) {
			paths->push_back(argv[i]);
			OutText("  [" + to_string(i - 1) + "] : " + argv[i]);
		}
	}
	//引数がない場合(argcが1以下)
	else {
		OutText("変換するファイルを選択してください。", OS_INFO);
		SelectFile(paths);
		if (paths->size() == 0) {
			OutText("ファイルが選択されませんでした。終了します。", OS_INFO);
			return false;
		}
		else {
			OutText("読込データ：");
			for (int i = 0; i < paths->size(); i++) {
				OutText("  [" + to_string(i) + "] : " + (*paths)[i]);
			}
		}
	}
	return true;
}

void Parser::SelectFile(vector<string>* paths)
{
	char fileName[MAX_PATH] = "";  //ファイル名を入れる変数

	//「ファイルを開く」ダイアログの設定
	OPENFILENAME ofn;                         	//名前をつけて保存ダイアログの設定用構造体
	ZeroMemory(&ofn, sizeof(ofn));            	//構造体初期化
	ofn.lStructSize = sizeof(OPENFILENAME);   	//構造体のサイズ
	ofn.lpstrFilter = TEXT("マップデータ(*.json)\0*.json\0")			//─┬ファイルの種類
		TEXT("すべてのファイル(*.*)\0*.*\0\0");                     //─┘
	ofn.lpstrFile = fileName;               	//ファイル名
	ofn.nMaxFile = MAX_PATH;               	//パスの最大文字数
	//ofn.Flags = OFN_OVERWRITEPROMPT;   		//フラグ（同名ファイルが存在したら上書き確認）
	ofn.lpstrDefExt = "json";                  	//デフォルト拡張子

	//「ファイルを開く」ダイアログ
	BOOL selFile;
	selFile = GetOpenFileName(&ofn);

	//キャンセルしたら中断
	if (selFile == FALSE) return;

	//複数取る方法わからんので今は1個だけプッシュ
	paths->push_back(fileName);

}

bool Parser::Read(string _path, json* data)
{
	std::filesystem::path path = _path;
	path.replace_extension(".json");

	//json読込
	std::ifstream f(path);
	//std::ifstream f(path.string());

	try { *data = json::parse(f); }
	catch (json::parse_error e) {
		OutText("マップデータの読込に失敗しました：" + (string)(e.what()), OS_ERROR);
		return false;
	}

	return true;
}

void Parser::Parse(string _path, json _data)
{
	//WHを持ってくる
	int height = _data["height"];
	int width = _data["width"];

	ofstream output(exeDir + "\\output\\" + filesystem::path(_path).stem().string() + "_output.txt");
	for (int layer = 0; layer < _data["layers"].size(); layer++) {
		output << "Begin Object Class=/Script/Paper2D.PaperTileLayer Name=\"\"\n";
		output << "   LayerName=NSLOCTEXT(\"\", \"\", " << _data["layers"][layer]["name"] << ")\n";
		output << "   LayerWidth=" << width << "\n";
		output << "   LayerHeight=" << height << "\n";
		output << "   AllocatedWidth=" << width << "\n";
		output << "   AllocatedHeight=" << height << "\n";

		//ここにマップ変換機構
		for (int h = 0; h < height; h++) {
			for (int w = 0; w < width; w++) {
				int index = h * width + w;	//インデックス設定
				unsigned int tiledValue = _data["layers"][layer]["data"][index];	//TILEDのデータ値
				//最後の値のときのみ、空の場合も生成
				//値が0のときは何もないのでスキップ
				if (tiledValue != 0) {
					//データ変換関数
					string ueTileset = "";
					int uePackedTileIndex = -1;
					ConvertData(tiledValue, &ueTileset, &uePackedTileIndex);

					//変換失敗時に出る値が帰ってきたとき処理終了
					if (ueTileset != "" && uePackedTileIndex != -1) {
						output << "   AllocatedCells(" << index << ")=(TileSet=PaperTileSet'\"" << ueTileset << "\"',PackedTileIndex=" << uePackedTileIndex << ")\n";
					}
					else {
						OutText("変換に失敗しました。(" + to_string(index) + ")", OS_ERROR);
						return;
					}
				}
				else if (index == height * width - 1) {
					output << "   AllocatedCells(" << index << ")=()\n";
				}
			}
		}
		output << "End Object\n\n";
	}

}

bool Parser::End() {

	std::cout << "何かキーを押してください..." << std::endl;
	_getch(); // キー入力を待機（入力内容は表示されない）
	return 0;
}
void Parser::Result() {
	cout << endl << "処理が終了しました。" << endl;
	End();
}

//ここの処理をかいておわり 多分そうでもないけど
void Parser::ConvertData(unsigned int tiledValue, string* uePath, int* ueTileValue) {
	/*
	Tiled Format -> UE4 Format
	unsigned int -> singed int
	* 32bit:Y反転(垂直反転)
	* 31bit:X反転(水平反転)
	* 30bit:斜反転
			0  90 180         270
	 時計 000 101 110(XY反転) 011
	X反転 100 111 010(Y反転)  001

	*/
	int gid = tiledValue & (int)(pow(2, 29) - 1);	//29ビット目までとる
	//int flipFlag = (tiledValue >> 29) & 0b111;		//29ビットシフトして3ビット取る(30-32)

	//unsigned int tmp = tiledValue;
	for (int tilesetID = data["tilesets"].size() - 1; tilesetID >= 0; tilesetID--) {
		if (gid >= data["tilesets"][tilesetID]["firstgid"]) {

			//*ueTileValue = gid - data["tilesets"][tilesetID]["firstgid"];
			*uePath = linkPathData[data["tilesets"][tilesetID]["source"]]["ue4"];
			*ueTileValue = tiledValue - data["tilesets"][tilesetID]["firstgid"];

			//cout << "CONVERT " << tiledValue << " -> " << *ueTileValue << endl;
			break;
		}
	}
}

void Parser::OutText(string str, OUTPUT_STATE outState) {
	switch (outState)
	{
	case OS_NONE:		cout << " ";						break;
	case OS_INFO:		cout << "[INFO] ";					break;
	case OS_WARNING:	cout << "\033[33m" << "[WARNING] ";	break;
	case OS_ERROR:		cout << "\033[31m" << "[ERROR] ";	break;
	}

	cout << str << "\033[0m" << endl;
}