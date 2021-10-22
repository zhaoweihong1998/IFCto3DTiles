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

struct kdAccelNode
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

class kdTreeAccel
{
public:
	kdTreeAccel(TileInfo* rootTile, int isectCost = 80, int traversalCost = 1,
		float emptyBonus = 0.5, int maxPrims = 800, int _maxDepth = 5):isectCost(isectCost), 
		traversalCost(traversalCost), maxPrims(maxPrims), emptyBonus(emptyBonus),nodes(nullptr) {
		nAllocedNodes = nextFreeNode = 0;
		for (int i = 0; i < rootTile->myMeshInfos.size(); i++) {
			primitives.push_back(&rootTile->myMeshInfos[i]);
		}
		if (_maxDepth <= 0)
			this->maxDepth = std::round(8 + 1.3f * _Bit_scan_reverse(primitives.size()));
		else this->maxDepth = _maxDepth;
		bounds.min.X() = bounds.min.Y() = bounds.min.Z() = INFINITY;
		bounds.max.X() = bounds.max.Y() = bounds.max.Z() = -INFINITY;
	}
	~kdTreeAccel() {
		free(nodes);
	}
	void BuildTree();
	kdAccelNode* getNode() { return nodes; }
	int getIndex(int i) { return primitivesIndices[i]; }
	kdAccelNode* getNode(int i) { return &nodes[i]; }
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
	std::vector<MyMeshInfo*> primitives;
	std::vector<int> primitivesIndices;
	kdAccelNode* nodes;
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
	TileInfo* BigMeshes;
	const Option op;
	TreeBuilder *Tree;
	kdTreeAccel* kdTree;
private:
	void splitTreeNode(TileInfo* parentTile);
	void recomputeTileBox(TileInfo* parent);
	void buildTree(TileInfo* parent, shared_ptr<BuildNode> node);
	void buildTree(TileInfo* parent, int index);
};



