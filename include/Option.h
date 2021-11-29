#pragma once
#include <string>
#if defined(_MSC_VER)
#include <direct.h>
#include <io.h>
#define GetCurrentDir _getcwd
#elif defined(__unix__)
#include <unistd.h>
#define GetCurrentDir getcwd
#else
#endif



struct Option
{
	int MaxMeshPerNode;
	int Level;
	std::string Filename;
	bool Binary;
	bool Log;
	std::string OutputDir;
	int Method;
	int nThreads;
	bool detach;
	bool simplify;
	float simplifyTarget;
	bool merged;
	Option() {
		MaxMeshPerNode = 25;
		Level = 5;
		Filename = "";
		Binary = false;
		Log = true;
		OutputDir = "./output/";
		Method = 3;
		nThreads = 8;
		detach = false;
		simplify = false;
		simplifyTarget = 0.5;
		merged = false;
	}
	std::string get_current_directory()
	{
		char buff[250];
		GetCurrentDir(buff, 250);
		string current_working_directory(buff);
		return current_working_directory;
	}

	void makedir(string name) {
		std::string pre = get_current_directory();
		pre += "/";
		pre += name;
		if (access(pre.c_str(), 0)) {
			std::string command = "mkdir " + pre;
			system(command.c_str());
		}
	}
};