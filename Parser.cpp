#define _CRT_SECURE_NO_WARNINGS
#include "Parser.h"

#include <iostream>
#include <cmath>



using namespace std;

Parser::Parser()
{
}

bool Parser::Init(char* exePath) {
	//���s�t�@�C���̃f�B���N�g�����擾
	std::filesystem::path path = exePath;
	outDir = path.parent_path().string() + "\\output\\";
	//Tiled��UE4�̃����N�t�@�C����Ǎ�
	string lpPath = path.parent_path().string() + "\\linkPath.json";
	if (filesystem::exists(lpPath)) {
		std::ifstream lp(lpPath);

		try { linkPathData = json::parse(lp); }
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
		OutText(parsePaths[i] + " ��ϊ����Ă��܂�...(" + to_string(i + 1) + "/" + to_string(parsePaths.size()) + ")", OS_INFO);

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
		SelectFile(paths);
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

void Parser::SelectFile(vector<string>* paths)
{
	char fileName[MAX_PATH] = "";  //�t�@�C����������ϐ�

	//�u�t�@�C�����J���v�_�C�A���O�̐ݒ�
	OPENFILENAME ofn;                         	//���O�����ĕۑ��_�C�A���O�̐ݒ�p�\����
	ZeroMemory(&ofn, sizeof(ofn));            	//�\���̏�����
	ofn.lStructSize = sizeof(OPENFILENAME);   	//�\���̂̃T�C�Y
	ofn.lpstrFilter = TEXT("�}�b�v�f�[�^(*.json)\0*.json\0")			//�����t�@�C���̎��
		TEXT("���ׂẴt�@�C��(*.*)\0*.*\0\0");                     //����
	ofn.lpstrFile = fileName;               	//�t�@�C����
	ofn.nMaxFile = MAX_PATH;               	//�p�X�̍ő啶����
	//ofn.Flags = OFN_OVERWRITEPROMPT;   		//�t���O�i�����t�@�C�������݂�����㏑���m�F�j
	ofn.lpstrDefExt = "json";                  	//�f�t�H���g�g���q

	//�u�t�@�C�����J���v�_�C�A���O
	BOOL selFile;
	selFile = GetOpenFileName(&ofn);

	//�L�����Z�������璆�f
	if (selFile == FALSE) return;

	//���������@�킩���̂ō���1�����v�b�V��
	paths->push_back(fileName);

}

bool Parser::Read(string _path, json* data)
{

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
		try { *data = json::parse(f); }
		catch (json::parse_error e) {
			OutText("�}�b�v�f�[�^�̓Ǎ��Ɏ��s���܂����F" + (string)(e.what()), OS_ERROR);
			return false;
		}

	//}
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
				//�Ō�̒l�̂Ƃ��̂݁A��̏ꍇ������
				//�l��0�̂Ƃ��͉����Ȃ��̂ŃX�L�b�v
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
	json convJson;
	convJson = {
		{"height", _data["height"]},
		{"width", _data["width"]},
		{"orientation", _data["orientation"]},
		{"tileheight", _data["tileheight"]},
		{"tilewidth", _data["tilewidth"]},
		{"version", _data["version"]}
	};
	ofstream outJson(outFile + ".json");
	outJson << convJson;
	outJson.close();
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
	//int flipFlag = (tiledValue >> 29) & 0b111;		//29�r�b�g�V�t�g����3�r�b�g���(30-32)

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