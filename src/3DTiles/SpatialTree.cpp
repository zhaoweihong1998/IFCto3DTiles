#include "SpatialTree.h"
#include <omp.h>

SpatialTree::SpatialTree(const aiScene& mScene, vector<shared_ptr<MyMesh> >& meshes, const Option& op):
	op(op)
{
	this->mScene = &mScene;
	this->m_meshes = &meshes;
	this->m_pTileRoot = new TileInfo();
	this->BigMeshes = new TileInfo();
	this->m_correntDepth = 0;
	this->m_treeDepth = 0;
	root = nullptr;
	Tree = nullptr;
	kdTree = nullptr;
}

void SpatialTree::Initialize()
{
	Box3f* sceneBox = new Box3f();
	sceneBox->min.X() = sceneBox->min.Y() = sceneBox->min.Z() = INFINITY;
	sceneBox->max.X() = sceneBox->max.Y() = sceneBox->max.Z() = -INFINITY;
	m_pTileRoot->boundingBox = new Box3f();
	//add 20211020
	m_pTileRoot->boundingBox->min.X() = m_pTileRoot->boundingBox->min.Y() = m_pTileRoot->boundingBox->min.Z() = INFINITY;
	m_pTileRoot->boundingBox->max.X() = m_pTileRoot->boundingBox->max.Y() = m_pTileRoot->boundingBox->max.Z() = -INFINITY;
	//BigMeshes->boundingBox->min.X() = BigMeshes->boundingBox->min.Y() = BigMeshes->boundingBox->min.Z() = INFINITY;
	//BigMeshes->boundingBox->max.X() = BigMeshes->boundingBox->max.Y() = BigMeshes->boundingBox->max.Z() = -INFINITY;
	BigMeshes->originalVertexCount = 0;
	//end
	m_pTileRoot->originalVertexCount = 0;
	for (int i = 0; i < m_meshes->size(); i++) {
		sceneBox->Add((*m_meshes)[i]->bbox);
		/*MyMeshInfo meshInfo;
		meshInfo.myMesh = (*m_meshes)[i];
		meshInfo.material = (*m_meshes)[i]->maxterialIndex;
		m_pTileRoot->myMeshInfos.push_back(meshInfo);
		m_pTileRoot->originalVertexCount += meshInfo.myMesh->vn;*/
	}
	for (int i = 0; i < m_meshes->size(); i++) 
	{
		int maxDim = (*m_meshes)[i]->bbox.MaxDim();
		if ((*m_meshes)[i]->bbox.Dim()[maxDim] > 0.4 * (sceneBox->Dim()[maxDim])&&op.detach){
			MyMeshInfo meshInfo;
			//BigMeshes->boundingBox->Add((*m_meshes)[i]->bbox);
			meshInfo.myMesh = (*m_meshes)[i];
			meshInfo.material = (*m_meshes)[i]->maxterialIndex;
			BigMeshes->myMeshInfos.push_back(meshInfo);
			BigMeshes->originalVertexCount += meshInfo.myMesh->vn;
		}
		else {
			MyMeshInfo meshInfo;
			m_pTileRoot->boundingBox->Add((*m_meshes)[i]->bbox);
			meshInfo.myMesh = (*m_meshes)[i];
			meshInfo.material = (*m_meshes)[i]->maxterialIndex;
			m_pTileRoot->myMeshInfos.push_back(meshInfo);
			m_pTileRoot->originalVertexCount += meshInfo.myMesh->vn;
		}
	}
	if (BigMeshes->myMeshInfos.size() != 0)
	{
		m_pTileRoot->parent = BigMeshes;
		BigMeshes->children.push_back(m_pTileRoot);
		BigMeshes->boundingBox = sceneBox;
		BigMeshes->parent = nullptr;
	}
	else 
		m_pTileRoot->parent = nullptr;
	//m_pTileRoot->boundingBox = sceneBox;
	//m_pTileRoot->parent = nullptr;
	if(m_pTileRoot->myMeshInfos.size()>op.MaxMeshPerNode)splitTreeNode(m_pTileRoot);
}

TileInfo* SpatialTree::GetTilesetInfo()
{
	if (op.Method==0) {
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
		if (m_pTileRoot->parent != nullptr)return m_pTileRoot->parent;
		else return m_pTileRoot;
	}
	else {
		recomputeTileBox(m_pTileRoot);
		if (m_pTileRoot->parent != nullptr)return m_pTileRoot->parent;
		else return m_pTileRoot;
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
	if(op.Method==0)
	{
		if (m_correntDepth > m_treeDepth) {
			m_treeDepth = m_correntDepth;
		}
		m_correntDepth++;
		Point3f dim = parentTile->boundingBox->Dim();
		int i;
		int totalVertexCount = parentTile->originalVertexCount;
		if (op.Log)std::cout << m_correntDepth << ":" << totalVertexCount << std::endl;

		if (parentTile->myMeshInfos.size() < op.MaxMeshPerNode || m_correntDepth > op.Level) {
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
	else if (op.Method == 5) {
		kdTree = new kdTreeAccel(parentTile);
		kdTree->BuildTree();
		kdTree->setMaxMesh(op.MaxMeshPerNode);
		kdTree->setMaxDepth(op.Level);
		buildTree(parentTile, 0);
	}
	else {
		Tree = new TreeBuilder(parentTile);
		Tree->setMinMeshPerNode(op.MaxMeshPerNode);
		Tree->setThreadNum(op.nThreads);
		Tree->setMethod(op.Method);
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

void SpatialTree::buildTree(TileInfo* parent, int index) {
	if (m_correntDepth > m_treeDepth) {
		m_treeDepth = m_correntDepth;
	}
	m_correntDepth++;
	kdAccelNode* node = kdTree->getNode(index);
	if (node->IsLeaf()) {
		int nPrimitives = node->nPrimitives();
		if (nPrimitives == 1) {
			parent->myMeshInfos.push_back(m_pTileRoot->myMeshInfos[node->onePrimitive]);
		}
		else {
			for (int i = 0; i < nPrimitives; i++) {
				int index_ = kdTree->getIndex(node->primitiveindicesOffset + i);
				parent->myMeshInfos.push_back(m_pTileRoot->myMeshInfos[index_]);
			}
		}
	}
	else {
		TileInfo* pLeft = new TileInfo;
		TileInfo* pRight = new TileInfo;
		pLeft->boundingBox = new Box3f(*(parent->boundingBox));
		pRight->boundingBox = new Box3f(*(parent->boundingBox));
		pLeft->boundingBox->max[node->SplitAxis()] = pRight->boundingBox->min[node->SplitAxis()] = node->SplitPos();
		buildTree(pLeft, index + 1);
		buildTree(pRight, node->AboveChild());
		parent->children.push_back(pLeft);
		parent->children.push_back(pRight);
	}
	m_correntDepth--;
}

TreeBuilder::TreeBuilder(TileInfo* rootTile)
{
	for (int i = 0; i < rootTile->myMeshInfos.size(); i++) {
		primitives.push_back(&rootTile->myMeshInfos[i]);
	}
	nThreads = 4;
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

	if (nPrimitive <= maxPrimInNode) {
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
				float pmid = (centroidBounds.min[dim]+centroidBounds.max[dim]) / 2;
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
	if(method==TreeBuilder::SplitMethod::HLBVH)root = HLBuild(primitiveInfo, orderedPrims);
	else root = recursiveBuild(primitiveInfo, 0, primitives.size(), orderedPrims);
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
	omp_set_num_threads(nThreads);

	bounds.min.X() = bounds.min.Y() = bounds.min.Z() = INFINITY;
	bounds.max.X() = bounds.max.Y() = bounds.max.Z() = -INFINITY;
	for (const PrimitiveInfo& pi : primitiveInfo) {
		bounds.Add(pi.bounds);
	}
	vector<MortonPrimitive> mortonPrims(primitiveInfo.size());
	printf("start to encode bounds\n");
	float startTime = omp_get_wtime();

#pragma omp parallel for shared(mortonPrims)
	for (int i = 0; i < primitiveInfo.size(); i++) {
		int id = omp_get_thread_num();
		printf("i=%d, hello, i am thread %d\n", i,id);
		constexpr int mortonBits = 10;
		constexpr int mortonScale = 1 << mortonBits;
		mortonPrims[i].primitiveIndex = primitiveInfo[i].primitiveNumber;
		Point3f centroidOffset = Offset(primitiveInfo[i].centroid, bounds);
		mortonPrims[i].mortonCode = EncodeMorton3(centroidOffset*mortonScale);
	}
#pragma omp barrier 
	float endTime = omp_get_wtime();
	printf("used time: %f\n", endTime - startTime);

	RadixSort(&mortonPrims);

	vector<LBVHTreeLet> treeLetsToBuild;

	for (int start = 0, end = 1; end <= (int)mortonPrims.size(); ++end) {
		uint32_t mask = 0b00111111000000000000000000000000;
		if (end == (int)mortonPrims.size() || (mortonPrims[start].mortonCode & mask) != (mortonPrims[end].mortonCode & mask) ){
			int nPrimitives = end - start;
			treeLetsToBuild.push_back({ start,nPrimitives,nullptr});
			start = end;
		}
	}

	orderedPrims.resize(primitiveInfo.size());
	int orderedPrimsOffset = 0;
	printf("start to build leaf node\n");
	startTime = omp_get_wtime();
#pragma omp parallel for shared(orderedPrims,orderedPrimsOffset,primitiveInfo,mortonPrims,treeLetsToBuild)
	for (int i = 0; i < treeLetsToBuild.size(); i++) {
		int id = omp_get_thread_num();
		printf("i = %d, hello, I am thread %d\n", i,id);

		const int firstBitIndex = 29 - 6;
		LBVHTreeLet& tr = treeLetsToBuild[i];
		tr.buildNodes = emitBVH(primitiveInfo, &mortonPrims[tr.startIndex], tr.nPrimitives, orderedPrims, orderedPrimsOffset, firstBitIndex);
	}
#pragma omp barrier 
	endTime = omp_get_wtime();
	printf("used time: %f\n", endTime - startTime);


	vector<BuildNode*> finishedTreeLets;
	for (LBVHTreeLet& treeLet : treeLetsToBuild) {
		finishedTreeLets.push_back(treeLet.buildNodes);
	}

	return buildUpperSAH(finishedTreeLets, 0, finishedTreeLets.size());

}

BuildNode* TreeBuilder::emitBVH(const vector<PrimitiveInfo>& primitiveInfo, MortonPrimitive* mortonPrims, int nPrimitives, vector<MyMeshInfo*>& orderedPrims, int& orderedPrimOffset, int bitIndex) {
	if (bitIndex == -1 || nPrimitives < maxPrimInNode) {
		BuildNode* node = new BuildNode();
		Box3f bounds;
		bounds.min.X() = bounds.min.Y() = bounds.min.Z() = INFINITY;
		bounds.max.X() = bounds.max.Y() = bounds.max.Z() = -INFINITY;
		int firstPrimOffset = orderedPrimOffset;
#pragma omp atomic
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
		node->InitInterior(axis, ptr1, ptr2);
		return node;
	}
}

shared_ptr<BuildNode> TreeBuilder::buildUpperSAH(vector<BuildNode*>& treeletRoot, int start, int end) {
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
			BuildNode** pmid = std::partition(&treeletRoot[start], &treeletRoot[end - 1] + 1,
				[=](const BuildNode* pi) {
					int b = nBuckets * Offset(pi->bounds.Center(), centroidBounds)[dim];
					if (b == nBuckets)b = nBuckets - 1;
					return b <= minCostSplit;
				});
			mid = pmid - &treeletRoot[0];
		}
		node->InitInterior(dim, buildUpperSAH(treeletRoot, start, mid), buildUpperSAH(treeletRoot, mid, end));
		return node;
	}
}

void TreeBuilder::RadixSort(vector<MortonPrimitive>* v) {
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

	if (nPasses & 1) {
		std::swap(*v, tempVector);
	}
}

void kdTreeAccel::BuildTree()
{
	std::vector<Box3f> primBounds;
	for (const MyMeshInfo* prim : primitives) {
		Box3f b = prim->myMesh->bbox;
		bounds.Add(b);
		primBounds.push_back(b);
	}

	std::unique_ptr<int[]> primNums(new int[primitives.size()]);
	for (int i = 0; i < primitives.size(); i++) {
		primNums[i] = i;
	}

	std::unique_ptr<BoundEdge[]> edges[3];
	for (int i = 0; i < 3; i++) {
		edges[i].reset(new BoundEdge[2 * primitives.size()]);
	}
	std::unique_ptr<int[]> prims0(new int[primitives.size()]);
	std::unique_ptr<int[]> prims1(new int[(maxDepth + 1) * primitives.size()]);
	buildTree(0, bounds, primBounds, primNums.get(), primitives.size(), maxDepth, edges, prims0.get(), prims1.get(),0);
}

void kdTreeAccel::buildTree(int nodeNum, const Box3f nodeBounds, const std::vector<Box3f>& allPrimimBounds,
	int* primNums, int nPrimitives, int depth,
	const std::unique_ptr<BoundEdge[]> edges[3], int* prims0, int* prims1, int badRefines) {
	if (nextFreeNode == nAllocedNodes) {
		int nNewAllocaNodes = std::max(2 * nAllocedNodes, 512);
		kdAccelNode* n = (kdAccelNode*)malloc(nNewAllocaNodes * sizeof(kdAccelNode));
		if (nAllocedNodes > 0) {
			memcpy(n, nodes, nAllocedNodes * sizeof(kdAccelNode));
		}
		nodes = n;
		nAllocedNodes = nNewAllocaNodes;
	}
	++nextFreeNode;

	if (nPrimitives <= maxPrims || depth == 0) {
		nodes[nodeNum].InitialLeaf(primNums, nPrimitives, &primitivesIndices);
		return;
	}
	int bestAxis = -1, bestOffset = -1;
	float bestCost = INFINITY;
	float oldCost = isectCost * float(nPrimitives);
	float totalSA = nodeBounds.Volume();
	float invTotalSA = 1 / totalSA;
	Point3f d = nodeBounds.max - nodeBounds.min;
	int axis = nodeBounds.MaxDim();
	int retries = 0;
retrySplit:
	for (int i = 0; i < nPrimitives; ++i) {
		int pn = primNums[i];
		const Box3f& bounds = allPrimimBounds[pn];
		edges[axis][2 * i] = BoundEdge(bounds.min[axis], pn, true);
		edges[axis][2 * i + 1] = BoundEdge(bounds.max[axis], pn, false);
	}
	sort(&edges[axis][0], &edges[axis][2 * nPrimitives],
		[](const BoundEdge& e0, const BoundEdge& e1)->bool {
			if (e0.t == e1.t) {
				return (int)e0.type < (int)e1.type;
			}
			else return e0.t < e1.t;
		}
	);
	int nBelow = 0, nAbove = nPrimitives;
	for (int i = 0; i < 2 * nPrimitives; ++i) {
		if (edges[axis][i].type == EdgeType::End)--nAbove;
		float edgeT = edges[axis][i].t;
		if (edgeT > nodeBounds.min[axis] && edgeT < nodeBounds.max[axis]) {
			int otherAxis0 = (axis + 1) % 3, otherAxis1 = (axis + 2) % 3;
			float beblowSA = d[otherAxis0] * d[otherAxis1] * (edgeT - nodeBounds.min[axis]);
			float aboveSA = d[otherAxis0] * d[otherAxis1] * (nodeBounds.max[axis] - edgeT);
			float pBelow = beblowSA * invTotalSA;
			float pAbove = aboveSA * invTotalSA;
			float eb = (nAbove == 0 || nBelow == 0) ? emptyBonus : 0;
			float cost = traversalCost + isectCost * (1 - eb) * (pBelow * nBelow + pAbove * nAbove);
			if (cost < bestCost) {
				bestCost = cost;
				bestAxis = axis;
				bestOffset = i;
			}
		}
		if (edges[axis][i].type == EdgeType::Start)++nBelow;
	}

	if (bestAxis == -1 && retries < 2) {
		++retries;
		axis = (axis + 1) % 3;
		goto retrySplit;
	}
	if (bestCost > oldCost)++badRefines;
	if ((bestCost > 4 * oldCost && nPrimitives < 16) || bestAxis == -1 || badRefines == 3) {
		nodes[nodeNum].InitialLeaf(primNums, nPrimitives, &primitivesIndices);
		return;
	}
	int n0 = 0, n1 = 0;
	for (int i = 0; i < bestOffset; ++i) {
		if (edges[bestAxis][i].type == EdgeType::Start) {
			prims0[n0++] = edges[bestAxis][i].primNum;
		}
	}
	for (int i = bestOffset + 1; i < 2 * nPrimitives; i++) {
		if (edges[bestAxis][i].type == EdgeType::End) {
			prims1[n1++] = edges[bestAxis][i].primNum;
		}
	}

	float tSplit = edges[bestAxis][bestOffset].t;
	Box3f bounds0 = nodeBounds, bounds1 = nodeBounds;
	bounds0.max[bestAxis] = bounds1.min[bestAxis] = tSplit;
	buildTree(nodeNum + 1, bounds0, allPrimimBounds, prims0, n0,
		depth - 1, edges, prims0, prims1 + nPrimitives, badRefines);
	int aboveChild = nextFreeNode;
	nodes[nodeNum].InitiInteror(bestAxis, aboveChild, tSplit);
	buildTree(aboveChild, bounds1, allPrimimBounds, prims1, n1,
		depth - 1, edges, prims0, prims1 + nPrimitives, badRefines);
}


