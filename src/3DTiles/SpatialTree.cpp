#include "SpatialTree.h"

SpatialTree::SpatialTree(const aiScene& mScene, vector<MyMesh*>& meshes, const Option& op):
	op(op)
{
	this->mScene = &mScene;
	this->m_meshes = &meshes;
	this->m_pTileRoot = new TileInfo();
	this->m_correntDepth = 0;
	this->m_treeDepth = 0;
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
	splitTreeNode(m_pTileRoot);
}

TileInfo* SpatialTree::GetTilesetInfo()
{
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
	if (m_correntDepth > m_treeDepth) {
		m_treeDepth = m_correntDepth;
	}
	m_correntDepth++;
	Point3f dim = parentTile->boundingBox->Dim();
	int i;
	int totalVertexCount = parentTile->originalVertexCount;
	if(op.log)std::cout << m_correntDepth << ":" << totalVertexCount << std::endl;
	//cout << m_correntDepth << ' ' << totalVertexCount << endl;
	/*for (int i = 0; i < parentTile->myMeshInfos.size(); ++i) {
		totalVertexCount += parentTile->myMeshInfos[i].myMesh->vn;
	}*/

	//parentTile->originalVertexCount = totalVertexCount;

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
		parentTile->children.push_back(pLeft);

	}
	if (pRight->myMeshInfos.size() == 0) {
		delete pRight;
		pRight = nullptr;
	}
	else {
		splitTreeNode(pRight);
		parentTile->children.push_back(pRight);

	}

	//修改 20210614 赵伟宏
	//20210714 修改 在添加mesh的时候更新box
	//partion000->boundingBox->min.X() = partion000->boundingBox->min.Y() = partion000->boundingBox->min.Z() = INFINITY;
	//partion000->boundingBox->max.X() = partion000->boundingBox->max.Y() = partion000->boundingBox->max.Z() = -INFINITY;
	//partion000->boundingBox->Add(meshInfos[i].myMesh->bbox);
	/*
	float mid_x = (parentTile->boundingBox->min.X() + parentTile->boundingBox->max.X()) / 2;
	float mid_y = (parentTile->boundingBox->min.Y() + parentTile->boundingBox->max.Y()) / 2;
	float mid_z = (parentTile->boundingBox->min.Z() + parentTile->boundingBox->max.Z()) / 2;

	TileInfo* partion000 = new TileInfo;
	TileInfo* partion001 = new TileInfo;
	partion000->boundingBox = new Box3f(*parentTile->boundingBox);
	partion000->boundingBox->min.X() = partion000->boundingBox->min.Y() = partion000->boundingBox->min.Z() = INFINITY;
	partion000->boundingBox->max.X() = partion000->boundingBox->max.Y() = partion000->boundingBox->max.Z() = -INFINITY;

	partion001->boundingBox = new Box3f(*parentTile->boundingBox);
	partion001->boundingBox->min.X() = partion001->boundingBox->min.Y() = partion001->boundingBox->min.Z() = INFINITY;
	partion001->boundingBox->max.X() = partion001->boundingBox->max.Y() = partion001->boundingBox->max.Z() = -INFINITY;


	TileInfo* partion010 = new TileInfo;
	TileInfo* partion011 = new TileInfo;
	partion010->boundingBox = new Box3f(*parentTile->boundingBox);
	partion010->boundingBox->min.X() = partion010->boundingBox->min.Y() = partion010->boundingBox->min.Z() = INFINITY;
	partion010->boundingBox->max.X() = partion010->boundingBox->max.Y() = partion010->boundingBox->max.Z() = -INFINITY;

	partion011->boundingBox = new Box3f(*parentTile->boundingBox);
	partion011->boundingBox->min.X() = partion011->boundingBox->min.Y() = partion011->boundingBox->min.Z() = INFINITY;
	partion011->boundingBox->max.X() = partion011->boundingBox->max.Y() = partion011->boundingBox->max.Z() = -INFINITY;


	TileInfo* partion100 = new TileInfo;
	TileInfo* partion101 = new TileInfo;
	partion100->boundingBox = new Box3f(*parentTile->boundingBox);
	partion100->boundingBox->min.X() = partion100->boundingBox->min.Y() = partion100->boundingBox->min.Z() = INFINITY;
	partion100->boundingBox->max.X() = partion100->boundingBox->max.Y() = partion100->boundingBox->max.Z() = -INFINITY;

	partion101->boundingBox = new Box3f(*parentTile->boundingBox);
	partion101->boundingBox->min.X() = partion101->boundingBox->min.Y() = partion101->boundingBox->min.Z() = INFINITY;
	partion101->boundingBox->max.X() = partion101->boundingBox->max.Y() = partion101->boundingBox->max.Z() = -INFINITY;


	TileInfo* partion110 = new TileInfo;
	TileInfo* partion111 = new TileInfo;
	partion110->boundingBox = new Box3f(*parentTile->boundingBox);
	partion110->boundingBox->min.X() = partion110->boundingBox->min.Y() = partion110->boundingBox->min.Z() = INFINITY;
	partion110->boundingBox->max.X() = partion110->boundingBox->max.Y() = partion110->boundingBox->max.Z() = -INFINITY;

	partion111->boundingBox = new Box3f(*parentTile->boundingBox);
	partion111->boundingBox->min.X() = partion111->boundingBox->min.Y() = partion111->boundingBox->min.Z() = INFINITY;
	partion111->boundingBox->max.X() = partion111->boundingBox->max.Y() = partion111->boundingBox->max.Z() = -INFINITY;



	for (int i = 0; i < meshInfos.size(); ++i) {
		if (meshInfos[i].myMesh->bbox.Center().X()<mid_x) {
			if (meshInfos[i].myMesh->bbox.Center().Y() < mid_y) {
				if (meshInfos[i].myMesh->bbox.Center().Z() < mid_z) {
					partion000->boundingBox->Add(meshInfos[i].myMesh->bbox);
					partion000->myMeshInfos.push_back(meshInfos[i]);

				}
				else {
					partion001->boundingBox->Add(meshInfos[i].myMesh->bbox);
					partion001->myMeshInfos.push_back(meshInfos[i]);

				}
			}
			else {
				if (meshInfos[i].myMesh->bbox.Center().Z() < mid_z) {
					partion010->boundingBox->Add(meshInfos[i].myMesh->bbox);
					partion010->myMeshInfos.push_back(meshInfos[i]);

				}
				else {
					partion011->boundingBox->Add(meshInfos[i].myMesh->bbox);
					partion011->myMeshInfos.push_back(meshInfos[i]);

				}
			}
		}
		else {
			if (meshInfos[i].myMesh->bbox.Center().Y() < mid_y) {
				if (meshInfos[i].myMesh->bbox.Center().Z() < mid_z) {
					partion100->boundingBox->Add(meshInfos[i].myMesh->bbox);
					partion100->myMeshInfos.push_back(meshInfos[i]);

				}
				else {
					partion101->boundingBox->Add(meshInfos[i].myMesh->bbox);
					partion101->myMeshInfos.push_back(meshInfos[i]);

				}
			}
			else {
				if (meshInfos[i].myMesh->bbox.Center().Z() < mid_z) {
					partion110->boundingBox->Add(meshInfos[i].myMesh->bbox);
					partion110->myMeshInfos.push_back(meshInfos[i]);

				}
				else {
					partion111->boundingBox->Add(meshInfos[i].myMesh->bbox);
					partion111->myMeshInfos.push_back(meshInfos[i]);

				}
			}
		}
	}

	if (partion000->myMeshInfos.size() == 0) {
		delete partion000;
		partion000 = nullptr;
	}
	else {
		splitTreeNode(partion000);
		parentTile->children.push_back(partion000);

	}

	if (partion001->myMeshInfos.size() == 0) {
		delete partion001;
		partion001 = nullptr;
	}
	else {
		splitTreeNode(partion001);
		parentTile->children.push_back(partion001);

	}

	if (partion010->myMeshInfos.size() == 0) {
		delete partion010;
		partion010 = nullptr;
	}
	else {
		splitTreeNode(partion010);
		parentTile->children.push_back(partion010);

	}

	if (partion011->myMeshInfos.size() == 0) {
		delete partion011;
		partion011 = nullptr;
	}
	else {
		splitTreeNode(partion011);
		parentTile->children.push_back(partion011);

	}

	if (partion100->myMeshInfos.size() == 0) {
		delete partion100;
		partion100 = nullptr;
	}
	else {
		splitTreeNode(partion100);
		parentTile->children.push_back(partion100);

	}

	if (partion101->myMeshInfos.size() == 0) {
		delete partion101;
		partion101 = nullptr;
	}
	else {
		splitTreeNode(partion101);
		parentTile->children.push_back(partion101);

	}

	if (partion110->myMeshInfos.size() == 0) {
		delete partion110;
		partion110 = nullptr;
	}
	else {
		splitTreeNode(partion110);
		parentTile->children.push_back(partion110);

	}

	if (partion111->myMeshInfos.size() == 0) {
		delete partion111;
		partion111 = nullptr;
	}
	else {
		splitTreeNode(partion111);
		parentTile->children.push_back(partion111);

	}
	*/
	//end




	m_correntDepth--;
}

void SpatialTree::recomputeTileBox(TileInfo* parent)
{
	for (int i = 0; i < parent->children.size(); ++i) {
		recomputeTileBox(parent->children[i]);
	}
	//20210714 赵伟宏 已经更新了
	/*parent->boundingBox->min.X() = parent->boundingBox->min.Y() = parent->boundingBox->min.Z() = INFINITY;
	parent->boundingBox->max.X() = parent->boundingBox->max.Y() = parent->boundingBox->max.Z() = -INFINITY;
	for (int i = 0; i < parent->myMeshInfos.size(); ++i) {
		parent->boundingBox->Add(parent->myMeshInfos[i].myMesh->bbox);
	}*/
	//end 
	if (parent->children.size() > 0) {
		parent->myMeshInfos.clear();
	}
}
