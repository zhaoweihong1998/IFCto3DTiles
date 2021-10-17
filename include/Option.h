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
	int Method;
	int nThreads;
	Option() {
		Max_Mesh_per_Node = 800;
		Min_Mesh_Per_Node = 300;
		Level = 0;
		filename = "";
		binary = true;
		log = false;
		outputDir = "output";
		Method = true;
		nThreads = 1;
	}
};