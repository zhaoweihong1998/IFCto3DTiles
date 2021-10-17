#pragma once
#include "MyMesh.h"
#include <assimp/scene.h>
#include "Option.h"

struct BuildNode
{
	void InitLeaf(int first, int n, const Box3f& b) {
		firstPrimOffset = first;
		nPrimitives = n;
		bounds = b;
		children[0] = children[1] = nullptr;
	}

	void InitInterior(int axis, shared_ptr<BuildNode> c0, shared_ptr<BuildNode> c1) {
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
	shared_ptr<BuildNode> children[2];
	int splitAxis, firstPrimOffset, nPrimitives;
};

class TreeBuilder
{
public:
	TreeBuilder(TileInfo* rootTile);
	~TreeBuilder();
	shared_ptr<BuildNode> getRoot();
	MyMeshInfo* getMeshInfo(int i) {
		return primitives[i];
	}
	void setMinMeshPerNode(int number) {
		maxPrimInNode = number;
	}
	void setThreadNum(int number) {
		nThreads = number;
	}

	void setMethod(int number) {
		method = (SplitMethod)number;
	}
private:
	Point3f Offset(const Point3f& p, const Box3f& bounds) {
		Point3f o = p - bounds.min;
		if (bounds.max.X() > bounds.min.X())o.X() /= bounds.max.X() - bounds.min.X();
		if (bounds.max.Y() > bounds.min.Y())o.Y() /= bounds.max.Y() - bounds.min.Y();
		if (bounds.max.Z() > bounds.min.Z())o.Z() /= bounds.max.Z() - bounds.min.Z();
		return o;
	}
	enum class SplitMethod
	{
		Middle=1, EqualCounts, SAH, HLBVH
	};

	struct PrimitiveInfo
	{
		PrimitiveInfo() {

		}
		PrimitiveInfo(int primitiveNumber, const Box3f& bounds) :
			primitiveNumber(primitiveNumber), bounds(bounds), centroid(bounds.Center()) {}
		int primitiveNumber;
		Box3f bounds;
		Point3f centroid;
	};

	struct MortonPrimitive
	{
		int primitiveIndex;
		uint32_t mortonCode;
	};
	
	struct LBVHTreeLet
	{
		int startIndex, nPrimitives;
		BuildNode* buildNodes;
	};

	std::vector<MyMeshInfo*> primitives;
	SplitMethod method;
	int maxPrimInNode;
	int nThreads;
	shared_ptr<BuildNode> recursiveBuild(std::vector<PrimitiveInfo>& primitiveInfo, int start, int end, std::vector<MyMeshInfo*>& orderedPrims);
	shared_ptr<BuildNode> HLBuild(std::vector<PrimitiveInfo>& primitiveInfo, std::vector<MyMeshInfo*>& orderedPrims);

	void RadixSort(vector<MortonPrimitive>* v);

	BuildNode* emitBVH(const vector<PrimitiveInfo>& primitiveInfo, MortonPrimitive* mortonPrims, int nPrimitives, vector<MyMeshInfo*>& orderedPrims, int& orderedPrimOffset, int bitIndex);

	shared_ptr<BuildNode> buildUpperSAH(vector<BuildNode*>& treeletRoot, int start, int end);
};


class SpatialTree
{
public:
	SpatialTree(const aiScene& mScene, vector<shared_ptr<MyMesh>>& meshes, const Option& op);
	~SpatialTree(){}
	void Initialize();
	TileInfo* GetTilesetInfo();
private:
	shared_ptr<BuildNode> root;
	const aiScene* mScene;
	vector<shared_ptr<MyMesh>>* m_meshes;
	int m_correntDepth;
	int m_treeDepth;
	TileInfo* m_pTileRoot;
	const Option op;
	TreeBuilder *Tree;
private:
	void splitTreeNode(TileInfo* parentTile);
	void recomputeTileBox(TileInfo* parent);
	void buildTree(TileInfo* parent, shared_ptr<BuildNode> node);
};



