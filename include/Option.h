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
	bool simplify;
	float simplifyTarget;
	Option() {
		MaxMeshPerNode = 800;
		Level = 5;
		Filename = "";
		Binary = true;
		Log = false;
		OutputDir = "output";
		Method = 3;
		nThreads = 8;

		detach = false;
		simplify = false;
		simplifyTarget = 0.5;
	}
};