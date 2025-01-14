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
	//���s�t�@�C���̃f�B���N�g�����擾
	std::filesystem::path path = exePath;
	parentDir = path.parent_path().wstring();
	outDir = parentDir + L"\\output\\";

	//Tiled��UE4�̃����N�t�@�C����Ǎ�
	wstring lpPath = parentDir + L"\\linkPath.json";
	if (filesystem::exists(lpPath)) {
		std::ifstream lp(lpPath);

		try {
			linkPathJson = json::parse(lp);
		}
		catch (json::parse_error e) {
			OutText(L"�����N�p�X�t�@�C���̓Ǎ����ɖ�肪�������܂����F" + (wchar_t)e.what(), OS_ERROR);
			return false;
		}
	}
	else {
		OutText(L"�����N�p�X�t�@�C����������܂���B", OS_ERROR);
		return 0;
	}

	if (!filesystem::is_directory(outDir)) {
		OutText(L"�o�̓t�H���_������܂���B�f�B���N�g���������ǉ����܂��B", OS_WARNING);
		if (!filesystem::create_directory(outDir)) {
			OutText(L"�o�̓t�H���_�̍쐬�Ɏ��s���܂����B�Ǘ��Ҍ������K�v�ȏꏊ�Ŏ��s���Ă��邩�A�e�ʂ�����Ȃ��\��������܂��B", OS_ERROR);
			return 0;
		};
	}
}

bool Parser::Process(int argc, wchar_t* argv[])
{
	//������
	OutText(L"�p�[�T�̃Z�b�g�A�b�v���J�n", OS_INFO);
	if (Init(argv[0])) {
		OutText(L"�p�[�T�̃Z�b�g�A�b�v������", OS_INFO);
	}
	else {
		OutText(L"�Z�b�g�A�b�v���ɖ�肪�������܂����B�������I�����܂��B", OS_NONE);
		End();
		return false;
	}

	//�t�@�C���I��
	if (!StoreParseFile(argc, argv, &parsePaths)) {
		OutText(L"�t�@�C�����I������܂���ł����B�������I�����܂��B", OS_NONE);
		End();
		return false;
	}

	//�Ǎ�+����
	for (int i = 0; i < parsePaths.size(); i++) {
		Br();
		OutText(parsePaths[i] + L" ��ϊ����Ă��܂�...(" + to_wstring(i + 1) + L"/" + to_wstring(parsePaths.size()) + L")", OS_INFO);
		Br();

		//�Ǎ�
		if (!Read(parsePaths[i], &data))continue;

		//����
		Parse(parsePaths[i], data);
	}
	//�Ǎ�+����

	Result();
	return 0;
}

bool Parser::StoreParseFile(int argc, wchar_t* argv[], vector<wstring>* paths)
{
	// ������1�ȏ゠��ꍇ�i0�Ԗڂ͎��s�t�@�C�����g�̃p�X�j
	if (argc > 1) {
		OutText(L"D&D���ꂽ�t�@�C����ǂݍ��݂܂��B", OS_INFO);
		OutText(L"�Ǎ��f�[�^�F");
		for (int i = 1; i < argc; i++) {
			paths->push_back(argv[i]);
			OutText(L"  [" + to_wstring(i - 1) + L"] : " + argv[i]);
			wcout << L"����" << endl;
		}
	}
	//�������Ȃ��ꍇ(argc��1�ȉ�)
	else {
		OutText(L"�ϊ�����t�@�C����I�����Ă��������B", OS_INFO);
		SelectFile(paths, STR_FILTER{ "�}�b�v�f�[�^", "json" }, PGetCurrentDirectoryW());
		if (paths->size() == 0) {
			//OutText(L"�t�@�C�����I������܂���ł����B�I�����܂��B", OS_INFO);
			return false;
		}
		else {
			OutText(L"�Ǎ��f�[�^�F");
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
	
	//�w��f�B���N�g�������݂��Ȃ��Ƃ��A�J�����g�f�B���N�g���ɂȂ�悤�w��
	if (!filesystem::exists(dir)) 	dir = currentDir;

	wchar_t fileName[MAX_PATH] = L"";  //�t�@�C����������ϐ�

	vector<char> filterArr;

	auto AddFilter = [=](vector<char>* flt, STR_FILTER& addFlt) {
		string overview = addFlt.descr + "(*." + addFlt.ext + ")";
		string ext = "*." + addFlt.ext;

		flt->insert(flt->end(), overview.begin(), overview.end());
		flt->push_back('\0');
		flt->insert(flt->end(), ext.begin(), ext.end());
		flt->push_back('\0');
		};

	//�w��t�B���^�ǉ�
	for (auto& flt : filters) {
		AddFilter(&filterArr, flt);
	}

	//�S�t�@�C���t�B���^�̓K�p�֘A
	if (enAllFile) {
		//�S�t�@�C���t�B���^�ǉ�
		STR_FILTER filterAll = { "���ׂẴt�@�C��" , "*" };
		AddFilter(&filterArr, filterAll);
	}

	// �t�B���^������S�̂� NULL �I�[����
	filterArr.push_back('\0');

	//�u�t�@�C�����J���v�_�C�A���O�̐ݒ�
	OPENFILENAMEW ofnw;
	ZeroMemory(&ofnw, sizeof(ofnw));            	//�\���̏�����
	ofnw.lStructSize = sizeof(OPENFILENAMEW);   	//�\���̂̃T�C�Y

	// �I�[�������l�����ăf�[�^���R�s�[
	std::string utf8Str(filterArr.begin(), filterArr.end());
	// UTF-8 ���� UTF-16 �ɕϊ�
	int wideSize = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
	std::wstring filterWStr(wideSize, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &filterWStr[0], wideSize);

	// �Ō�̏I�[�������폜
	filterWStr.pop_back();

	//wstring filterWStr;
	ofnw.lpstrFilter = filterWStr.c_str();
	ofnw.lpstrFile = fileName;               	//�t�@�C����
	ofnw.nMaxFile = MAX_PATH;               	//�p�X�̍ő啶����
	if (!isSingleFile)	ofnw.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER;	//����ŕ����I�����ł�����
	ofnw.lpstrDefExt = L"json";                  	//�f�t�H���g�g���q
	ofnw.lpstrInitialDir = dir.c_str();
	//�u�t�@�C�����J���v�_�C�A���O
	BOOL selFile;
	selFile = GetOpenFileNameW(&ofnw);

	//OPENFILENAME ofn;                         	//���O�����ĕۑ��_�C�A���O�̐ݒ�p�\����
	//ZeroMemory(&ofn, sizeof(ofn));            	//�\���̏�����
	//ofn.lStructSize = sizeof(OPENFILENAME);   	//�\���̂̃T�C�Y
	//ofn.lpstrFilter = (LPCWSTR)filterArr.data();
	//ofn.lpstrFile = (LPWSTR)fileName;               	//�t�@�C����
	//ofn.nMaxFile = MAX_PATH;               	//�p�X�̍ő啶����
	//if (!isSingleFile)	ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER;	//����ŕ����I�����ł�����
	//ofn.lpstrDefExt = L"json";                  	//�f�t�H���g�g���q
	//ofn.lpstrInitialDir = (LPCWSTR)dir.c_str();
	////�u�t�@�C�����J���v�_�C�A���O
	//BOOL selFile;
	//selFile = GetOpenFileNameW(&ofn);

	//�L�����Z�������璆�f
	if (selFile == FALSE) return vector<wstring>();

	// �����t�@�C�����I�����ꂽ�ꍇ
	std::vector<std::wstring> selectedFiles;
	std::wstring directory(fileName);
	TCHAR* ptr = (TCHAR*)fileName + directory.length() + 1;

	if (*ptr == '\0')	selectedFiles.push_back(directory);	//�P��t�@�C���̏ꍇ
	else {
		//�����t�@�C���̏ꍇ
		while (*ptr) {
			wstring s = ptr;
			selectedFiles.push_back(directory + L"\\" + wstring(ptr));
			ptr += wcslen(ptr) + 1;
		}
	}

	// �I�����ꂽ�t�@�C�����o��
	for (const auto& file : selectedFiles) std::wcout << "Selected File: " << file << std::endl;

	//�J�����g�f�B���N�g�������Ƃɂ��ǂ�
	SetCurrentDirectory((LPCWSTR)currentDir.c_str());

	//���������@�킩���̂ō���1�����v�b�V��
	//260���������Ď��s�����Ƃ��̏��������ĂȂ����ǑS���Ԃ�
	return selectedFiles;

}

bool Parser::Read(wstring _path, json* _data)
{
	//������
	useLinkDataIndexes.clear();

	json& linkData = linkPathJson["linkData"];
	//if (1/*Unicode�����邩*/) {
	//	//�f�[�^�R�s�[
	//	string tmpPath = outDir + "uniTemp.tmp";
	//	ofstream tmp(tmpPath);
	//	tmp << 
	//	//std::filesystem::path path = _path;
	//	//path.replace_extension(".json");
	//}
	//else {
		//json�Ǎ�
		filesystem::path inPath = _path;
		std::ifstream f(_path);
		try { *_data = json::parse(f); }
		catch (json::parse_error e) {
			OutText(L"�}�b�v�f�[�^�̓Ǎ��Ɏ��s���܂����F" + (wchar_t)(e.what()), OS_ERROR);
			return false;
		}

	//}
	
	
	bool isLinked = false;
	//�g�p�����^�C���Z�b�g��UE�p�X�ƕR�t�����Ă��邩
	for (json& source : (*_data)["tilesets"]) {
		wstring sourcePath;
		StoreWStr(&sourcePath, &source["source"]);
		isLinked = false;	//��U�����N����ĂȂ�����ɂ���

		for (int index = 0; index < linkData.size(); index++) {
			wstring linkPath;

			StoreWStr(&linkPath, &linkData[index]["tiled"]);
			if (sourcePath == linkPath) {
				//���݂����ꍇ
				useLinkDataIndexes.push_back(index);	//json���̎g�p�^�C���Z�b�g�C���f�b�N�X(0���珇�Ƀ��[�v���Ă��邽��push_back��OK)��linkPath�̃C���f�b�N�X��R�t��
				isLinked = true;
				break;
			}
		}
		//���݂��Ȃ������ꍇ
		if (!isLinked) {
			OutText(L"�g�p�^�C���Z�b�g " + sourcePath + L" �������N����Ă��܂���B", OS_ERROR);

			vector<int> sameFileNameIndexes;
			//�܂��͎g�p�^�C���Z�b�g�̃t�@�C�����Ɠ����t�@�C�����������N�t�@�C���ɓo�^����Ă��邩����
			wstring sourceStem = GetStem(sourcePath);
			for (int index = 0; index < linkData.size(); index++) {
				wstring linkPath;
				StoreWStr(&linkPath, &linkData[index]["tiled"]);
				if (GetStem(linkPath) == sourceStem) {
					sameFileNameIndexes.push_back(index);
				}
			}
			//�������ꍇ�AUE�̃p�X�ƕ\������
			if (sameFileNameIndexes.size() > 0) {
				OutText(L"�����N�t�@�C������p�X�݂̂��قȂ铯���̃����N�f�[�^��������܂����B����UE�A�Z�b�g�������N����ꍇ�A�Ή�����ԍ�����͂��Ă��������B\n", OS_INFO);
				OutText(L"  [" + to_wstring(0) + L"] : �I�����Ȃ�");
				
				for (int i = 0; i < sameFileNameIndexes.size(); i++) {
					//if (i > 6) {
					//	pages++;		//8�ȏ�̏d���t�@�C���ɑΉ�����ɂ͂�����ւ��������
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
					inch = GetKey(L"�ԍ�����́F");
					if (inch == '0')break;
					if (inch >= '1' && inch <= '7') {

						//�ԍ����Ȃ�Ώ�������A�ԍ��O�Ȃ�������
						if ((inch - '0') <= sameFileNameIndexes.size()) {
							//�����N����
							wstring ue4_path;
							StoreWStr(&ue4_path, &linkData[sameFileNameIndexes[inch - '1']]["ue4"]);

							OutText(sourcePath + L" �� " + ue4_path + L"�������N���܂��B", OS_INFO);
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
			//�Ȃ��ꍇ�A�ق��̏�����I��������
			wstring wSrc;
			StoreWStr(&wSrc, &source["source"]);
			OutText(L"�������N�̃^�C���Z�b�g " + wSrc + L" �ɑ΂��鏈����I�����Ă��������B", OS_INFO);
			vector<wstring> noLinkedProcStrArr = {
				L"�Ǎ������𒆎~",
				L"�����N�σ��X�g����I�����ă����N",
				L"�G�N�X�v���[������uasset�t�@�C����I�����ă����N",
				L"UE4���ŃR�s�y�����A�Z�b�g�p�X�𒼐ڋL�����ă����N"
			};
			PrintStrList(&noLinkedProcStrArr);
			bool endFlag = false;
			//string in;
			while (!endFlag) {
				char in;
				while (true) {
					in = GetKey(L"  �����ԍ������:");

					if (in < '0' || (in-'0') >= noLinkedProcStrArr.size()) {
						//cout << "�����ȏ����ԍ��ł��B" << endl;
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
				case '0':	//���~
					OutText(L"���̃}�b�v�f�[�^�̓Ǎ��𒆎~���܂����B", OS_INFO);
					return false;
					break;
				case '1':	//�����N�σ��X�g����I��
					OutText(L"�ȉ��̃��X�g�ɂ���ꍇ�A���̃C���f�b�N�X����͂��Ă��������B");
					for (int index = 0; index < linkData.size(); index++) {

						StoreWStr(&linkedWStr, &linkData[index]["ue4"]);

						OutText(index + L" -> " + linkedWStr, OS_NONE);
					}
					break;
				case '2':	//�G�N�X�v���[���I��
					//�G�N�X�v���[���̐ݒ�
					//ini�t�@�C���̃f�B���N�g�����w��
					StoreWStr(&pjPath, &linkPathJson["projectPath"]);
					SelectFile(&exprWStr, STR_FILTER{ "uasset�t�@�C��", "uasset" }, pjPath);
					//�o�C�i������^�C���Z�b�g���ǂ������m�F����(�^�C���Z�b�g����Ȃ�������x��������)
					//�����N�f�[�^��ǉ�����
					AddLinkDataW(sourcePath, exprWStr);

					break;
				case '3':	//�p�X����
					OutText(L"UE4�̃R���e���c�u���E�U�ŃR�s�[�����A�Z�b�g�p�X����͂��Ă��������B", OS_INFO);
					//SetConsoleOutputCP(1252);
					//SetConsoleCP(1252);
					struct IMPORT_DATA_ATTR {
						int isPaperTileSet = -1;	//-1=�s�� 0=�s 1=��
						bool exists = false;
						bool isUEPath = false;		//UE�p�X��(/Game����̃p�X�ɂȂ��Ă��邩)
					};
					while (inputAsset != "-1") {
						//����
						wstring s;
						getline(wcin, s);
						bool isAbsolutePath = false;
						bool existsFile = false;
						/*
						�z�肳������
						UE4�R�s�y
						PaperTileSet'/Game/Assets/tilemap/tileset_field_TileSet.tileset_field_TileSet'
						/Game/Assets/tilemap/tileset_field_TileSet.tileset_field_TileSet
						"D:\GE3A09\Unreal Projects\UE2D\Content\Assets\tilemap\tileset_field_TileSet.uasset"
						D:\GE3A09\Unreal Projects\UE2D\Content\Assets\tilemap\tileset_field_TileSet.uasset
						����ȊO�͑Ή����܂���@����
						*/
						IMPORT_DATA_ATTR attr;
						if (Like(inputAsset, "%/Game/%")) {
							//UE�p�X�̂Ƃ�
							OutText(L"�Ǎ��`��: UE Path");
							attr.isUEPath = true;
							//�A�Z�b�g�̌`����ǂ݂Ƃ�(������)
							//attr.isPaperTileSet = ExtractImportData(UEDirectory + "\\")
						}
						else if(Like(inputAsset, "%:\\%.uasset%")){
							//��΃p�X�̂Ƃ�
							OutText(L"�Ǎ��`��: Absolute Path");

							attr.isPaperTileSet = -1;	//�^�C���Z�b�g���͕s��(������)
							attr.isUEPath = false;		//UEPath�ł͂Ȃ�
							attr.exists = filesystem::exists(inputAsset);	//�t�@�C�������݂��邩

						}

						//if (attr.exists) {
						//	if(attr.isUEPath)
						//}


						//���̃p�X�����݂��邩(�Ȃ�������x��)
						if(filesystem::exists(linkPathJson["projectPath"] + "\\Content\\"))
						//����ɓ��͂����Ƃ�								PaperTileSet'/Game/Assets/tilemap/tileset_field_TileSet.tileset_field_TileSet'	
						//���͓��e����΃p�X�������Ƃ�
						  //���̃p�X�͌��݂̃v���W�F�N�g���H(�Ȃ���Όx��)
						  
						//�p�X�̂����A''�ň͂܂ꂽ�����������͂��Ă����Ƃ�
						//���͂������A�^�C���Z�b�g�ł͂Ȃ��p�X�������ꍇ(�x��)
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
					//OutText(sourcePath + " �� " + to_wstring(linkPathData[sameFileNameIndexes[inch - '1']]["ue4"]) + "�������N���܂��B", OS_INFO);
				}

			}

		}
	}
	
	return true;
}

void Parser::Parse(wstring _path, json _data)
{
	//WH�������Ă���
	int height = _data["height"];
	int width = _data["width"];

	//JSON�̃r���h�G���[�ɑ΍R���ׂ���X�̓A�}�]���̉��n�ւƌ�������

	wstring outFile = outDir + filesystem::path(_path).stem().wstring();
	if (filesystem::exists(outFile + L"_output.txt") || filesystem::exists(outFile + L".json")) {
		OutText(L"�o�̓t�H���_���ɑI�������t�@�C���Ɠ������O�̃t�@�C�������݂��܂��B�i���o�����O���s���܂��B", OS_WARNING);
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
		//�����Ƀ}�b�v�ϊ��@�\
		for (int h = 0; h < height; h++) {
			for (int w = 0; w < width; w++) {
				int index = h * width + w;	//�C���f�b�N�X�ݒ�
				unsigned int tiledValue = _data["layers"][layer]["data"][index];	//TILED�̃f�[�^�l

				//�l��0�̂Ƃ��͉����Ȃ��̂ŃX�L�b�v
				//(�Ō�̒l�̂Ƃ��̂݁A��̏ꍇ�ł�����)
				if (tiledValue != 0) {
					//�f�[�^�ϊ��֐�
					wstring ueTileset = L"";
					int uePackedTileIndex = -1;
					ConvertData(tiledValue, &ueTileset, &uePackedTileIndex);

					//�ϊ����s���ɏo��l���A���Ă����Ƃ������I��
					if (ueTileset != L"" && uePackedTileIndex != -1) {
						output << L"   AllocatedCells(" << index << L")=(TileSet=PaperTileSet'\"" << ueTileset << L"\"',PackedTileIndex=" << uePackedTileIndex << L")\n";
					}
					else {
						OutText(L"�ϊ��Ɏ��s���܂����B(" + to_wstring(index) + L")", OS_ERROR);
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

	std::wcout << L"�����L�[�������Ă�������..." << std::endl;
	_getch(); // �L�[���͂�ҋ@�i���͓��e�͕\������Ȃ��j
	return 0;
}
void Parser::Result() {
	wcout << endl << L"�������I�����܂����B" << endl;
	End();
}

//�����̏����������Ă���� ���������ł��Ȃ�����
void Parser::ConvertData(unsigned int tiledValue, wstring* uePath, int* ueTileValue) {
	json& linkData = linkPathJson["linkData"];
	/*
	Tiled Format -> UE4 Format
	unsigned int -> singed int
	* 32bit:Y���](�������])
	* 31bit:X���](�������])
	* 30bit:�Δ��]
			0  90 180         270
	 ���v 000 101 110(XY���]) 011
	X���] 100 111 010(Y���])  001

	*/
	int gid = tiledValue & (int)(pow(2, 29) - 1);	//29�r�b�g�ڂ܂łƂ�
	try {
		//�^�C���Z�b�gID(json���^�C���Z�b�g���X�g�̃C���f�b�N�X)��ID�f�J���ق�����T��
		for (int tilesetID = data["tilesets"].size() - 1; tilesetID >= 0; tilesetID--) {
			//�^�C���Z�b�g���X�g�Ɋi�[���ꂽ�O���[�o��ID�Ɣ�r�A����������݃^�C���̒l���傫�����UE�p�X��UE�^�C���l���i�[
			if (gid >= data["tilesets"][tilesetID]["firstgid"]) {
				StoreWStr(uePath, &linkData[useLinkDataIndexes[tilesetID]]["ue4"]);
				*ueTileValue = tiledValue - data["tilesets"][tilesetID]["firstgid"];

				break;
			}
		}
	}
	catch (json::type_error &e) {
		OutText(L"�}�b�v�f�[�^�܂��̓����N�t�@�C���ɕK�v�ƂȂ�L�[�����݂��Ȃ����A�s���Ȓl�������Ă��܂��B�ϊ��𒆎~���܂��B", OS_ERROR);
		return;
	}
}

wstring Parser::GetStem(wstring path)
{
	wstring ret = filesystem::path(path).stem().wstring();
	//cout << "�X�e�������F" << path << " -> " << ret << endl;
	return ret;
}

void Parser::AddLinkDataW(wstring tiled_sourcePath, wstring ue4_path)
{
	json& linkData = linkPathJson["linkData"];
	//�����N����
	OutText(tiled_sourcePath + L" �� " + ue4_path + L"�������N���܂��B", OS_INFO);
	json value;
	value += json::object_t::value_type("tiled", tiled_sourcePath);
	value += json::object_t::value_type("ue4", ue4_path);
	try {
		linkData.push_back(value);
		OutputJson(parentDir + L"\\linkPath�Q������.json", linkPathJson);
	}
	catch (json::type_error e) {
		OutText(L"�ۑ����ɃG���[���������܂���(" + (wchar_t)e.what(), OS_ERROR);
	}
}

void Parser::StoreWStr(wstring* wstr, json* j)
{
	// UTF-8 �� UTF-16 (���C�h������) �ɕϊ�
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
	if (((filesystem::path)(filePath)).extension() == ".json") OutText(L"JSON�`���ŏo�͂����t�@�C���̊g���q��.json�ł͂���܂���B�g�p����ۂ͂����ӂ��������B", OS_WARNING);
	ofstream out(filePath);
	out << content.dump(2);
	out.close();
}

//void Parser::OutText(string str, OUTPUT_STATE outState) {
//	wcout << "A�˃f";
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
	return InputBool(L"���s���܂����H");
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
	//���K�\���ϊ�
	string regexPattern;
	for (char c : search) {
		switch (c) {
		case '%': // �C�ӂ̕�����ɑΉ�
			regexPattern += ".*";
			break;
		case '_': // �C�ӂ�1�����ɑΉ�
			regexPattern += ".";
			break;
		case '.': // '.'�͐��K�\���œ��ꕶ���Ȃ̂ŃG�X�P�[�v
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
		case '\\': // '\'���G�X�P�[�v���K�v
			regexPattern += '\\';
			[[fallthrough]];
		default: // ����ȊO�̕���
			regexPattern += c;
			break;
		}
	}

	//���K�\���I�u�W�F�N�g�쐬(icase�ő啶���������𖳎�)
	std::regex re(regexPattern, std::regex::icase);

	//����
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
�p�X��������Ȃ��Ƃ��ɃG���[�f���ċ����I������o�O
�����I��(��D&D��)

issue
�����̃����N�f�[�^��8�ȏ゠�����Ƃ��A8�ȍ~�̃f�[�^���\������Ȃ�(8��9��pageUp/Down�����蓖�ĂĂ邽��)


memo

250108 LIKE������
*/