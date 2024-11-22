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
	//�t�@�C���w��
	string filePath = "";
	// ������1�ȏ゠��ꍇ�i0�Ԗڂ͎��s�t�@�C�����g�̃p�X�j
	if (argc > 1) {
		if (argc > 2) {
			OutText("2�ȏ�̃t�@�C�����h���b�v���Ă��܂��B�ŏ��ɔF�������t�@�C���������I�ɕϊ��Ώۂɂ��܂����B", OS_WARNING);
		}
		filePath = argv[1];
		OutText("�t�@�C���Ǎ�:" + filePath, OS_INFO);
	}
	else {
		OutText("�t�@�C�����I������Ă��܂���B�ϊ�����t�@�C����I�����Ă��������B", OS_INFO);
		SelectFile(&filePath);
		OutText("conv:" + filePath);
		if (filePath == "") {
			OutText("�t�@�C�����I������܂���ł����B�I�����܂��B", OS_INFO);
			return End();
		}
	}

	//json�Ǎ�
	std::ifstream f(filePath);
	//���g�̃f�B���N�g���Ƒg�ݍ��킹�Ȃ��ƃG����
	std::filesystem::path path(argv[0]);
	string lpPath = path.parent_path().string() + "\\linkPath.json";
	cout << lpPath << endl;
	std::ifstream lp(lpPath);
	try {
		data = json::parse(f);
	}
	catch (json::parse_error e) { OutText("�}�b�v�f�[�^�̓ǂݍ��݂Ɏ��s���܂����B(" + (string)(e.what()), OS_ERROR); }
	try {
		linkPathData = json::parse(lp);
	}
	catch (json::parse_error e) { cout << e.what() << endl; }
}

bool Parser::Process()
{
	//WH�������Ă���
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

		//�����Ƀ}�b�v�ϊ��@�\
		for (int h = 0; h < height; h++) {
			for (int w = 0; w < width; w++) {
				int index = h * width + w;	//�C���f�b�N�X�ݒ�
				unsigned int tiledValue = data["layers"][layer]["data"][index];	//TILED�̃f�[�^�l
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
	if (selFile == FALSE) return false;

	*path = fileName;

}

bool Parser::End() {

	std::cout << "�����L�[�������Ă�������..." << std::endl;
	_getch(); // �L�[���͂�ҋ@�i���͓��e�͕\������Ȃ��j
	return 0;
}
//void Parser::Result() {
//	cout << "������" << endl;
//	End();
//}

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
			break;
		}
	}

	////�l��2^29�ȏ�(�Q�i����30�r�b�g�ȏ�)�̂Ƃ��AUE4�̃t�H�[�}�b�g�ɕϊ������l�����Z
	//if (flipFlag > 0) {
	//	if (flipFlag & 1) {
	//		*ueTileValue += pow(2, 29);
	//		//�Δ��]�t���O
	//	}
	//	if ((flipFlag >> 1) & 1) {
	//		*ueTileValue += pow(2, 30);
	//		//�㉺��]�t���O
	//	}
	//	if ((flipFlag >> 2) & 1) {
	//		*ueTileValue -= pow(2, 31);
	//		//���E���]�t���O
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