#pragma once

#include "./Include/json.hpp"
#include <iostream>

#include <filesystem>
#include <vector>
#include <tchar.h>
#include <bitset>
#include <sstream>
#include <Windows.h>
#include <conio.h>	//�L�[�����v��
#include <locale>
#include <string>
using std::bitset;

using std::vector;
using json = nlohmann::json;
using std::string;
using std::wstring;

using namespace std;
class Parser
{
public:
	//�R���X�g���N�^
	Parser();
	//���C�������@�\
	bool Process(int argc, wchar_t* argv[]);

private:
	wstring parentDir;
	json linkPathJson;

	json data;	//�Ǎ�json�f�[�^�̊i�[�ϐ�
	vector<int> useLinkDataIndexes;

	//������
	wstring outDir;
	bool Init(wchar_t* exePath);

	//�ϊ��f�[�^�n
	vector<wstring> parsePaths;	//�ϊ��f�[�^�̃p�X���i�[����z��
	bool StoreParseFile(int argc, wchar_t* argv[], vector<wstring>* paths);	//�Ǎ�-��{

	struct STR_FILTER {
		wstring descr = L"";
		wstring ext = L"*";
	};
	void SelectFile(vector<wstring>* storePaths, vector<STR_FILTER> filters, wstring dir = L"", bool enAllFile = true);		//�����I���A�����t�B���^
	void SelectFile(vector<wstring>* storePaths, STR_FILTER filter, wstring dir = L"", bool enAllFile = true);				//�����I���A�P��t�B���^
	void SelectFile(wstring* storePath, vector<STR_FILTER> filters, wstring dir = L"", bool enAllFile = true);											//�P��I���A�����t�B���^
	void SelectFile(wstring* storePath, STR_FILTER filter, wstring dir = L"", bool enAllFile = true);													//�P��I���A�P��t�B���^
	vector<wstring> SelectFile_proc(vector<STR_FILTER> filters, wstring dir, bool enAllFile, bool isSingleFile);	//����

	bool Read(wstring _path, json* _data);								//�}�b�v�f�[�^��ǂݍ��݃f�[�^���i�[����
	void Parse(wstring _path, json data);								//�ϊ�

	bool End();
	void Result();
	void ConvertData(unsigned int tiledValue, wstring* uePath, int* ueTileValue);


	wstring GetStem(wstring path);


	string ExtractTexture(string filePath_tileset);
	string ExtractImportData(string filePath_);

	void AddLinkDataW(wstring tiled_sourcePath, wstring ue4_path);

	//�ϊ��֐�
	void StoreWStr(wstring* wstr, json* j);
	wchar_t* GetWC(const char* c);
	string ConvStr(wstring& src);
	// ===================== �ėp���o�͊֐� =====================
	//�L�[����(isgraph)
	char GetKey(wstring descr = L"");

	//Y/N����
	bool InputBool(wstring descr = L"");

	//JSON�o��
	void OutputJson(wstring filePath, json content);

	// ===================== �W�����o�͊֐� =====================
	enum OUTPUT_STATE {
		OS_NONE,
		OS_INFO,
		OS_WARNING,
		OS_ERROR
	};
	//��ԕt���o��
	//void OutText(string str, OUTPUT_STATE outState = OS_NONE);
	void OutText(wstring wstr, OUTPUT_STATE outState = OS_NONE);

	//�������f�E�x��
	bool BreakNIsContinue(wstring warnStr);

	//���s
	void Br() { std::wcout << std::endl; }

	//string�z���S�o��
	void PrintStrList(vector<wstring>* descrList, int startNum = 0);
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
	string PGetCurrentDirectoryA();
	wstring PGetCurrentDirectoryW();
};

