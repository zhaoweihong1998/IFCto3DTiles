#pragma once
#include <string>

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
	Option() {
		MaxMeshPerNode = 800;
		Level = 5;
		Filename = "";
		Binary = true;
		Log = true;
		OutputDir = "output";
		Method = 3;
		nThreads = 8;
		detach = false;
	}
};