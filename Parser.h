#pragma once

#include "./Include/json.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

#include <bitset>
#include <sstream>
#include <Windows.h>
#include <conio.h>	//�L�[�����v��
#include <locale>
using std::bitset;

using std::vector;
using json = nlohmann::json;
using std::string;

class Parser
{
public:
	//�R���X�g���N�^
	Parser();
	//���C�������@�\
	bool Process(int argc, char* argv[]);

private:
	string parentDir;
	json linkPathJson;

	json data;	//�Ǎ�json�f�[�^�̊i�[�ϐ�
	vector<int> useLinkDataIndexes;

	//������
	string outDir;
	bool Init(char* exePath);

	//�ϊ��f�[�^�n
	vector<string> parsePaths;	//�ϊ��f�[�^�̃p�X���i�[����z��
	bool StoreParseFile(int argc, char* argv[], vector<string>* paths);	//�Ǎ�-��{

	struct STR_FILTER {
		string descr = "";
		string ext = "*";
	};
	void SelectFile(vector<string>* storePaths, vector<STR_FILTER> filters, string dir = "", bool enAllFile = true);		//�����I���A�����t�B���^
	void SelectFile(vector<string>* storePaths, STR_FILTER filter, string dir = "", bool enAllFile = true);				//�����I���A�P��t�B���^
	void SelectFile(string* storePath, vector<STR_FILTER> filters, string dir = "", bool enAllFile = true);											//�P��I���A�����t�B���^
	void SelectFile(string* storePath, STR_FILTER filter, string dir = "", bool enAllFile = true);													//�P��I���A�P��t�B���^
	vector<string> SelectFile_proc(vector<STR_FILTER> filters, string dir, bool enAllFile, bool isSingleFile);	//����

	bool Read(string _path, json* _data);								//�}�b�v�f�[�^��ǂݍ��݃f�[�^���i�[����
	void Parse(string _path, json data);								//�ϊ�

	bool End();
	void Result();
	void ConvertData(unsigned int tiledValue, string* uePath, int* ueTileValue);


	string GetStem(string path);


	string ExtractTexture(string filePath_tileset);
	string ExtractImportData(string filePath_);

	// ===================== �ėp���o�͊֐� =====================
	//�L�[����(isgraph)
	char GetKey(string descr = "");

	//Y/N����
	bool InputBool(string descr = "");

	//JSON�o��
	void OutputJson(string filePath, json content);

	// ===================== �W�����o�͊֐� =====================
	enum OUTPUT_STATE {
		OS_NONE,
		OS_INFO,
		OS_WARNING,
		OS_ERROR
	};
	//��ԕt���o��
	void OutText(string str, OUTPUT_STATE outState = OS_NONE);

	//�������f�E�x��
	bool BreakNIsContinue(string warnStr);

	//���s
	void Br() { std::cout << std::endl; }

	//string�z���S�o��
	void PrintStrList(vector<string>* descrList, int startNum = 0);
	// ===================== �ėp�֐� =====================

	//�l���͈͓���
	template <class T>
	bool Between(T value, T min, T max) { return (min <= value && value <= max); }

	//SQL��In��Ɠ���
	template <class T>
	bool In(T val, vector<T> search) {
		for (auto& word : search) {
			if (val == word)return true;
		}
		return false;
	}

	//SQL��LIKE��Ɠ���
	bool Like(string val, string search);

	//�����ɂ��� �^�����̂܂�ܕԂ�����int�Ȃǂ͎����؂�̂�
	template <class T>
	T Half(T value) { return (value / 2.0f); }

	//2�{�ɂ���
	template <class T>
	T Twice(T value) { return (value * 2); }

	//������������
	bool IsEven(int value) { return (value % 2 == 0); }

	//0d to 0b �o�C�i���ϊ�
	string toBinary(unsigned int n);

	//�J�����g�f�B���N�g���̎擾
	string PGetCurrentDirectory();
};

