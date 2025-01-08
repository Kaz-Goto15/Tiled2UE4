#define _CRT_SECURE_NO_WARNINGS
#include "jsonTest.h"

#include <iostream>
#include <cmath>



using namespace std;

jsonTest::jsonTest()
{
}

bool jsonTest::Process(char* exePath)
{	//���s�t�@�C���̃f�B���N�g�����擾
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

	cout << linkPathData;
	return false;
}

void jsonTest::OutText(string str, OUTPUT_STATE outState) {
	switch (outState)
	{
	case OS_NONE:		cout << " ";						break;
	case OS_INFO:		cout << "[INFO] ";					break;
	case OS_WARNING:	cout << "\033[33m" << "[WARNING] ";	break;
	case OS_ERROR:		cout << "\033[31m" << "[ERROR] ";	break;
	}

	cout << str << "\033[0m" << endl;
}