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
	std::string outputDir;
	Option() {
		Max_Vertices_per_Node = 500;
		Min_Mesh_Per_Node = 10;
		Level = 0;
		filename = "";
		binary = false;
		log = false;
		outputDir = "output";
	}
};

struct BuildNode
{
	void InitLeaf(int first, int n, const Box3f& b) {
		firstPrimOffset = first;
		nPrimitives = n;
		bounds = b;
		children[0] = children[1] = nullptr;
	}

	void InitInterior(int axis, BuildNode* c0, BuildNode* c1) {
		children[0] = c0;
		children[1] = c1;
		bounds.min.X() = bounds.min.Y() = bounds.min.Z() = INFINITY;
		bounds.max.X() = bounds.max.Y() = bounds.max.Z() = -INFINITY;
		bounds.Add(c0->bounds);
		bounds.Add(c1->bounds);
		splitAxis = axis;
		nPrimitives = 0;
	}
	Box3f bounds;
	BuildNode* children[2];
	int splitAxis, firstPrimOffset, nPrimitives;
};

class SpatialTree
{
public:
	SpatialTree(const aiScene& mScene, vector<MyMesh*>& meshes, const Option& op);
	~SpatialTree(){}
	void Initialize();
	TileInfo* GetTilesetInfo();
private:
	BuildNode* root;
	const aiScene* mScene;
	vector<MyMesh*>* m_meshes;
	int m_correntDepth;
	int m_treeDepth;
	TileInfo* m_pTileRoot;
	const Option op;
private:
	void splitTreeNode(TileInfo* parentTile);
	void recomputeTileBox(TileInfo* parent);
	void buildTree(TileInfo* parent,BuildNode* node);
};



class TreeBuilder
{
public:
	TreeBuilder(TileInfo* rootTile);
	~TreeBuilder();
	BuildNode* getRoot();
	MyMesh* getMesh(int i) {
		return primitives[i];
	}
private:
	Point3f Offset(const Point3f& p,const Box3f& bounds) {
		Point3f o = p - bounds.min;
		if (bounds.max.X() > bounds.min.X())o.X() /= bounds.max.X() - bounds.min.X();
		if (bounds.max.Y() > bounds.min.Y())o.Y() /= bounds.max.Y() - bounds.min.Y();
		if (bounds.max.Z() > bounds.min.Z())o.Z() /= bounds.max.Z() - bounds.min.Z();
		return o;
	}
	enum class SplitMethod
	{
		Middle, EqualCounts,SAH
	};

	struct PrimitiveInfo
	{
		PrimitiveInfo() {
			
		}
		PrimitiveInfo(int primitiveNumber, const Box3f& bounds):
			primitiveNumber(primitiveNumber),bounds(bounds),centroid(bounds.Center()){}
		int primitiveNumber;
		Box3f bounds;
		Point3f centroid;
	};
	std::vector<MyMesh*> primitives;
	SplitMethod method;
	int maxPrimInNode;
	BuildNode* recursiveBuild(std::vector<PrimitiveInfo>& primitiveInfo, int start, int end, std::vector<MyMesh*>& orderedPrims);
	
};