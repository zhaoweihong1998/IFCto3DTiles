#include "SpatialTree.h"

SpatialTree::SpatialTree(const aiScene& mScene, vector<shared_ptr<MyMesh> >& meshes, const Option& op):
	op(op)
{
	this->mScene = &mScene;
	this->m_meshes = &meshes;
	this->m_pTileRoot = new TileInfo();
	this->m_correntDepth = 0;
	this->m_treeDepth = 0;
	root = nullptr;
	Tree = nullptr;
}

void SpatialTree::Initialize()
{
	Box3f* sceneBox = new Box3f();
	sceneBox->min.X() = sceneBox->min.Y() = sceneBox->min.Z() = INFINITY;
	sceneBox->max.X() = sceneBox->max.Y() = sceneBox->max.Z() = -INFINITY;
	m_pTileRoot->originalVertexCount = 0;
	for (int i = 0; i < m_meshes->size(); i++) {
		sceneBox->Add((*m_meshes)[i]->bbox);
		MyMeshInfo meshInfo;
		meshInfo.myMesh = (*m_meshes)[i];
		meshInfo.material = (*m_meshes)[i]->maxterialIndex;
		m_pTileRoot->myMeshInfos.push_back(meshInfo);
		m_pTileRoot->originalVertexCount += meshInfo.myMesh->vn;
	}
	m_pTileRoot->boundingBox = sceneBox;
	m_pTileRoot->parent = nullptr;
	if(m_pTileRoot->myMeshInfos.size()>op.Max_Mesh_per_Node)splitTreeNode(m_pTileRoot);
}

TileInfo* SpatialTree::GetTilesetInfo()
{
	if (!op.newMethod) {
		if (m_treeDepth < op.Level) {
			for (int i = 0; i < op.Level - m_treeDepth; ++i) {
				TileInfo* tileInfo = new TileInfo;
				tileInfo->boundingBox = m_pTileRoot->boundingBox;
				tileInfo->myMeshInfos = m_pTileRoot->myMeshInfos;
				tileInfo->children.push_back(m_pTileRoot);
				m_pTileRoot = tileInfo;
			}
			m_treeDepth = op.Level;
		}
		recomputeTileBox(m_pTileRoot);
		return m_pTileRoot;
	}
	else {
		recomputeTileBox(m_pTileRoot);
		return m_pTileRoot;
	}
}

bool myCompareX(MyMeshInfo& a, MyMeshInfo& b) {
	return a.myMesh->bbox.Center().V()[0] < b.myMesh->bbox.Center().V()[0];
}

bool myCompareY(MyMeshInfo& a, MyMeshInfo& b) {
	return a.myMesh->bbox.Center().V()[1] < b.myMesh->bbox.Center().V()[1];
}

bool myCompareZ(MyMeshInfo& a, MyMeshInfo& b) {
	return a.myMesh->bbox.Center().V()[2] < b.myMesh->bbox.Center().V()[2];
}

void SpatialTree::splitTreeNode(TileInfo* parentTile)
{
	if(!op.newMethod)
	{
		if (m_correntDepth > m_treeDepth) {
			m_treeDepth = m_correntDepth;
		}
		m_correntDepth++;
		Point3f dim = parentTile->boundingBox->Dim();
		int i;
		int totalVertexCount = parentTile->originalVertexCount;
		if (op.log)std::cout << m_correntDepth << ":" << totalVertexCount << std::endl;

		if (parentTile->myMeshInfos.size() < op.Min_Mesh_Per_Node || m_correntDepth > op.Level) {
			m_correntDepth--;
			return;
		}

		TileInfo* pLeft = new TileInfo;
		TileInfo* pRight = new TileInfo;
		pLeft->boundingBox = new Box3f(*parentTile->boundingBox);
		pRight->boundingBox = new Box3f(*parentTile->boundingBox);
		pLeft->boundingBox->min.X() = pLeft->boundingBox->min.Y() = pLeft->boundingBox->min.Z() = INFINITY;
		pLeft->boundingBox->max.X() = pLeft->boundingBox->max.Y() = pLeft->boundingBox->max.Z() = -INFINITY;
		pRight->boundingBox->min.X() = pRight->boundingBox->min.Y() = pRight->boundingBox->min.Z() = INFINITY;
		pRight->boundingBox->max.X() = pRight->boundingBox->max.Y() = pRight->boundingBox->max.Z() = -INFINITY;
		vector<MyMeshInfo> meshInfos = parentTile->myMeshInfos;
		pLeft->originalVertexCount = 0;
		pRight->originalVertexCount = 0;
		int vertexCount = 0;
		if (dim.X() > dim.Y() && dim.X() > dim.Z()) {
			sort(meshInfos.begin(), meshInfos.end(), myCompareX);
			for (int i = 0; i < meshInfos.size(); ++i) {
				vertexCount += meshInfos[i].myMesh->vn;
				if (vertexCount < totalVertexCount / 2) {
					pLeft->myMeshInfos.push_back(meshInfos[i]);
					pLeft->originalVertexCount += meshInfos[i].myMesh->vn;
					pLeft->boundingBox->Add(meshInfos[i].myMesh->bbox);
				}
				else {
					pRight->myMeshInfos.push_back(meshInfos[i]);
					pRight->originalVertexCount += meshInfos[i].myMesh->vn;
					pRight->boundingBox->Add(meshInfos[i].myMesh->bbox);
				}
			}
		}
		else if (dim.Y() > dim.X() && dim.Y() > dim.Z()) {
			sort(meshInfos.begin(), meshInfos.end(), myCompareY);
			for (int i = 0; i < meshInfos.size(); ++i) {
				vertexCount += meshInfos[i].myMesh->vn;
				if (vertexCount < totalVertexCount / 2) {
					pLeft->myMeshInfos.push_back(meshInfos[i]);
					pLeft->originalVertexCount += meshInfos[i].myMesh->vn;
					pLeft->boundingBox->Add(meshInfos[i].myMesh->bbox);
				}
				else {
					pRight->myMeshInfos.push_back(meshInfos[i]);
					pRight->originalVertexCount += meshInfos[i].myMesh->vn;
					pRight->boundingBox->Add(meshInfos[i].myMesh->bbox);
				}
			}
		}
		else {
			sort(meshInfos.begin(), meshInfos.end(), myCompareZ);
			for (int i = 0; i < meshInfos.size(); ++i) {
				vertexCount += meshInfos[i].myMesh->vn;
				if (vertexCount < totalVertexCount / 2) {
					pLeft->myMeshInfos.push_back(meshInfos[i]);
					pLeft->originalVertexCount += meshInfos[i].myMesh->vn;
					pLeft->boundingBox->Add(meshInfos[i].myMesh->bbox);
				}
				else {
					pRight->myMeshInfos.push_back(meshInfos[i]);
					pRight->originalVertexCount += meshInfos[i].myMesh->vn;
					pRight->boundingBox->Add(meshInfos[i].myMesh->bbox);
				}
			}
		}
		if (pLeft->myMeshInfos.size() == 0) {
			delete pLeft;
			pLeft = nullptr;
		}
		else {
			splitTreeNode(pLeft);
			pLeft->parent = parentTile;
			parentTile->children.push_back(pLeft);

		}
		if (pRight->myMeshInfos.size() == 0) {
			delete pRight;
			pRight = nullptr;
		}
		else {
			splitTreeNode(pRight);
			pRight->parent = parentTile;
			parentTile->children.push_back(pRight);

		}
		m_correntDepth--;
	}
	else {
		Tree = new TreeBuilder(parentTile);
		Tree->setMinMeshPerNode(op.Min_Mesh_Per_Node);
		root = Tree->getRoot();
		buildTree(parentTile, root);
	}
}

void SpatialTree::recomputeTileBox(TileInfo* parent)
{
	for (int i = 0; i < parent->children.size(); ++i) {
		recomputeTileBox(parent->children[i]);
	}
	if (parent->children.size() > 0) {
		parent->myMeshInfos.clear();
	}
}

void SpatialTree::buildTree(TileInfo* parent, shared_ptr<BuildNode> node) {
	if (m_correntDepth > m_treeDepth) {
		m_treeDepth = m_correntDepth;
	}
	m_correntDepth++;
	TileInfo* pLeft = new TileInfo;
	TileInfo* pRight = new TileInfo;

	if (node->children[0] != nullptr) {
		buildTree(pLeft, node->children[0]);
		parent->children.push_back(pLeft);
	}
	if (node->children[1] != nullptr) {
		buildTree(pRight, node->children[1]);
		parent->children.push_back(pRight);
	}
	parent->boundingBox = &node->bounds;
	if (parent->children.size() < 1) {
		for (int i = node->firstPrimOffset; i < node->firstPrimOffset + node->nPrimitives; i++) {
			parent->myMeshInfos.push_back(*Tree->getMeshInfo(i));
		}
	}
	m_correntDepth--;
}

TreeBuilder::TreeBuilder(TileInfo* rootTile)
{
	for (int i = 0; i < rootTile->myMeshInfos.size(); i++) {
		primitives.push_back(&rootTile->myMeshInfos[i]);
	}
	maxPrimInNode = 500;
	method = TreeBuilder::SplitMethod::SAH;
}

TreeBuilder::~TreeBuilder()
{
}
shared_ptr<BuildNode> TreeBuilder::recursiveBuild(std::vector<PrimitiveInfo>& primitiveInfo, int start, int end, std::vector< MyMeshInfo*>& orderedPrims) {
	shared_ptr<BuildNode> node(new BuildNode());
	Box3f bounds;
	bounds.min.X() = bounds.min.Y() = bounds.min.Z() = INFINITY;
	bounds.max.X() = bounds.max.Y() = bounds.max.Z() = -INFINITY;
	for (int i = start; i < end; ++i) {
		bounds.Add(primitiveInfo[i].bounds);
	}

	int nPrimitive = end - start;

	if (nPrimitive == 1) {
		int firstOffset = orderedPrims.size();
		for (int i = start; i < end; i++) {
			int primNum = primitiveInfo[i].primitiveNumber;
			orderedPrims.push_back(primitives[primNum]);
		}
		node->InitLeaf(firstOffset, nPrimitive, bounds);
		return node;
	}
	else {
		Box3f centroidBounds;
		centroidBounds.min.X() = centroidBounds.min.Y() = centroidBounds.min.Z() = INFINITY;
		centroidBounds.max.X() = centroidBounds.max.Y() = centroidBounds.max.Z() = -INFINITY;
		for (int i = start; i < end; i++) {
			centroidBounds.Add(primitiveInfo[i].centroid);
		}
		int dim = centroidBounds.MaxDim();
		int mid = (start + end) / 2;
		if (centroidBounds.max[dim] == centroidBounds.min[dim]) {
			int firstOffset = orderedPrims.size();
			for (int i = start; i < end; i++) {
				int primNum = primitiveInfo[i].primitiveNumber;
				orderedPrims.push_back(primitives[primNum]);
			}
			node->InitLeaf(firstOffset, nPrimitive, bounds);
			return node;
		}
		else {
			switch (method)
			{
			case SplitMethod::Middle: {
				float pmid = (centroidBounds.min[dim], centroidBounds.max[dim]) / 2;
				PrimitiveInfo* midPtr = std::partition(&primitiveInfo[start], &primitiveInfo[end - 1] + 1,
					[dim, pmid](const PrimitiveInfo& pi) {
						return pi.centroid[dim] < pmid;
					});
				mid = midPtr - &primitiveInfo[0];
				if (mid != start && mid != end)
					break;
			}
			case SplitMethod::EqualCounts: {
				mid = (start + end) / 2;
				std::nth_element(&primitiveInfo[start], &primitiveInfo[mid], &primitiveInfo[end - 1] + 1,
					[dim](const PrimitiveInfo& a, const PrimitiveInfo& b) {
						return a.centroid[dim] < b.centroid[dim];
					});
				break;
			}
			case SplitMethod::SAH:
			default: {
				if (nPrimitive <= 4 ) {
					mid = (start + end) / 2;
					std::nth_element(&primitiveInfo[start], &primitiveInfo[mid], &primitiveInfo[end - 1] + 1,
						[dim](const PrimitiveInfo& a, const PrimitiveInfo& b) {
							return a.centroid[dim] < b.centroid[dim];
						});
				}
				else {
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

						int b = nBuckets * Offset(primitiveInfo[i].centroid,centroidBounds)[dim];
						if (b == nBuckets)b = nBuckets - 1;
						buckets[b].count++;
						buckets[b].bounds.Add(primitiveInfo[i].bounds);
					}
					float cost[nBuckets-1];
					for (int i = 0; i < nBuckets - 1; ++i) {
						Box3f b0, b1;
						b0.min.X() = b0.min.Y() = b0.min.Z() = INFINITY;
						b0.max.X() = b0.max.Y() = b0.max.Z() = -INFINITY;
						b1.min.X() = b1.min.Y() = b1.min.Z() = INFINITY;
						b1.max.X() = b1.max.Y() = b1.max.Z() = -INFINITY;
						int count0 = 0, count1 = 0;
						for (int j = 0; j <= i; j++) {
							b0.Add(buckets[j].bounds);
							count0+=buckets[j].count;
						}
						for (int j = i + 1; j < nBuckets; j++) {
							b1.Add(buckets[j].bounds);
							count1+=buckets[j].count;
						}
						cost[i] = 0.125f + (count0 * b0.Volume() + count1 * b1.Volume()) / bounds.Volume();
					}
					float minCost = cost[0];
					int minCostSplit = 0;
					for (int i = 1; i < nBuckets-1; i++) {
						if (cost[i] < minCost) {
							minCost = cost[i];
							minCostSplit = i;
						}
					}
					float leafCost = nPrimitive;
					if (nPrimitive>maxPrimInNode && minCost < leafCost) {
						PrimitiveInfo* pmid = std::partition(&primitiveInfo[start], &primitiveInfo[end - 1] + 1,
							[=](const PrimitiveInfo& pi) {
								int b = nBuckets * Offset(pi.centroid, centroidBounds)[dim];
								if (b == nBuckets)b = nBuckets - 1;
								return b <= minCostSplit;
							});
						mid = pmid - &primitiveInfo[0];
					}
					else
					{
						int firstOffset = orderedPrims.size();
						for (int i = start; i < end; i++) {
							int primNum = primitiveInfo[i].primitiveNumber;
							orderedPrims.push_back(primitives[primNum]);
						}
						node->InitLeaf(firstOffset, nPrimitive, bounds);
						return node;
					}
				}
				break;
			}
			}
			node->InitInterior(dim, recursiveBuild(primitiveInfo, start, mid, orderedPrims), recursiveBuild(primitiveInfo, mid, end, orderedPrims));
		}
	}
	return node;
}
shared_ptr<BuildNode> TreeBuilder::getRoot(){
	std::vector<PrimitiveInfo> primitiveInfo(primitives.size());
	for (int i = 0; i < primitives.size(); i++) {
		primitiveInfo[i] = { i,primitives[i]->myMesh->bbox };
	}
	std::vector<MyMeshInfo*> orderedPrims;
	shared_ptr<BuildNode> root;
	//root = recursiveBuild(primitiveInfo, 0, primitives.size(), orderedPrims);
	root = HLBuild(primitiveInfo,orderedPrims);
	primitives.swap(orderedPrims);
	return root;
}

static inline uint32_t Left(uint32_t x) {
	if (x == (1 << 10))--x;
	x = (x | x << 16) & 0b00000011000000000000000011111111;
	x = (x | x << 8) & 0b00000011000000001111000000001111;
	x = (x | x << 4) & 0b00000011000011000011000011000011;
	x = (x | x << 2) & 0b00001001001001001001001001001001;
	return x;
}

static inline uint32_t EncodeMorton3(const Point3f& p) {
	return (Left(p.X()) << 2) | (Left(p.Y()) < 1) | (Left(p.Z()));
}


shared_ptr<BuildNode> TreeBuilder::HLBuild(std::vector<PrimitiveInfo>& primitiveInfo, std::vector<MyMeshInfo*>& orderedPrims) {
	Box3f bounds;
	bounds.min.X() = bounds.min.Y() = bounds.min.Z() = INFINITY;
	bounds.max.X() = bounds.max.Y() = bounds.max.Z() = -INFINITY;
	for (const PrimitiveInfo& pi : primitiveInfo) {
		bounds.Add(pi.bounds);
	}


	vector<MortonPrimitive> mortonPrims(primitiveInfo.size());

	for (int i = 0; i < primitiveInfo.size(); i++) {
		constexpr int mortonBits = 10;
		constexpr int mortonScale = 1 << mortonBits;
		mortonPrims[i].primitiveIndex = primitiveInfo[i].primitiveNumber;
		Point3f centroidOffset = Offset(primitiveInfo[i].centroid, bounds);
		mortonPrims[i].mortonCode = EncodeMorton3(centroidOffset*mortonScale);
	}

	RadixSort(&mortonPrims);

	vector<LBVHTreeLet> treeLetsToBuild;

	for (int start = 0, end = 1; end <= (int)mortonPrims.size(); ++end) {
		uint32_t mask = 0b00111111111000000000000000000000;
		if (end == (int)mortonPrims.size() || (mortonPrims[start].mortonCode & mask) != (mortonPrims[end].mortonCode & mask) ){
			int nPrimitives = end - start;
			//int maxBuildNode = 2 * nPrimitives - 1;
			//BuildNode* nodes = (BuildNode*)malloc(maxBuildNode*sizeof(BuildNode));
			treeLetsToBuild.push_back({ start,nPrimitives,nullptr});
			start = end;
		}
	}

	orderedPrims.resize(primitiveInfo.size());
	int orderedPrimsOffset = 0;
	for (int i = 0; i < treeLetsToBuild.size(); i++) {
		const int firstBitIndex = 29 - 9;
		LBVHTreeLet& tr = treeLetsToBuild[i];
		tr.buildNodes = emitBVH(primitiveInfo, &mortonPrims[tr.startIndex], tr.nPrimitives, orderedPrims, orderedPrimsOffset, firstBitIndex);
	}

	vector<BuildNode*> finishedTreeLets;
	for (LBVHTreeLet& treeLet : treeLetsToBuild) {
		finishedTreeLets.push_back(treeLet.buildNodes);
	}

	return buildUpperSAH(finishedTreeLets, 0, finishedTreeLets.size());

}