#define _CRT_SECURE_NO_WARNINGS
#include "Parser.h"

#include <iostream>
#include <cmath>
#include <regex>

#include <d3d11.h>

using namespace std;

Parser::Parser()
{
}

bool Parser::Init(char* exePath) {
	//実行ファイルのディレクトリを取得
	std::filesystem::path path = exePath;
	parentDir = path.parent_path().string();
	outDir = parentDir + "\\output\\";

	//TiledとUE4のリンクファイルを読込
	string lpPath = parentDir + "\\linkPath.json";
	if (filesystem::exists(lpPath)) {
		std::ifstream lp(lpPath);

		try {
			linkPathJson = json::parse(lp);
		}
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
}

bool Parser::Process(int argc, char* argv[])
{
	//初期化
	OutText("パーサのセットアップを開始", OS_INFO);
	if (Init(argv[0])) {
		OutText("パーサのセットアップが完了", OS_INFO);
	}
	else {
		OutText("セットアップ時に問題が発生しました。処理を終了します。", OS_NONE);
		End();
		return false;
	}

	//ファイル選択
	if (!StoreParseFile(argc, argv, &parsePaths)) {
		OutText("ファイルが選択されませんでした。処理を終了します。", OS_NONE);
		End();
		return false;
	}

	//読込+処理
	for (int i = 0; i < parsePaths.size(); i++) {
		Br();
		OutText(parsePaths[i] + " を変換しています...(" + to_string(i + 1) + "/" + to_string(parsePaths.size()) + ")", OS_INFO);
		Br();

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
		SelectFile(paths, STR_FILTER{ "マップデータ", "json" }, PGetCurrentDirectory());
		if (paths->size() == 0) {
			//OutText("ファイルが選択されませんでした。終了します。", OS_INFO);
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


void Parser::SelectFile(vector<string>* storePaths, vector<STR_FILTER> filters, string dir, bool enAllFile)
{
	*storePaths = SelectFile_proc(filters, dir, enAllFile, false);
}

void Parser::SelectFile(vector<string>* storePaths, STR_FILTER filter, string dir, bool enAllFile)
{
	vector<STR_FILTER> inFilter;
	inFilter.push_back(filter);

	*storePaths = SelectFile_proc(inFilter, dir, enAllFile, false);
}

void Parser::SelectFile(string* storePath, vector<STR_FILTER> filters, string dir, bool enAllFile)
{
	vector<string> retStr = SelectFile_proc(filters, dir, enAllFile, true);

	if (retStr.size() > 0)*storePath = retStr[0];
}

void Parser::SelectFile(string* storePath, STR_FILTER filter, string dir, bool enAllFile)
{
	vector<STR_FILTER> inFilter;
	inFilter.push_back(filter);

	vector<string> retStr = SelectFile_proc(inFilter, dir, enAllFile, true);

	if (retStr.size() > 0)*storePath = retStr[0];
}

vector<string> Parser::SelectFile_proc(vector<STR_FILTER> filters, string dir, bool enAllFile, bool isSingleFile)
{
	string currentDir = PGetCurrentDirectory();

	//指定ディレクトリが空白ではない場合のみカレントディレクトリを更新
	if (dir != "") {
		SetCurrentDirectory(dir.c_str());
	}

	char fileName[MAX_PATH] = "";  //ファイル名を入れる変数

	vector<char> filterArr;

	auto AddFilter = [=](vector<char>* flt, STR_FILTER& addFlt) {
		string overview = addFlt.descr + "(*." + addFlt.ext + ")";
		string ext = "*." + addFlt.ext;

		flt->insert(flt->end(), overview.begin(), overview.end());
		flt->push_back('\0');
		flt->insert(flt->end(), ext.begin(), ext.end());
		flt->push_back('\0');
		};

	//指定フィルタ追加
	for (auto& flt : filters) {
		AddFilter(&filterArr, flt);
	}

	//全ファイルフィルタの適用関連
	if (enAllFile) {
		//全ファイルフィルタ追加
		STR_FILTER filterAll = { "すべてのファイル" , "*" };
		AddFilter(&filterArr, filterAll);
	}

	// フィルタ文字列全体を NULL 終端する
	filterArr.push_back('\0');

	//「ファイルを開く」ダイアログの設定
	OPENFILENAME ofn;                         	//名前をつけて保存ダイアログの設定用構造体
	ZeroMemory(&ofn, sizeof(ofn));            	//構造体初期化
	ofn.lStructSize = sizeof(OPENFILENAME);   	//構造体のサイズ
	ofn.lpstrFilter = filterArr.data();
	ofn.lpstrFile = fileName;               	//ファイル名
	ofn.nMaxFile = MAX_PATH;               	//パスの最大文字数
	if (!isSingleFile)	ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER;	//これで複数選択ができそう
	ofn.lpstrDefExt = "json";                  	//デフォルト拡張子

	//「ファイルを開く」ダイアログ
	BOOL selFile;
	selFile = GetOpenFileNameA(&ofn);

	//キャンセルしたら中断
	if (selFile == FALSE) return vector<string>();

	// 複数ファイルが選択された場合
	std::vector<std::string> selectedFiles;
	std::string directory(fileName);
	TCHAR* ptr = fileName + directory.length() + 1;

	if (*ptr == '\0')	selectedFiles.push_back(directory);	//単一ファイルの場合
	else {
		//複数ファイルの場合
		while (*ptr) {
			selectedFiles.push_back(directory + "\\" + std::string(ptr));
			ptr += strlen(ptr) + 1;
		}
	}

	// 選択されたファイルを出力
	for (const auto& file : selectedFiles) std::cout << "Selected File: " << file << std::endl;

	//カレントディレクトリをもとにもどす
	SetCurrentDirectory(currentDir.c_str());

	//複数取る方法わからんので今は1個だけプッシュ
	//260文字超えて失敗したときの処理書いてないけど全部返す
	return selectedFiles;

}

bool Parser::Read(string _path, json* _data)
{
	//初期化
	useLinkDataIndexes.clear();

	json& linkData = linkPathJson["linkData"];
	//if (1/*Unicodeがあるか*/) {
	//	//データコピー
	//	string tmpPath = outDir + "uniTemp.tmp";
	//	ofstream tmp(tmpPath);
	//	tmp << 
	//	//std::filesystem::path path = _path;
	//	//path.replace_extension(".json");
	//}
	//else {
		//json読込
		filesystem::path inPath = _path;
		std::ifstream f(_path);
		try { *_data = json::parse(f); }
		catch (json::parse_error e) {
			OutText("マップデータの読込に失敗しました：" + (string)(e.what()), OS_ERROR);
			return false;
		}

	//}
	
	
	bool isLinked = false;
	//使用したタイルセットがUEパスと紐付けられているか
	for (json& source : (*_data)["tilesets"]) {
		string sourcePath = source["source"];
		isLinked = false;	//一旦リンクされてない判定にする

		for (int index = 0; index < linkData.size(); index++) {
			string linkPath = linkData[index]["tiled"];
			
			if (sourcePath == linkPath) {
				//存在した場合
				useLinkDataIndexes.push_back(index);	//json内の使用タイルセットインデックス(0から順にループしているためpush_backでOK)とlinkPathのインデックスを紐付け
				isLinked = true;
				break;
			}
		}
		//存在しなかった場合
		if (!isLinked) {
			OutText("使用タイルセット " + sourcePath + " がリンクされていません。", OS_ERROR);

			vector<int> sameFileNameIndexes;
			//まずは使用タイルセットのファイル名と同じファイル名がリンクファイルに登録されているか判定
			string sourceStem = GetStem(sourcePath);
			for (int index = 0; index < linkData.size(); index++) {
				string linkPath = linkData[index]["tiled"];
				if (GetStem(linkPath) == sourceStem) {
					sameFileNameIndexes.push_back(index);
				}
			}
			//あった場合、UEのパスと表示する
			if (sameFileNameIndexes.size() > 0) {
				OutText("リンクファイルからパスのみが異なる同名のリンクデータが見つかりました。同じUEアセットをリンクする場合、対応する番号を入力してください。\n", OS_INFO);
				OutText("  [" + to_string(0) + "] : 選択しない");
				for (int i = 0; i < sameFileNameIndexes.size(); i++) {
					//if (i > 6) {
					//	pages++;		//8以上の重複ファイルに対応するにはここらへんをいじる
					//}
					OutText("  [" + to_string(i+1) + "] : " + 
						to_string(linkData[sameFileNameIndexes[i]]["tiled"]) + " <-> " +
						to_string(linkData[sameFileNameIndexes[i]]["ue4"])
					);
				}
				char inch = 0x00;
				while (inch < '0' || inch > '9') {
					inch = GetKey("番号を入力：");
					if (inch == '0')break;
					if (inch >= '1' && inch <= '7') {

						//番号内ならば処理する、番号外ならもう一回
						if ((inch - '0') <= sameFileNameIndexes.size()) {
							//リンク処理
							OutText(sourcePath + " と " + to_string(linkData[sameFileNameIndexes[inch - '1']]["ue4"]) + "をリンクします。", OS_INFO);
							json value;
							value += json::object_t::value_type("tiled", sourcePath);
							value += json::object_t::value_type("ue4", linkData[sameFileNameIndexes[inch - '1']]["ue4"]);
							try {
								linkData.push_back(value);
								OutputJson(parentDir + "\\linkPath.json", linkPathJson);
							}
							catch (json::type_error e) {
								OutText("保存時にエラーが発生しました(" + (string)e.what(), OS_ERROR);
							}
							break;
						}
					}
					//if (inch == '8') {
					//	if (nowPage > 0)nowPage--;
					//	PrintPage(nowPage);
					//}
					//if (inch == '9') {
					//	if (nowPage < maxPage)nowPage++;
					//	PrintPage(nowPage);
					//}
				}
			}
			//ない場合、ほかの処理を選択させる

			OutText("未リンクのタイルセット " + to_string(source["source"]) + " に対する処理を選択してください。", OS_INFO);
			vector<string> noLinkedProcStrArr = {
				"読込処理を中止",
				"リンク済リストから選択してリンク",
				"エクスプローラからuassetファイルを選択してリンク",
				"UE4内でコピペしたアセットパスを直接記入してリンク"
			};
			PrintStrList(&noLinkedProcStrArr);
			bool endFlag = false;
			//string in;
			while (!endFlag) {
				char in;
				while (true) {
					in = GetKey("  処理番号を入力:");

					if (in < '0' || (in-'0') >= noLinkedProcStrArr.size()) {
						//cout << "無効な処理番号です。" << endl;
					}
					else break;
				}

				string inputAsset = "";
				vector<string> vstr;
				switch (in)
				{
				case '0':	//中止
					OutText("このマップデータの読込を中止しました。", OS_INFO);
					return false;
					break;
				case '1':	//リンク済リストから選択
					OutText("以下のリストにある場合、左のインデックスを入力してください。");
					for (int index = 0; index < linkData.size(); index++) {
						OutText(index + " -> " + to_string(linkData[index]["ue4"]), OS_NONE);
					}
					break;
				case '2':	//エクスプローラ選択
					//エクスプローラの設定
					SelectFile(&vstr, STR_FILTER{ "uassetファイル", "uasset" }, linkPathJson["projectPath"]);
					//最初はiniファイルのディレクトリを指定
					//バイナリからタイルセットかどうかを確認する(タイルセットじゃなかったら警告を入れる)
					//リンクデータを追加する

					break;
				case '3':	//パス入力
					OutText("UE4のコンテンツブラウザでコピーしたアセットパスを入力してください。", OS_INFO);
					//SetConsoleOutputCP(1252);
					//SetConsoleCP(1252);
					struct IMPORT_DATA_ATTR {
						int isPaperTileSet = -1;	//-1=不明 0=不 1=正
						bool exists = false;
						bool isUEPath = false;		//UEパスか(/Gameからのパスになっているか)
					};
					while (inputAsset != "-1") {
						//入力
						string s;
						getline(cin, s);
						bool isAbsolutePath = false;
						bool existsFile = false;
						/*
						想定される入力
						UE4コピペ
						PaperTileSet'/Game/Assets/tilemap/tileset_field_TileSet.tileset_field_TileSet'
						/Game/Assets/tilemap/tileset_field_TileSet.tileset_field_TileSet
						"D:\GE3A09\Unreal Projects\UE2D\Content\Assets\tilemap\tileset_field_TileSet.uasset"
						D:\GE3A09\Unreal Projects\UE2D\Content\Assets\tilemap\tileset_field_TileSet.uasset
						それ以外は対応しません　無理
						*/
						IMPORT_DATA_ATTR attr;
						if (Like(inputAsset, "%/Game/%")) {
							//UEパスのとき
							OutText("読込形式: UE Path");
							attr.isUEPath = true;
							//アセットの形式を読みとる(未実装)
							//attr.isPaperTileSet = ExtractImportData(UEDirectory + "\\")
						}
						else if(Like(inputAsset, "%:\\%.uasset%")){
							//絶対パスのとき
							OutText("読込形式: Absolute Path");

							attr.isPaperTileSet = -1;	//タイルセットかは不明(未実装)
							attr.isUEPath = false;		//UEPathではない
							attr.exists = filesystem::exists(inputAsset);	//ファイルが存在するか

						}

						//if (attr.exists) {
						//	if(attr.isUEPath)
						//}


						//そのパスが存在するか(なかったら警告)
						if(filesystem::exists(linkPathJson["projectPath"] + "\\Content\\"))
						//正常に入力したとき								PaperTileSet'/Game/Assets/tilemap/tileset_field_TileSet.tileset_field_TileSet'	
						//入力内容が絶対パスだったとき
						  //そのパスは現在のプロジェクトか？(なければ警告)
						  
						//パスのうち、''で囲まれた部分だけ入力してきたとき
						//入力したが、タイルセットではないパスだった場合(警告)
						//
						//cin.getline(inputAsset, sizeof(inputAsset));
						u16string u16str;
						string inStr;
						//getline(cin, inStr);

						typedef std::basic_ofstream<char16_t> u16ofstream;
						//u16ofstream outfile("testtttst.txt", std::ios_base::app);
						//outfile << u16str;
						//outfile.close();
						
						ios_base::sync_with_stdio(false);
						wcin.imbue(locale("en_US.UTF-8"));
						wcout.imbue(locale("en_US.UTF-8"));

						cout << s << endl;

						//wofstream outfile("testtttst.txt");
						ofstream outfile("testtttst.txt");
						outfile << s;
						outfile.close();


						//cin >> inputAsset;
						//cout << inputAsset << endl;
					}
					//ExtractTexture(inputAsset);
					//OutText(sourcePath + " と " + to_string(linkPathData[sameFileNameIndexes[inch - '1']]["ue4"]) + "をリンクします。", OS_INFO);
				}

			}

		}
	}
	
	return true;
}

void Parser::Parse(string _path, json _data)
{
	//WHを持ってくる
	int height = _data["height"];
	int width = _data["width"];

	string outFile = outDir + filesystem::path(_path).stem().string();
	if (filesystem::exists(outFile + "_output.txt") || filesystem::exists(outFile + ".json")) {
		OutText("出力フォルダ内に選択したファイルと同じ名前のファイルが存在します。ナンバリングを行います。", OS_WARNING);
		time_t now = time(nullptr);
		auto nowM = localtime(&now);
		ostringstream oss;
		oss << put_time(localtime(&now), "_%Y%m%d%H%M%S");
		outFile += oss.str();
		cout << outFile << endl;
	}
	ofstream output(outFile + "_output.txt");
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

				//値が0のときは何もないのでスキップ
				//(最後の値のときのみ、空の場合でも生成)
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
	OutputJson(outFile + ".json",
		{
	{"height", _data["height"]},
	{"width", _data["width"]},
	{"orientation", _data["orientation"]},
	{"tileheight", _data["tileheight"]},
	{"tilewidth", _data["tilewidth"]},
	{"version", _data["version"]}
		}
	);
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
	json& linkData = linkPathJson["linkData"];
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
	try {
		//タイルセットID(json内タイルセットリストのインデックス)をIDデカいほうから探索
		for (int tilesetID = data["tilesets"].size() - 1; tilesetID >= 0; tilesetID--) {
			//タイルセットリストに格納されたグローバルIDと比較、それよりも現在タイルの値が大きければUEパスとUEタイル値を格納
			if (gid >= data["tilesets"][tilesetID]["firstgid"]) {
				*uePath = linkData[useLinkDataIndexes[tilesetID]]["ue4"];
				*ueTileValue = tiledValue - data["tilesets"][tilesetID]["firstgid"];

				break;
			}
		}
	}
	catch (json::type_error &e) {
		OutText("マップデータまたはリンクファイルに必要となるキーが存在しないか、不正な値が入っています。変換を中止します。", OS_ERROR);
		return;
	}
}

string Parser::GetStem(string path)
{
	string ret = filesystem::path(path).stem().string();
	//cout << "ステムを取る：" << path << " -> " << ret << endl;
	return ret;
}

char Parser::GetKey(string descr)
{
	std::cout << descr;
	int inch = _getch();
	if (isgraph(inch)) {
		cout << (char)inch << endl;
		return (char)inch;
	}
	else {
		cout << "Invalid Input" << endl;
	}
	return 0x00;
}

bool Parser::InputBool(string descr)
{
	std::cout << descr << "[Y/N]:";
	while (true) {
		int inch = toupper(_getch());
		if (In(inch, { 'Y','N' })) {
			cout << (char)inch << endl;
			return (inch == 'Y' ? true : false);
			//if (inch == 'Y')return true;
			//return false;
		}
	}
	return false;
}

void Parser::OutputJson(string filePath, json content)
{
	if (((filesystem::path)(filePath)).extension() == ".json") OutText("JSON形式で出力されるファイルの拡張子が.jsonではありません。使用する際はご注意ください。", OS_WARNING);
	ofstream out(filePath);
	out << content.dump(2);
	out.close();
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

bool Parser::BreakNIsContinue(string warnStr)
{
	OutText(warnStr, OS_WARNING);
	return InputBool("続行しますか？");
}

void Parser::PrintStrList(vector<string>* descrList, int startNum)
{
	for (int i = 0; i < descrList->size(); i++) {
		OutText("  [" + to_string(i + startNum) + "] : " +
			(*descrList)[i]
		);
	}
}

bool Parser::Like(string val, string search)
{
	//正規表現変換
	string regexPattern;
	for (char c : search) {
		switch (c) {
		case '%': // 任意の文字列に対応
			regexPattern += ".*";
			break;
		case '_': // 任意の1文字に対応
			regexPattern += ".";
			break;
		case '.': // '.'は正規表現で特殊文字なのでエスケープ
		case '^':
		case '$':
		case '|':
		case '(':
		case ')':
		case '[':
		case ']':
		case '*':
		case '+':
		case '?':
		case '\\': // '\'もエスケープが必要
			regexPattern += '\\';
			[[fallthrough]];
		default: // それ以外の文字
			regexPattern += c;
			break;
		}
	}

	//正規表現オブジェクト作成(icaseで大文字小文字を無視)
	std::regex re(regexPattern, std::regex::icase);

	//判定
	return std::regex_match(val, re);
}

string Parser::toBinary(unsigned int n)
{
	std::string r;
	while (n != 0) {
		r = (n % 2 == 0 ? "0" : "1") + r;
		n /= 2;
	}
	return r;
}

string Parser::PGetCurrentDirectory()
{
	char dir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, dir);
	return dir;
}

/*
TODO
パスが見つからないときにエラー吐いて強制終了するバグ
複数選択(非D&D時)

issue
同名のリンクデータが8つ以上あったとき、8以降のデータが表示されない(8と9にpageUp/Downを割り当ててるため)


memo

250108 LIKEを実装
*/