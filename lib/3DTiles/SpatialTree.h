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
		Middle, EqualCounts, SAH
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
	shared_ptr<BuildNode> recursiveBuild(std::vector<PrimitiveInfo>& primitiveInfo, int start, int end, std::vector<MyMeshInfo*>& orderedPrims);
	shared_ptr<BuildNode> HLBuild(std::vector<PrimitiveInfo>& primitiveInfo, std::vector<MyMeshInfo*>& orderedPrims);

	static void RadixSort(vector<MortonPrimitive>* v) {
		vector<MortonPrimitive> tempVector(v->size());
		constexpr int bitsPerPass = 6;
		constexpr int nBits = 30;
		constexpr int nPasses = nBits / bitsPerPass;
		for (int pass = 0; pass < nPasses; ++pass) {
			int lowBits = pass * bitsPerPass;
			vector<MortonPrimitive>& in = (pass & 1) ? tempVector : *v;
			vector<MortonPrimitive>& out = (pass & 1) ? *v : tempVector;
			constexpr int nBuckets = 1 << bitsPerPass;
			int bucketsCount[nBuckets] = { 0 };
			constexpr int bitMask = (1 << bitsPerPass) - 1;
			for (const MortonPrimitive& mp : in) {
				int bucket = (mp.mortonCode >> lowBits) & bitMask;
				++bucketsCount[bucket];
			}

			int outIndex[nBuckets];
			outIndex[0] = 0;
			for (int i = 1; i < nBuckets; i++) {
				outIndex[i] = outIndex[i - 1] + bucketsCount[i - 1];
			}

			for (const MortonPrimitive& mp : in) {
				int bucket = (mp.mortonCode >> lowBits) & bitMask;
				out[outIndex[bucket]++] = mp;
			}
		}

		if (nPasses&1) {
			std::swap(*v, tempVector);
		}
	}

	BuildNode* emitBVH(const vector<PrimitiveInfo>& primitiveInfo, MortonPrimitive* mortonPrims, int nPrimitives, vector<MyMeshInfo*>& orderedPrims, int& orderedPrimOffset, int bitIndex) {
		if (bitIndex == -1 || nPrimitives < maxPrimInNode) {
			BuildNode* node = new BuildNode();
			Box3f bounds;
			bounds.min.X() = bounds.min.Y() = bounds.min.Z() = INFINITY;
			bounds.max.X() = bounds.max.Y() = bounds.max.Z() = -INFINITY;
			int firstPrimOffset = orderedPrimOffset;
			orderedPrimOffset += nPrimitives;
			for (int i = 0; i < nPrimitives; i++) {
				int primitiveIndex = mortonPrims[i].primitiveIndex;
				orderedPrims[firstPrimOffset + i] = primitives[primitiveIndex];
				bounds.Add(primitiveInfo[primitiveIndex].bounds);
			}
			node->InitLeaf(firstPrimOffset, nPrimitives, bounds);
			return node;
		}
		else {
			int mask = 1 << bitIndex;
			if ((mortonPrims[0].mortonCode & mask) == (mortonPrims[nPrimitives - 1].mortonCode & mask))return emitBVH(primitiveInfo, mortonPrims, nPrimitives,
				orderedPrims, orderedPrimOffset, bitIndex - 1);
			int searchStart = 0, searchEnd = nPrimitives - 1;
			while (searchStart + 1 != searchEnd) {
				int mid = (searchStart + searchEnd) / 2;
				if ((mortonPrims[searchStart].mortonCode & mask) == (mortonPrims[mid].mortonCode & mask)) {
					searchStart = mid;
				}
				else {
					searchEnd = mid;
				}
			}
			int splitOffset = searchEnd;
			BuildNode* node = new BuildNode();
			BuildNode* lbvh[2] = {
				emitBVH(primitiveInfo,mortonPrims,splitOffset,orderedPrims, orderedPrimOffset,bitIndex - 1),
				emitBVH(primitiveInfo,&mortonPrims[splitOffset],nPrimitives - splitOffset,orderedPrims, orderedPrimOffset,bitIndex - 1)
			};
			int axis = bitIndex % 3;
			shared_ptr<BuildNode> ptr1(lbvh[0]);
			shared_ptr<BuildNode> ptr2(lbvh[1]);
			node->InitInterior(axis, ptr1,ptr2);
			return node;
		}
	}

	shared_ptr<BuildNode> buildUpperSAH(vector<BuildNode*>& treeletRoot, int start, int end) {
		int nPrimitive = end - start;
		if (nPrimitive == 1) {
			return shared_ptr<BuildNode>(treeletRoot[start]);
		}
		else {
			shared_ptr<BuildNode> node(new BuildNode());
			Box3f bounds;
			bounds.min.X() = bounds.min.Y() = bounds.min.Z() = INFINITY;
			bounds.max.X() = bounds.max.Y() = bounds.max.Z() = -INFINITY;
			for (int i = start; i < end; ++i) {
				bounds.Add(treeletRoot[i]->bounds);
			}
			Box3f centroidBounds;
			centroidBounds.min.X() = centroidBounds.min.Y() = centroidBounds.min.Z() = INFINITY;
			centroidBounds.max.X() = centroidBounds.max.Y() = centroidBounds.max.Z() = -INFINITY;
			for (int i = start; i < end; i++) {
				centroidBounds.Add(treeletRoot[i]->bounds.Center());
			}
			int dim = centroidBounds.MaxDim();
			int mid = (start + end) / 2;
			if (nPrimitive <= 4) {
				/*int mid = (start + end) / 2;*/
				std::nth_element(&treeletRoot[start], &treeletRoot[mid], &treeletRoot[end - 1] + 1,
					[dim](const BuildNode* a, const BuildNode* b) {
						return a->bounds.Center()[dim] < b->bounds.Center()[dim];
					});
			}
			else 
			{
				constexpr int nBuckets = 12;
				struct BucketInfo
				{
					int count = 0;
					Box3f bounds;
				};
				BucketInfo buckets[nBuckets];
				for (int i = 0; i < 12; i++) {
					buckets[i].bounds.min.X() = buckets[i].bounds.min.Y() = buckets[i].bounds.min.Z() = INFINITY;
					buckets[i].bounds.max.X() = buckets[i].bounds.max.Y() = buckets[i].bounds.max.Z() = -INFINITY;
				}
				for (int i = start; i < end; i++) {
					int b = nBuckets * Offset(treeletRoot[i]->bounds.Center(), centroidBounds)[dim];
					if (b == nBuckets)b = nBuckets - 1;
					buckets[b].count++;
					buckets[b].bounds.Add(treeletRoot[i]->bounds);
				}
				float cost[nBuckets - 1];
				for (int i = 0; i < nBuckets - 1; ++i) {
					Box3f b0, b1;
					b0.min.X() = b0.min.Y() = b0.min.Z() = INFINITY;
					b0.max.X() = b0.max.Y() = b0.max.Z() = -INFINITY;
					b1.min.X() = b1.min.Y() = b1.min.Z() = INFINITY;
					b1.max.X() = b1.max.Y() = b1.max.Z() = -INFINITY;
					int count0 = 0, count1 = 0;
					for (int j = 0; j <= i; j++) {
						b0.Add(buckets[j].bounds);
						count0 += buckets[j].count;
					}
					for (int j = i + 1; j < nBuckets; j++) {
						b1.Add(buckets[j].bounds);
						count1 += buckets[j].count;
					}
					cost[i] = 0.125f + (count0 * b0.Volume() + count1 * b1.Volume()) / bounds.Volume();
				}
				float minCost = cost[0];
				int minCostSplit = 0;
				for (int i = 1; i < nBuckets - 1; i++) {
					if (cost[i] < minCost) {
						minCost = cost[i];
						minCostSplit = i;
					}
				}
				/*float leafCost = nPrimitive;
				if (nPrimitive > 20 || minCost < leafCost) {*/
				BuildNode** pmid = std::partition(&treeletRoot[start], &treeletRoot[end - 1] + 1,
					[=](const BuildNode* pi) {
						int b = nBuckets * Offset(pi->bounds.Center(), centroidBounds)[dim];
						if (b == nBuckets)b = nBuckets - 1;
						return b <= minCostSplit;
					});
				mid = pmid - &treeletRoot[0];
				/*}
				else*/
				//{
					/*node->InitLeaf(start, nPrimitive, bounds);
					return node;*/
				//}
			}
			node->InitInterior(dim, buildUpperSAH(treeletRoot, start, mid), buildUpperSAH(treeletRoot, mid, end));
			return node;
		}

		

		
	}
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



