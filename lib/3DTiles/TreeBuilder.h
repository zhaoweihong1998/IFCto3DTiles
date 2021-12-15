#pragma once
#include "MyMesh.h"
#include <assimp/scene.h>
#include "Option.h"

struct BVHAccelNode
{
	void InitLeaf(int first, int n, const Box3f& b) {
		firstPrimOffset = first;
		nPrimitives = n;
		bounds = b;
		children[0] = children[1] = nullptr;
	}

	void InitInterior(int axis, shared_ptr<BVHAccelNode> c0, shared_ptr<BVHAccelNode> c1) {
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
	shared_ptr<BVHAccelNode> children[2];
	int splitAxis, firstPrimOffset, nPrimitives;
};

class BVHAccel
{
public:
	BVHAccel(TileInfo* rootTile);
	~BVHAccel();
	shared_ptr<BVHAccelNode> getRoot();
	shared_ptr<MyMesh> getMeshInfo(int i) {
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
		BVHAccelNode* buildNodes;
	};

	std::vector<shared_ptr<MyMesh>> primitives;
	SplitMethod method;
	int maxPrimInNode;
	int nThreads;
	shared_ptr<BVHAccelNode> recursiveBuild(std::vector<PrimitiveInfo>& primitiveInfo, int start, int end, std::vector<shared_ptr<MyMesh>>& orderedPrims);
	shared_ptr<BVHAccelNode> HLBuild(std::vector<PrimitiveInfo>& primitiveInfo, std::vector<shared_ptr<MyMesh>>& orderedPrims);

	void RadixSort(vector<MortonPrimitive>* v);

	BVHAccelNode* emitBVH(const vector<PrimitiveInfo>& primitiveInfo, MortonPrimitive* mortonPrims, int nPrimitives, vector<shared_ptr<MyMesh>>& orderedPrims, int& orderedPrimOffset, int bitIndex);

	shared_ptr<BVHAccelNode> buildUpperSAH(vector<BVHAccelNode*>& treeletRoot, int start, int end);
};

struct KDAccelNode
{
	void InitialLeaf(int* primNums, int np, std::vector<int>* primitiveIndices) 
	{
		flags = 3;
		nPrims |= (np << 2);
		if (np == 0) {
			onePrimitive = 0;
		}
		else if (np == 1) {
			onePrimitive = primNums[0];
		}
		else {
			primitiveindicesOffset = primitiveIndices->size();
			for (int i = 0; i < np; i++) {
				primitiveIndices->push_back(primNums[i]);
			}
		}
	}

	void InitiInteror(int axis, int ac, float s)
	{
		split = s;
		flags = axis;
		aboveChild |= (ac << 2);
	}
	float SplitPos()const { return split; }
	int nPrimitives() const { return nPrims >> 2;}
	int SplitAxis() const { return flags&3; }
	bool IsLeaf() const { return (flags & 3) == 3; }
	int AboveChild() const { return aboveChild >> 2; }
	union {
		float split;
		int onePrimitive;
		int primitiveindicesOffset;
	};

	union {
		int flags;
		int nPrims;
		int aboveChild;
	};
};

class KDTreeAccel
{
public:
	KDTreeAccel(TileInfo* rootTile, int isectCost = 80, int traversalCost = 1,
		float emptyBonus = 0.5, int maxPrims = 800, int _maxDepth = 5);
	~KDTreeAccel() {
		free(nodes);
	}
	void BuildTree();
	KDAccelNode* getNode() { return nodes; }
	int getIndex(int i) { return primitivesIndices[i]; }
	KDAccelNode* getNode(int i) { return &nodes[i]; }
	void setMaxMesh(int i) {
		maxPrims = i;
	}
	void setMaxDepth(int i) {
		maxDepth = i;
	}
private:
	const int isectCost, traversalCost;
	int maxPrims;
	const float emptyBonus;
	std::vector<shared_ptr<MyMesh>> primitives;
	std::vector<int> primitivesIndices;
	KDAccelNode* nodes;
	int nAllocedNodes, nextFreeNode;
	Box3f bounds;
	int maxDepth;

	enum class EdgeType
	{
		Start, End
	};

	struct BoundEdge {
		float t;
		int primNum;
		EdgeType type;
		BoundEdge(){}
		BoundEdge(float t, int primNum, bool starting):t(t),primNum(primNum) {
			type = starting ? EdgeType::Start : EdgeType::End;
		}
	};
	void buildTree(int nodeNum, const Box3f nodeBounds, const std::vector<Box3f>& allPrimimBounds, 
		int* primNums, int nPrimitives, int depth, 
		const std::unique_ptr<BoundEdge[]> edges[3], int* prims0, int* prims1, int badRefines);
};

class TreeBuilder
{
public:
	TreeBuilder(vector<shared_ptr<MyMesh>>& meshes, const Option& op);
	~TreeBuilder(){}
	void Initialize();
	TileInfo* GetTilesetInfo();
private:
	shared_ptr<BVHAccelNode> root;
	//const aiScene* mScene;
	vector<shared_ptr<MyMesh>>* m_meshes;
	int m_correntDepth;
	int m_treeDepth;
	TileInfo* m_pTileRoot;
	TileInfo* BigMeshes;
	const Option op;
	BVHAccel *Tree;
	KDTreeAccel* kdTree;
	friend class KDTreeAccel;
	friend class BVHAccel;
private:
	void splitTreeNode(TileInfo* parentTile);
	void recomputeTileBox(TileInfo* parent);
	void buildTree(TileInfo* parent, shared_ptr<BVHAccelNode> node);
	void buildTree(TileInfo* parent, int index);
	void preMeshes(TileInfo* BigMeshes, TileInfo* root, Box3f* box, vector<shared_ptr<MyMesh>>* meshes,bool detach=false);
	void preMeshes(TileInfo* BigMeshes, Box3f* box, vector<shared_ptr<MyMesh>>* meshes);
	void levelTree();
};
