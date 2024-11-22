#include "Parser.h"

#include <iostream>
#include <cmath>
#include <filesystem>

using namespace std;

Parser::Parser()
{
}

bool Parser::Init(int argc, char* argv[])
{
	//ファイル指定
	string filePath = "";
	// 引数が1つ以上ある場合（0番目は実行ファイル自身のパス）
	if (argc > 1) {
		if (argc > 2) {
			OutText("2つ以上のファイルをドロップしています。最初に認識したファイルを自動的に変換対象にしました。", OS_WARNING);
		}
		filePath = argv[1];
		OutText("ファイル読込:" + filePath, OS_INFO);
	}
	else {
		OutText("ファイルが選択されていません。変換するファイルを選択してください。", OS_INFO);
		SelectFile(&filePath);
		OutText("conv:" + filePath);
		if (filePath == "") {
			OutText("ファイルが選択されませんでした。終了します。", OS_INFO);
			return End();
		}
	}

	//json読込
	std::ifstream f(filePath);
	//自身のディレクトリと組み合わせないとエラる
	std::filesystem::path path(argv[0]);
	string lpPath = path.parent_path().string() + "\\linkPath.json";
	cout << lpPath << endl;
	std::ifstream lp(lpPath);
	try {
		data = json::parse(f);
	}
	catch (json::parse_error e) { OutText("マップデータの読み込みに失敗しました。(" + (string)(e.what()), OS_ERROR); }
	try {
		linkPathData = json::parse(lp);
	}
	catch (json::parse_error e) { cout << e.what() << endl; }
}

bool Parser::Process()
{
	//WHを持ってくる
	int height = data["height"];
	int width = data["width"];

	ofstream output("test.txt");
	for (int layer = 0; layer < data["layers"].size(); layer++) {
		output << "Begin Object Class=/Script/Paper2D.PaperTileLayer Name=\"\"\n";
		output << "   LayerName=NSLOCTEXT(\"\", \"\", " << data["layers"][layer]["name"] << ")\n";
		output << "   LayerWidth=" << width << "\n";
		output << "   LayerHeight=" << height << "\n";
		output << "   AllocatedWidth=" << width << "\n";
		output << "   AllocatedHeight=" << height << "\n";

		//ここにマップ変換機構
		for (int h = 0; h < height; h++) {
			for (int w = 0; w < width; w++) {
				int index = h * width + w;	//インデックス設定
				unsigned int tiledValue = data["layers"][layer]["data"][index];	//TILEDのデータ値
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
						return End();
					}
				}
			}
		}
		output << "End Object\n\n";
	}
	//Result();
	return 0;
}

bool Parser::SelectFile(string* path)
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
	if (selFile == FALSE) return false;

	*path = fileName;

}

bool Parser::End() {

	std::cout << "何かキーを押してください..." << std::endl;
	_getch(); // キー入力を待機（入力内容は表示されない）
	return 0;
}
//void Parser::Result() {
//	cout << "けっか" << endl;
//	End();
//}

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
			break;
		}
	}

	////値が2^29以上(２進数で30ビット以上)のとき、UE4のフォーマットに変換した値を加算
	//if (flipFlag > 0) {
	//	if (flipFlag & 1) {
	//		*ueTileValue += pow(2, 29);
	//		//斜反転フラグ
	//	}
	//	if ((flipFlag >> 1) & 1) {
	//		*ueTileValue += pow(2, 30);
	//		//上下回転フラグ
	//	}
	//	if ((flipFlag >> 2) & 1) {
	//		*ueTileValue -= pow(2, 31);
	//		//左右反転フラグ
	//	}
	//}
	//cout << bit_cast<int>(tmp) << " | " << *ueTileValue << endl;
}

void Parser::OutText(string str, OUTPUT_STATE outState) {
	switch (outState)
	{
	case OS_INFO:		cout << "[INFO]";					break;
	case OS_WARNING:	cout << "\033[33m" << "[WARNING]";	break;
	case OS_ERROR:		cout << "\033[31m" << "[ERROR]";	break;
	}

	cout << str << "\033[0m" << endl;
}