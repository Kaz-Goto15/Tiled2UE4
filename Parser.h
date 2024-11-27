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
	json linkPathData;

	json data;	//�Ǎ�json�f�[�^�̊i�[�ϐ�

	enum OUTPUT_STATE {
		OS_NONE,
		OS_INFO,
		OS_WARNING,
		OS_ERROR
	};

	//������
	string exeDir;
	bool Init(char* exePath);

	//�ϊ��f�[�^�n
	vector<string> parsePaths;	//�ϊ��f�[�^�̃p�X���i�[����z��
	bool StoreParseFile(int argc, char* argv[], vector<string>* paths);	//�Ǎ�-��{
	void SelectFile(vector<string>* paths);								//�Ǎ�-�E�B���h�E�I��
	bool Read(string _path, json* _data);								//�}�b�v�f�[�^��ǂݍ��݃f�[�^���i�[����
	void Parse(string _path, json data);								//�ϊ�

	bool End();
	void Result();
	void ConvertData(unsigned int tiledValue, string* uePath, int* ueTileValue);
	void OutText(string str, OUTPUT_STATE outState = OS_NONE);


	std::string toBinary(unsigned int n)
	{
		std::string r;
		while (n != 0) { r = (n % 2 == 0 ? "0" : "1") + r; n /= 2; }
		return r;
	}
};

