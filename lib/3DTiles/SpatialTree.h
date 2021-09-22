#pragma once
#include "MyMesh.h"
#include <assimp/scene.h>

struct Option
{
	int Max_Vertices_per_Node;
	int Min_Mesh_Per_Node;
	int Level;
	string filename;
	bool binary;
	bool log;
	Option() {
		Max_Vertices_per_Node = 500;
		Min_Mesh_Per_Node = 10;
		Level = 3;
		filename = "";
		binary = false;
		log = false;
	}
};

class SpatialTree
{
public:
	SpatialTree(const aiScene& mScene, vector<MyMesh*>& meshes, const Option& op);
	~SpatialTree(){}
	void Initialize();
	TileInfo* GetTilesetInfo();
private:
	const aiScene* mScene;
	vector<MyMesh*>* m_meshes;
	int m_correntDepth;
	int m_treeDepth;
	TileInfo* m_pTileRoot;
	const Option op;
private:
	void splitTreeNode(TileInfo* parentTile);
	void recomputeTileBox(TileInfo* parent);
};

