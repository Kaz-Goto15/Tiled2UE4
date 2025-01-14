#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include "Parser.h"

#include <iostream>
#include <cmath>
#include <regex>

#include <d3d11.h>

//#include "Unicode.h"
#include <fstream>
#include <codecvt>

using namespace std;

Parser::Parser()
{
}

bool Parser::Init(wchar_t* exePath) {
	//実行ファイルのディレクトリを取得
	std::filesystem::path path = exePath;
	parentDir = path.parent_path().wstring();
	outDir = parentDir + L"\\output\\";

	//TiledとUE4のリンクファイルを読込
	wstring lpPath = parentDir + L"\\linkPath.json";
	if (filesystem::exists(lpPath)) {
		std::ifstream lp(lpPath);

		try {
			linkPathJson = json::parse(lp);
		}
		catch (json::parse_error e) {
			OutText(L"リンクパスファイルの読込時に問題が発生しました：" + (wchar_t)e.what(), OS_ERROR);
			return false;
		}
	}
	else {
		OutText(L"リンクパスファイルが見つかりません。", OS_ERROR);
		return 0;
	}

	if (!filesystem::is_directory(outDir)) {
		OutText(L"出力フォルダがありません。ディレクトリを自動追加します。", OS_WARNING);
		if (!filesystem::create_directory(outDir)) {
			OutText(L"出力フォルダの作成に失敗しました。管理者権限が必要な場所で実行しているか、容量が足りない可能性があります。", OS_ERROR);
			return 0;
		};
	}
}

bool Parser::Process(int argc, wchar_t* argv[])
{
	//初期化
	OutText(L"パーサのセットアップを開始", OS_INFO);
	if (Init(argv[0])) {
		OutText(L"パーサのセットアップが完了", OS_INFO);
	}
	else {
		OutText(L"セットアップ時に問題が発生しました。処理を終了します。", OS_NONE);
		End();
		return false;
	}

	//ファイル選択
	if (!StoreParseFile(argc, argv, &parsePaths)) {
		OutText(L"ファイルが選択されませんでした。処理を終了します。", OS_NONE);
		End();
		return false;
	}

	//読込+処理
	for (int i = 0; i < parsePaths.size(); i++) {
		Br();
		OutText(parsePaths[i] + L" を変換しています...(" + to_wstring(i + 1) + L"/" + to_wstring(parsePaths.size()) + L")", OS_INFO);
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

bool Parser::StoreParseFile(int argc, wchar_t* argv[], vector<wstring>* paths)
{
	// 引数が1つ以上ある場合（0番目は実行ファイル自身のパス）
	if (argc > 1) {
		OutText(L"D&Dされたファイルを読み込みます。", OS_INFO);
		OutText(L"読込データ：");
		for (int i = 1; i < argc; i++) {
			paths->push_back(argv[i]);
			OutText(L"  [" + to_wstring(i - 1) + L"] : " + argv[i]);
			wcout << L"うお" << endl;
		}
	}
	//引数がない場合(argcが1以下)
	else {
		OutText(L"変換するファイルを選択してください。", OS_INFO);
		SelectFile(paths, STR_FILTER{ "マップデータ", "json" }, PGetCurrentDirectoryW());
		if (paths->size() == 0) {
			//OutText(L"ファイルが選択されませんでした。終了します。", OS_INFO);
			return false;
		}
		else {
			OutText(L"読込データ：");
			for (int i = 0; i < paths->size(); i++) {
				OutText(L"  [" + to_wstring(i) + L"] : " + (*paths)[i]);
			}
		}
	}
	return true;
}


void Parser::SelectFile(vector<wstring>* storePaths, vector<STR_FILTER> filters, wstring dir, bool enAllFile)
{
	*storePaths = SelectFile_proc(filters, dir, enAllFile, false);
}

void Parser::SelectFile(vector<wstring>* storePaths, STR_FILTER filter, wstring dir, bool enAllFile)
{
	vector<STR_FILTER> inFilter;
	inFilter.push_back(filter);

	*storePaths = SelectFile_proc(inFilter, dir, enAllFile, false);
}

void Parser::SelectFile(wstring* storePath, vector<STR_FILTER> filters, wstring dir, bool enAllFile)
{
	vector<wstring> retStr = SelectFile_proc(filters, dir, enAllFile, true);

	if (retStr.size() > 0)*storePath = retStr[0];
}

void Parser::SelectFile(wstring* storePath, STR_FILTER filter, wstring dir, bool enAllFile)
{
	vector<STR_FILTER> inFilter;
	inFilter.push_back(filter);

	vector<wstring> retStr = SelectFile_proc(inFilter, dir, enAllFile, true);

	if (retStr.size() > 0)*storePath = retStr[0];
}

vector<wstring> Parser::SelectFile_proc(vector<STR_FILTER> filters, wstring dir, bool enAllFile, bool isSingleFile)
{
	wstring currentDir = PGetCurrentDirectoryW();
	
	//指定ディレクトリが存在しないとき、カレントディレクトリになるよう指定
	if (!filesystem::exists(dir)) 	dir = currentDir;

	wchar_t fileName[MAX_PATH] = L"";  //ファイル名を入れる変数

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
	OPENFILENAMEW ofnw;
	ZeroMemory(&ofnw, sizeof(ofnw));            	//構造体初期化
	ofnw.lStructSize = sizeof(OPENFILENAMEW);   	//構造体のサイズ

	// 終端文字を考慮してデータをコピー
	std::string utf8Str(filterArr.begin(), filterArr.end());
	// UTF-8 から UTF-16 に変換
	int wideSize = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
	std::wstring filterWStr(wideSize, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &filterWStr[0], wideSize);

	// 最後の終端文字を削除
	filterWStr.pop_back();

	//wstring filterWStr;
	ofnw.lpstrFilter = filterWStr.c_str();
	ofnw.lpstrFile = fileName;               	//ファイル名
	ofnw.nMaxFile = MAX_PATH;               	//パスの最大文字数
	if (!isSingleFile)	ofnw.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER;	//これで複数選択ができそう
	ofnw.lpstrDefExt = L"json";                  	//デフォルト拡張子
	ofnw.lpstrInitialDir = dir.c_str();
	//「ファイルを開く」ダイアログ
	BOOL selFile;
	selFile = GetOpenFileNameW(&ofnw);

	//OPENFILENAME ofn;                         	//名前をつけて保存ダイアログの設定用構造体
	//ZeroMemory(&ofn, sizeof(ofn));            	//構造体初期化
	//ofn.lStructSize = sizeof(OPENFILENAME);   	//構造体のサイズ
	//ofn.lpstrFilter = (LPCWSTR)filterArr.data();
	//ofn.lpstrFile = (LPWSTR)fileName;               	//ファイル名
	//ofn.nMaxFile = MAX_PATH;               	//パスの最大文字数
	//if (!isSingleFile)	ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER;	//これで複数選択ができそう
	//ofn.lpstrDefExt = L"json";                  	//デフォルト拡張子
	//ofn.lpstrInitialDir = (LPCWSTR)dir.c_str();
	////「ファイルを開く」ダイアログ
	//BOOL selFile;
	//selFile = GetOpenFileNameW(&ofn);

	//キャンセルしたら中断
	if (selFile == FALSE) return vector<wstring>();

	// 複数ファイルが選択された場合
	std::vector<std::wstring> selectedFiles;
	std::wstring directory(fileName);
	TCHAR* ptr = (TCHAR*)fileName + directory.length() + 1;

	if (*ptr == '\0')	selectedFiles.push_back(directory);	//単一ファイルの場合
	else {
		//複数ファイルの場合
		while (*ptr) {
			wstring s = ptr;
			selectedFiles.push_back(directory + L"\\" + wstring(ptr));
			ptr += wcslen(ptr) + 1;
		}
	}

	// 選択されたファイルを出力
	for (const auto& file : selectedFiles) std::wcout << "Selected File: " << file << std::endl;

	//カレントディレクトリをもとにもどす
	SetCurrentDirectory((LPCWSTR)currentDir.c_str());

	//複数取る方法わからんので今は1個だけプッシュ
	//260文字超えて失敗したときの処理書いてないけど全部返す
	return selectedFiles;

}

bool Parser::Read(wstring _path, json* _data)
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
			OutText(L"マップデータの読込に失敗しました：" + (wchar_t)(e.what()), OS_ERROR);
			return false;
		}

	//}
	
	
	bool isLinked = false;
	//使用したタイルセットがUEパスと紐付けられているか
	for (json& source : (*_data)["tilesets"]) {
		wstring sourcePath;
		StoreWStr(&sourcePath, &source["source"]);
		isLinked = false;	//一旦リンクされてない判定にする

		for (int index = 0; index < linkData.size(); index++) {
			wstring linkPath;

			StoreWStr(&linkPath, &linkData[index]["tiled"]);
			if (sourcePath == linkPath) {
				//存在した場合
				useLinkDataIndexes.push_back(index);	//json内の使用タイルセットインデックス(0から順にループしているためpush_backでOK)とlinkPathのインデックスを紐付け
				isLinked = true;
				break;
			}
		}
		//存在しなかった場合
		if (!isLinked) {
			OutText(L"使用タイルセット " + sourcePath + L" がリンクされていません。", OS_ERROR);

			vector<int> sameFileNameIndexes;
			//まずは使用タイルセットのファイル名と同じファイル名がリンクファイルに登録されているか判定
			wstring sourceStem = GetStem(sourcePath);
			for (int index = 0; index < linkData.size(); index++) {
				wstring linkPath;
				StoreWStr(&linkPath, &linkData[index]["tiled"]);
				if (GetStem(linkPath) == sourceStem) {
					sameFileNameIndexes.push_back(index);
				}
			}
			//あった場合、UEのパスと表示する
			if (sameFileNameIndexes.size() > 0) {
				OutText(L"リンクファイルからパスのみが異なる同名のリンクデータが見つかりました。同じUEアセットをリンクする場合、対応する番号を入力してください。\n", OS_INFO);
				OutText(L"  [" + to_wstring(0) + L"] : 選択しない");
				
				for (int i = 0; i < sameFileNameIndexes.size(); i++) {
					//if (i > 6) {
					//	pages++;		//8以上の重複ファイルに対応するにはここらへんをいじる
					//}
					wstring tiledPathW, ue4PathW;
					StoreWStr(&tiledPathW, &linkData[sameFileNameIndexes[i]]["tiled"]);
					StoreWStr(&ue4PathW, &linkData[sameFileNameIndexes[i]]["ue4"]);
					OutText(L"  [" + to_wstring(i+1) + L"] : " + 
						tiledPathW + L" <-> " +
						ue4PathW
					);
				}
				char inch = 0x00;
				while (inch < '0' || inch > '9') {
					inch = GetKey(L"番号を入力：");
					if (inch == '0')break;
					if (inch >= '1' && inch <= '7') {

						//番号内ならば処理する、番号外ならもう一回
						if ((inch - '0') <= sameFileNameIndexes.size()) {
							//リンク処理
							wstring ue4_path;
							StoreWStr(&ue4_path, &linkData[sameFileNameIndexes[inch - '1']]["ue4"]);

							OutText(sourcePath + L" と " + ue4_path + L"をリンクします。", OS_INFO);
							AddLinkDataW(sourcePath, linkData[sameFileNameIndexes[inch - '1']]["ue4"]);
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
			wstring wSrc;
			StoreWStr(&wSrc, &source["source"]);
			OutText(L"未リンクのタイルセット " + wSrc + L" に対する処理を選択してください。", OS_INFO);
			vector<wstring> noLinkedProcStrArr = {
				L"読込処理を中止",
				L"リンク済リストから選択してリンク",
				L"エクスプローラからuassetファイルを選択してリンク",
				L"UE4内でコピペしたアセットパスを直接記入してリンク"
			};
			PrintStrList(&noLinkedProcStrArr);
			bool endFlag = false;
			//string in;
			while (!endFlag) {
				char in;
				while (true) {
					in = GetKey(L"  処理番号を入力:");

					if (in < '0' || (in-'0') >= noLinkedProcStrArr.size()) {
						//cout << "無効な処理番号です。" << endl;
					}
					else break;
				}

				string inputAsset = "";
				wstring linkedWStr = L"";
				wstring exprWStr;
				u16string u16str;
				wstring pjPath;
				switch (in)
				{
				case '0':	//中止
					OutText(L"このマップデータの読込を中止しました。", OS_INFO);
					return false;
					break;
				case '1':	//リンク済リストから選択
					OutText(L"以下のリストにある場合、左のインデックスを入力してください。");
					for (int index = 0; index < linkData.size(); index++) {

						StoreWStr(&linkedWStr, &linkData[index]["ue4"]);

						OutText(index + L" -> " + linkedWStr, OS_NONE);
					}
					break;
				case '2':	//エクスプローラ選択
					//エクスプローラの設定
					//iniファイルのディレクトリを指定
					StoreWStr(&pjPath, &linkPathJson["projectPath"]);
					SelectFile(&exprWStr, STR_FILTER{ "uassetファイル", "uasset" }, pjPath);
					//バイナリからタイルセットかどうかを確認する(タイルセットじゃなかったら警告を入れる)
					//リンクデータを追加する
					AddLinkDataW(sourcePath, exprWStr);

					break;
				case '3':	//パス入力
					OutText(L"UE4のコンテンツブラウザでコピーしたアセットパスを入力してください。", OS_INFO);
					//SetConsoleOutputCP(1252);
					//SetConsoleCP(1252);
					struct IMPORT_DATA_ATTR {
						int isPaperTileSet = -1;	//-1=不明 0=不 1=正
						bool exists = false;
						bool isUEPath = false;		//UEパスか(/Gameからのパスになっているか)
					};
					while (inputAsset != "-1") {
						//入力
						wstring s;
						getline(wcin, s);
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
							OutText(L"読込形式: UE Path");
							attr.isUEPath = true;
							//アセットの形式を読みとる(未実装)
							//attr.isPaperTileSet = ExtractImportData(UEDirectory + "\\")
						}
						else if(Like(inputAsset, "%:\\%.uasset%")){
							//絶対パスのとき
							OutText(L"読込形式: Absolute Path");

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

						wcout << s << endl;

						//wofstream outfile("testtttst.txt");
						wofstream outfile(L"testtttst.txt");
						outfile << s;
						outfile.close();


						//cin >> inputAsset;
						//cout << inputAsset << endl;
					}
					//ExtractTexture(inputAsset);
					//OutText(sourcePath + " と " + to_wstring(linkPathData[sameFileNameIndexes[inch - '1']]["ue4"]) + "をリンクします。", OS_INFO);
				}

			}

		}
	}
	
	return true;
}

void Parser::Parse(wstring _path, json _data)
{
	//WHを持ってくる
	int height = _data["height"];
	int width = _data["width"];

	//JSONのビルドエラーに対抗すべく我々はアマゾンの奥地へと向かった

	wstring outFile = outDir + filesystem::path(_path).stem().wstring();
	if (filesystem::exists(outFile + L"_output.txt") || filesystem::exists(outFile + L".json")) {
		OutText(L"出力フォルダ内に選択したファイルと同じ名前のファイルが存在します。ナンバリングを行います。", OS_WARNING);
		time_t now = time(nullptr);
		auto nowM = localtime(&now);
		wostringstream wOss;
		wOss << put_time(localtime(&now), L"_%Y%m%d%H%M%S");
		outFile += wOss.str();
		wcout << outFile << endl;
	}
	
	wofstream output(outFile + L"_output.txt");
	for (int layer = 0; layer < _data["layers"].size(); layer++) {
		wstring layerName = _data["layers"][layer]["name"];
		output << L"Begin Object Class=/Script/Paper2D.PaperTileLayer Name=\"\"\n";
		output << L"   LayerName=NSLOCTEXT(\"\", \"\", " << layerName << L")\n";
		output << L"   LayerWidth=" << width << L"\n";
		output << L"   LayerHeight=" << height << L"\n";
		output << L"   AllocatedWidth=" << width << L"\n";
		output << L"   AllocatedHeight=" << height << L"\n";
		//ここにマップ変換機構
		for (int h = 0; h < height; h++) {
			for (int w = 0; w < width; w++) {
				int index = h * width + w;	//インデックス設定
				unsigned int tiledValue = _data["layers"][layer]["data"][index];	//TILEDのデータ値

				//値が0のときは何もないのでスキップ
				//(最後の値のときのみ、空の場合でも生成)
				if (tiledValue != 0) {
					//データ変換関数
					wstring ueTileset = L"";
					int uePackedTileIndex = -1;
					ConvertData(tiledValue, &ueTileset, &uePackedTileIndex);

					//変換失敗時に出る値が帰ってきたとき処理終了
					if (ueTileset != L"" && uePackedTileIndex != -1) {
						output << L"   AllocatedCells(" << index << L")=(TileSet=PaperTileSet'\"" << ueTileset << L"\"',PackedTileIndex=" << uePackedTileIndex << L")\n";
					}
					else {
						OutText(L"変換に失敗しました。(" + to_wstring(index) + L")", OS_ERROR);
						return;
					}
				}
				else if (index == height * width - 1) {
					output << L"   AllocatedCells(" << index << L")=()\n";
				}
			}
		}
		output << "End Object\n\n";
	}
	OutputJson(outFile + L".json",
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

	std::wcout << L"何かキーを押してください..." << std::endl;
	_getch(); // キー入力を待機（入力内容は表示されない）
	return 0;
}
void Parser::Result() {
	wcout << endl << L"処理が終了しました。" << endl;
	End();
}

//ここの処理をかいておわり 多分そうでもないけど
void Parser::ConvertData(unsigned int tiledValue, wstring* uePath, int* ueTileValue) {
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
				StoreWStr(uePath, &linkData[useLinkDataIndexes[tilesetID]]["ue4"]);
				*ueTileValue = tiledValue - data["tilesets"][tilesetID]["firstgid"];

				break;
			}
		}
	}
	catch (json::type_error &e) {
		OutText(L"マップデータまたはリンクファイルに必要となるキーが存在しないか、不正な値が入っています。変換を中止します。", OS_ERROR);
		return;
	}
}

wstring Parser::GetStem(wstring path)
{
	wstring ret = filesystem::path(path).stem().wstring();
	//cout << "ステムを取る：" << path << " -> " << ret << endl;
	return ret;
}

void Parser::AddLinkDataW(wstring tiled_sourcePath, wstring ue4_path)
{
	json& linkData = linkPathJson["linkData"];
	//リンク処理
	OutText(tiled_sourcePath + L" と " + ue4_path + L"をリンクします。", OS_INFO);
	json value;
	value += json::object_t::value_type("tiled", tiled_sourcePath);
	value += json::object_t::value_type("ue4", ue4_path);
	try {
		linkData.push_back(value);
		OutputJson(parentDir + L"\\linkPath２うお溢.json", linkPathJson);
	}
	catch (json::type_error e) {
		OutText(L"保存時にエラーが発生しました(" + (wchar_t)e.what(), OS_ERROR);
	}
}

void Parser::StoreWStr(wstring* wstr, json* j)
{
	// UTF-8 を UTF-16 (ワイド文字列) に変換
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	*wstr = converter.from_bytes(j->dump());
}

char Parser::GetKey(wstring descr)
{
	std::wcout << descr;
	int inch = _getch();
	if (isgraph(inch)) {
		wcout << (wchar_t)inch << endl;
		return (wchar_t)inch;
	}
	else {
		wcout << L"Invalid Input" << endl;
	}
	return 0x00;
}

bool Parser::InputBool(wstring descr)
{
	std::wcout << descr << L"[Y/N]:";
	while (true) {
		int inch = toupper(_getch());
		if (In(inch, { 'Y','N' })) {
			wcout << (wchar_t)inch << endl;
			return (inch == 'Y' ? true : false);
			//if (inch == 'Y')return true;
			//return false;
		}
	}
	return false;
}

void Parser::OutputJson(wstring filePath, json content)
{
	if (((filesystem::path)(filePath)).extension() == ".json") OutText(L"JSON形式で出力されるファイルの拡張子が.jsonではありません。使用する際はご注意ください。", OS_WARNING);
	ofstream out(filePath);
	out << content.dump(2);
	out.close();
}

//void Parser::OutText(string str, OUTPUT_STATE outState) {
//	wcout << "Aねデ";
//	switch (outState)
//	{
//	case OS_NONE:		wcout << " ";						break;
//	case OS_INFO:		wcout << "[INFO] ";					break;
//	case OS_WARNING:	wcout << "\033[33m" << "[WARNING] ";	break;
//	case OS_ERROR:		wcout << "\033[31m" << "[ERROR] ";	break;
//	}
//
//	wcout << str << "\033[0m" << endl;
//}
void Parser::OutText(wstring wstr, OUTPUT_STATE outState) {
	//SetConsoleOutputCP(CP_UTF8);
	//std::wcout.imbue(std::locale(""));
	switch (outState)
	{
	case OS_NONE:		wcout << L" ";						break;
	case OS_INFO:		wcout << L"[INFO] ";					break;
	case OS_WARNING:	wcout << L"\033[33m" << L"[WARNING] ";	break;
	case OS_ERROR:		wcout << L"\033[31m" << L"[ERROR] ";	break;
	}
	wcout << wstr << L"\033[0m" << endl;
}

bool Parser::BreakNIsContinue(wstring warnStr)
{
	OutText(warnStr, OS_WARNING);
	return InputBool(L"続行しますか？");
}

void Parser::PrintStrList(vector<wstring>* descrList, int startNum)
{
	for (int i = 0; i < descrList->size(); i++) {
		OutText(L"  [" + to_wstring(i + startNum) + L"] : " +
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

string Parser::PGetCurrentDirectoryA()
{
	char dir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, (LPWSTR)dir);
	return dir;
}

wstring Parser::PGetCurrentDirectoryW()
{
	wchar_t dir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, (LPWSTR)dir);
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