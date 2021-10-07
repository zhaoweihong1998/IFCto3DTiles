#pragma once
#include <string>

struct Option
{
	int Max_Mesh_per_Node;
	int Min_Mesh_Per_Node;
	int Level;
	std::string filename;
	bool binary;
	bool log;
	std::string outputDir;
	bool newMethod;
	int nThreads;
	Option() {
		Max_Mesh_per_Node = 500;
		Min_Mesh_Per_Node = 300;
		Level = 0;
		filename = "";
		binary = true;
		log = false;
		outputDir = "output";
		newMethod = true;
		nThreads = 1;
	}
};