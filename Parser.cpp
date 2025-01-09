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
	//���s�t�@�C���̃f�B���N�g�����擾
	std::filesystem::path path = exePath;
	parentDir = path.parent_path().string();
	outDir = parentDir + "\\output\\";

	//Tiled��UE4�̃����N�t�@�C����Ǎ�
	string lpPath = parentDir + "\\linkPath.json";
	if (filesystem::exists(lpPath)) {
		std::ifstream lp(lpPath);

		try {
			linkPathJson = json::parse(lp);
		}
		catch (json::parse_error e) {
			OutText("�����N�p�X�t�@�C���̓Ǎ����ɖ�肪�������܂����F" + (string)e.what(), OS_ERROR);
			return false;
		}
	}
	else {
		OutText("�����N�p�X�t�@�C����������܂���B", OS_ERROR);
		return 0;
	}

	if (!filesystem::is_directory(outDir)) {
		OutText("�o�̓t�H���_������܂���B�f�B���N�g���������ǉ����܂��B", OS_WARNING);
		if (!filesystem::create_directory(outDir)) {
			OutText("�o�̓t�H���_�̍쐬�Ɏ��s���܂����B�Ǘ��Ҍ������K�v�ȏꏊ�Ŏ��s���Ă��邩�A�e�ʂ�����Ȃ��\��������܂��B", OS_ERROR);
			return 0;
		};
	}
}

bool Parser::Process(int argc, char* argv[])
{
	//������
	OutText("�p�[�T�̃Z�b�g�A�b�v���J�n", OS_INFO);
	if (Init(argv[0])) {
		OutText("�p�[�T�̃Z�b�g�A�b�v������", OS_INFO);
	}
	else {
		OutText("�Z�b�g�A�b�v���ɖ�肪�������܂����B�������I�����܂��B", OS_NONE);
		End();
		return false;
	}

	//�t�@�C���I��
	if (!StoreParseFile(argc, argv, &parsePaths)) {
		OutText("�t�@�C�����I������܂���ł����B�������I�����܂��B", OS_NONE);
		End();
		return false;
	}

	//�Ǎ�+����
	for (int i = 0; i < parsePaths.size(); i++) {
		Br();
		OutText(parsePaths[i] + " ��ϊ����Ă��܂�...(" + to_string(i + 1) + "/" + to_string(parsePaths.size()) + ")", OS_INFO);
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

bool Parser::StoreParseFile(int argc, char* argv[], vector<string>* paths)
{
	// ������1�ȏ゠��ꍇ�i0�Ԗڂ͎��s�t�@�C�����g�̃p�X�j
	if (argc > 1) {
		OutText("D&D���ꂽ�t�@�C����ǂݍ��݂܂��B", OS_INFO);
		OutText("�Ǎ��f�[�^�F");
		for (int i = 1; i < argc; i++) {
			paths->push_back(argv[i]);
			OutText("  [" + to_string(i - 1) + "] : " + argv[i]);
		}
	}
	//�������Ȃ��ꍇ(argc��1�ȉ�)
	else {
		OutText("�ϊ�����t�@�C����I�����Ă��������B", OS_INFO);
		SelectFile(paths, STR_FILTER{ "�}�b�v�f�[�^", "json" }, PGetCurrentDirectory());
		if (paths->size() == 0) {
			//OutText("�t�@�C�����I������܂���ł����B�I�����܂��B", OS_INFO);
			return false;
		}
		else {
			OutText("�Ǎ��f�[�^�F");
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

	//�w��f�B���N�g�����󔒂ł͂Ȃ��ꍇ�̂݃J�����g�f�B���N�g�����X�V
	if (dir != "") {
		SetCurrentDirectory(dir.c_str());
	}

	char fileName[MAX_PATH] = "";  //�t�@�C����������ϐ�

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
	OPENFILENAME ofn;                         	//���O�����ĕۑ��_�C�A���O�̐ݒ�p�\����
	ZeroMemory(&ofn, sizeof(ofn));            	//�\���̏�����
	ofn.lStructSize = sizeof(OPENFILENAME);   	//�\���̂̃T�C�Y
	ofn.lpstrFilter = filterArr.data();
	ofn.lpstrFile = fileName;               	//�t�@�C����
	ofn.nMaxFile = MAX_PATH;               	//�p�X�̍ő啶����
	if (!isSingleFile)	ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER;	//����ŕ����I�����ł�����
	ofn.lpstrDefExt = "json";                  	//�f�t�H���g�g���q

	//�u�t�@�C�����J���v�_�C�A���O
	BOOL selFile;
	selFile = GetOpenFileNameA(&ofn);

	//�L�����Z�������璆�f
	if (selFile == FALSE) return vector<string>();

	// �����t�@�C�����I�����ꂽ�ꍇ
	std::vector<std::string> selectedFiles;
	std::string directory(fileName);
	TCHAR* ptr = fileName + directory.length() + 1;

	if (*ptr == '\0')	selectedFiles.push_back(directory);	//�P��t�@�C���̏ꍇ
	else {
		//�����t�@�C���̏ꍇ
		while (*ptr) {
			selectedFiles.push_back(directory + "\\" + std::string(ptr));
			ptr += strlen(ptr) + 1;
		}
	}

	// �I�����ꂽ�t�@�C�����o��
	for (const auto& file : selectedFiles) std::cout << "Selected File: " << file << std::endl;

	//�J�����g�f�B���N�g�������Ƃɂ��ǂ�
	SetCurrentDirectory(currentDir.c_str());

	//���������@�킩���̂ō���1�����v�b�V��
	//260���������Ď��s�����Ƃ��̏��������ĂȂ����ǑS���Ԃ�
	return selectedFiles;

}

bool Parser::Read(string _path, json* _data)
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
			OutText("�}�b�v�f�[�^�̓Ǎ��Ɏ��s���܂����F" + (string)(e.what()), OS_ERROR);
			return false;
		}

	//}
	
	
	bool isLinked = false;
	//�g�p�����^�C���Z�b�g��UE�p�X�ƕR�t�����Ă��邩
	for (json& source : (*_data)["tilesets"]) {
		string sourcePath = source["source"];
		isLinked = false;	//��U�����N����ĂȂ�����ɂ���

		for (int index = 0; index < linkData.size(); index++) {
			string linkPath = linkData[index]["tiled"];
			
			if (sourcePath == linkPath) {
				//���݂����ꍇ
				useLinkDataIndexes.push_back(index);	//json���̎g�p�^�C���Z�b�g�C���f�b�N�X(0���珇�Ƀ��[�v���Ă��邽��push_back��OK)��linkPath�̃C���f�b�N�X��R�t��
				isLinked = true;
				break;
			}
		}
		//���݂��Ȃ������ꍇ
		if (!isLinked) {
			OutText("�g�p�^�C���Z�b�g " + sourcePath + " �������N����Ă��܂���B", OS_ERROR);

			vector<int> sameFileNameIndexes;
			//�܂��͎g�p�^�C���Z�b�g�̃t�@�C�����Ɠ����t�@�C�����������N�t�@�C���ɓo�^����Ă��邩����
			string sourceStem = GetStem(sourcePath);
			for (int index = 0; index < linkData.size(); index++) {
				string linkPath = linkData[index]["tiled"];
				if (GetStem(linkPath) == sourceStem) {
					sameFileNameIndexes.push_back(index);
				}
			}
			//�������ꍇ�AUE�̃p�X�ƕ\������
			if (sameFileNameIndexes.size() > 0) {
				OutText("�����N�t�@�C������p�X�݂̂��قȂ铯���̃����N�f�[�^��������܂����B����UE�A�Z�b�g�������N����ꍇ�A�Ή�����ԍ�����͂��Ă��������B\n", OS_INFO);
				OutText("  [" + to_string(0) + "] : �I�����Ȃ�");
				for (int i = 0; i < sameFileNameIndexes.size(); i++) {
					//if (i > 6) {
					//	pages++;		//8�ȏ�̏d���t�@�C���ɑΉ�����ɂ͂�����ւ��������
					//}
					OutText("  [" + to_string(i+1) + "] : " + 
						to_string(linkData[sameFileNameIndexes[i]]["tiled"]) + " <-> " +
						to_string(linkData[sameFileNameIndexes[i]]["ue4"])
					);
				}
				char inch = 0x00;
				while (inch < '0' || inch > '9') {
					inch = GetKey("�ԍ�����́F");
					if (inch == '0')break;
					if (inch >= '1' && inch <= '7') {

						//�ԍ����Ȃ�Ώ�������A�ԍ��O�Ȃ�������
						if ((inch - '0') <= sameFileNameIndexes.size()) {
							//�����N����
							OutText(sourcePath + " �� " + to_string(linkData[sameFileNameIndexes[inch - '1']]["ue4"]) + "�������N���܂��B", OS_INFO);
							json value;
							value += json::object_t::value_type("tiled", sourcePath);
							value += json::object_t::value_type("ue4", linkData[sameFileNameIndexes[inch - '1']]["ue4"]);
							try {
								linkData.push_back(value);
								OutputJson(parentDir + "\\linkPath.json", linkPathJson);
							}
							catch (json::type_error e) {
								OutText("�ۑ����ɃG���[���������܂���(" + (string)e.what(), OS_ERROR);
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
			//�Ȃ��ꍇ�A�ق��̏�����I��������

			OutText("�������N�̃^�C���Z�b�g " + to_string(source["source"]) + " �ɑ΂��鏈����I�����Ă��������B", OS_INFO);
			vector<string> noLinkedProcStrArr = {
				"�Ǎ������𒆎~",
				"�����N�σ��X�g����I�����ă����N",
				"�G�N�X�v���[������uasset�t�@�C����I�����ă����N",
				"UE4���ŃR�s�y�����A�Z�b�g�p�X�𒼐ڋL�����ă����N"
			};
			PrintStrList(&noLinkedProcStrArr);
			bool endFlag = false;
			//string in;
			while (!endFlag) {
				char in;
				while (true) {
					in = GetKey("  �����ԍ������:");

					if (in < '0' || (in-'0') >= noLinkedProcStrArr.size()) {
						//cout << "�����ȏ����ԍ��ł��B" << endl;
					}
					else break;
				}

				string inputAsset = "";
				vector<string> vstr;
				switch (in)
				{
				case '0':	//���~
					OutText("���̃}�b�v�f�[�^�̓Ǎ��𒆎~���܂����B", OS_INFO);
					return false;
					break;
				case '1':	//�����N�σ��X�g����I��
					OutText("�ȉ��̃��X�g�ɂ���ꍇ�A���̃C���f�b�N�X����͂��Ă��������B");
					for (int index = 0; index < linkData.size(); index++) {
						OutText(index + " -> " + to_string(linkData[index]["ue4"]), OS_NONE);
					}
					break;
				case '2':	//�G�N�X�v���[���I��
					//�G�N�X�v���[���̐ݒ�
					SelectFile(&vstr, STR_FILTER{ "uasset�t�@�C��", "uasset" }, linkPathJson["projectPath"]);
					//�ŏ���ini�t�@�C���̃f�B���N�g�����w��
					//�o�C�i������^�C���Z�b�g���ǂ������m�F����(�^�C���Z�b�g����Ȃ�������x��������)
					//�����N�f�[�^��ǉ�����

					break;
				case '3':	//�p�X����
					OutText("UE4�̃R���e���c�u���E�U�ŃR�s�[�����A�Z�b�g�p�X����͂��Ă��������B", OS_INFO);
					//SetConsoleOutputCP(1252);
					//SetConsoleCP(1252);
					struct IMPORT_DATA_ATTR {
						int isPaperTileSet = -1;	//-1=�s�� 0=�s 1=��
						bool exists = false;
						bool isUEPath = false;		//UE�p�X��(/Game����̃p�X�ɂȂ��Ă��邩)
					};
					while (inputAsset != "-1") {
						//����
						string s;
						getline(cin, s);
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
							OutText("�Ǎ��`��: UE Path");
							attr.isUEPath = true;
							//�A�Z�b�g�̌`����ǂ݂Ƃ�(������)
							//attr.isPaperTileSet = ExtractImportData(UEDirectory + "\\")
						}
						else if(Like(inputAsset, "%:\\%.uasset%")){
							//��΃p�X�̂Ƃ�
							OutText("�Ǎ��`��: Absolute Path");

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

						cout << s << endl;

						//wofstream outfile("testtttst.txt");
						ofstream outfile("testtttst.txt");
						outfile << s;
						outfile.close();


						//cin >> inputAsset;
						//cout << inputAsset << endl;
					}
					//ExtractTexture(inputAsset);
					//OutText(sourcePath + " �� " + to_string(linkPathData[sameFileNameIndexes[inch - '1']]["ue4"]) + "�������N���܂��B", OS_INFO);
				}

			}

		}
	}
	
	return true;
}

void Parser::Parse(string _path, json _data)
{
	//WH�������Ă���
	int height = _data["height"];
	int width = _data["width"];

	string outFile = outDir + filesystem::path(_path).stem().string();
	if (filesystem::exists(outFile + "_output.txt") || filesystem::exists(outFile + ".json")) {
		OutText("�o�̓t�H���_���ɑI�������t�@�C���Ɠ������O�̃t�@�C�������݂��܂��B�i���o�����O���s���܂��B", OS_WARNING);
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
		//�����Ƀ}�b�v�ϊ��@�\
		for (int h = 0; h < height; h++) {
			for (int w = 0; w < width; w++) {
				int index = h * width + w;	//�C���f�b�N�X�ݒ�
				unsigned int tiledValue = _data["layers"][layer]["data"][index];	//TILED�̃f�[�^�l

				//�l��0�̂Ƃ��͉����Ȃ��̂ŃX�L�b�v
				//(�Ō�̒l�̂Ƃ��̂݁A��̏ꍇ�ł�����)
				if (tiledValue != 0) {
					//�f�[�^�ϊ��֐�
					string ueTileset = "";
					int uePackedTileIndex = -1;
					ConvertData(tiledValue, &ueTileset, &uePackedTileIndex);

					//�ϊ����s���ɏo��l���A���Ă����Ƃ������I��
					if (ueTileset != "" && uePackedTileIndex != -1) {
						output << "   AllocatedCells(" << index << ")=(TileSet=PaperTileSet'\"" << ueTileset << "\"',PackedTileIndex=" << uePackedTileIndex << ")\n";
					}
					else {
						OutText("�ϊ��Ɏ��s���܂����B(" + to_string(index) + ")", OS_ERROR);
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

	std::cout << "�����L�[�������Ă�������..." << std::endl;
	_getch(); // �L�[���͂�ҋ@�i���͓��e�͕\������Ȃ��j
	return 0;
}
void Parser::Result() {
	cout << endl << "�������I�����܂����B" << endl;
	End();
}

//�����̏����������Ă���� ���������ł��Ȃ�����
void Parser::ConvertData(unsigned int tiledValue, string* uePath, int* ueTileValue) {
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
				*uePath = linkData[useLinkDataIndexes[tilesetID]]["ue4"];
				*ueTileValue = tiledValue - data["tilesets"][tilesetID]["firstgid"];

				break;
			}
		}
	}
	catch (json::type_error &e) {
		OutText("�}�b�v�f�[�^�܂��̓����N�t�@�C���ɕK�v�ƂȂ�L�[�����݂��Ȃ����A�s���Ȓl�������Ă��܂��B�ϊ��𒆎~���܂��B", OS_ERROR);
		return;
	}
}

string Parser::GetStem(string path)
{
	string ret = filesystem::path(path).stem().string();
	//cout << "�X�e�������F" << path << " -> " << ret << endl;
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
	if (((filesystem::path)(filePath)).extension() == ".json") OutText("JSON�`���ŏo�͂����t�@�C���̊g���q��.json�ł͂���܂���B�g�p����ۂ͂����ӂ��������B", OS_WARNING);
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
	return InputBool("���s���܂����H");
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

string Parser::PGetCurrentDirectory()
{
	char dir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, dir);
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