#include "My3DTilesExporter.h"
#include "MyMeshOptimizer.h"
bool myCompareDim(MyMeshInfo& a, MyMeshInfo& b) {
	return a.myMesh->bbox.Diag() < b.myMesh->bbox.Diag();
}

My3DTilesExporter::My3DTilesExporter(const Option& op):
	op(op)
{
	importer = new Assimp::Importer();
	io = new Assimp::DefaultIOSystem();
	this->mScene = importer->ReadFile(this->op.filename, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);
	rootTile = nullptr;
	myMeshes.clear();
	m_currentTileLevel = 0;
	m_levelAccumMap.clear();
	batch_id = 0;
}

My3DTilesExporter::~My3DTilesExporter()
{
	/*if (mScene != nullptr) {
		delete mScene;
		mScene = nullptr;
	}

	if (rootTile != nullptr) {
		delete rootTile;
		rootTile = nullptr;
	}*/
}

void My3DTilesExporter::createMyMesh()
{
	for (int i = 0; i < mScene->mNumMeshes; ++i) {

		MyMesh* mesh = new MyMesh();
		myMeshes.push_back(mesh);
		aiMesh* m_mesh = mScene->mMeshes[i];
		mesh->maxterialIndex = m_mesh->mMaterialIndex;
		vector<VertexPointer> index;
		index.reserve(m_mesh->mNumVertices);

		VertexIterator vi = tri::Allocator<MyMesh>::AddVertices(*mesh, m_mesh->mNumVertices);
		for (int j = 0; j < m_mesh->mNumVertices; ++j) {
			(*vi).P()[0] = m_mesh->mVertices[j][0];
			(*vi).P()[1] = m_mesh->mVertices[j][1];
			(*vi).P()[2] = m_mesh->mVertices[j][2];

			if (m_mesh->mNormals != NULL) {
				(*vi).normalExist = true;
				(*vi).N()[0] = m_mesh->mNormals[j][0];
				(*vi).N()[1] = m_mesh->mNormals[j][1];
				(*vi).N()[2] = m_mesh->mNormals[j][2];
			}
			else {
				(*vi).normalExist = false;
			}

			index.push_back(&*vi);
			++vi;
		}

		FaceIterator fi = tri::Allocator<MyMesh>::AddFaces(*mesh, m_mesh->mNumFaces);
		for (int j = 0; j < m_mesh->mNumFaces; ++j) {
			(*fi).V(0) = index[m_mesh->mFaces[j].mIndices[0]];
			(*fi).V(1) = index[m_mesh->mFaces[j].mIndices[1]];
			(*fi).V(2) = index[m_mesh->mFaces[j].mIndices[2]];
			++fi;
		}

		tri::UpdateNormal<MyMesh>::PerVertexClear(*mesh);
		tri::UpdateNormal<MyMesh>::PerVertex(*mesh);
		/*tri::UpdateNormal<MyMesh>::PerVertexAngleWeighted(*mesh);
		tri::UpdateNormal<MyMesh>::PerVertexNelsonMaxWeighted(*mesh);*/
		tri::UpdateNormal<MyMesh>::NormalizePerVertex(*mesh);

	}
}

static void getNodeMeshInfos(aiNode* node, vector<MeshInfo>& meshInfos, unsigned int& batch_id, Matrix44f* parentMatrix = nullptr) {
	Matrix44f matrix;
	matrix.SetIdentity();
	if (!node->mTransformation.IsIdentity()) {
		for (int i = 0; i < 4; ++i) {
			matrix.V()[i * 4 + 0] = node->mTransformation[i][0];
			matrix.V()[i * 4 + 1] = node->mTransformation[i][1];
			matrix.V()[i * 4 + 2] = node->mTransformation[i][2];
			matrix.V()[i * 4 + 3] = node->mTransformation[i][3];
		}
	}

	if (parentMatrix != nullptr) {
		matrix = (*parentMatrix) * matrix;
	}

	if (node->mNumChildren > 0) {
		for (int i = 0; i < node->mNumChildren; ++i) {
			getNodeMeshInfos(node->mChildren[i], meshInfos, batch_id, &matrix);
		}
	}

	if (node->mNumMeshes > 0) {
		MeshInfo meshInfo;
		meshInfo.matrix = new Matrix44f(matrix);
		meshInfo.meshIndex = node->mMeshes;
		meshInfo.mNumMeshes = node->mNumMeshes;
		meshInfo.batchId = batch_id;
		meshInfo.name = (string)node->mName.C_Str();
		meshInfos.push_back(meshInfo);
		batch_id++;
	}
}

void My3DTilesExporter::createNodeBox()
{
	const aiNode* rootNode = mScene->mRootNode;
	Point4f tempPt;

	for (int i = 0; i < rootNode->mNumChildren; ++i) {
		aiNode* node = rootNode->mChildren[i];
		vector<MeshInfo> meshInfos;
		getNodeMeshInfos(node, meshInfos, batch_id);
		for (int j = 0; j < meshInfos.size(); j++) {
			//这里的batchId是node编号
			unsigned int  batchId = meshInfos[j].batchId;
			string name = meshInfos[j].name;
			//end
			if (meshInfos[j].matrix != nullptr) {
				Matrix44f matrix = *meshInfos[j].matrix;
				Matrix44f temp_ =  Inverse(matrix).transpose();
				Matrix33f normalMatrix = Matrix33f(temp_,3);
				//修改20210614 赵伟宏
				MyMesh* mergeNode = new MyMesh();
				//end
				for (int k = 0; k < meshInfos[j].mNumMeshes; ++k) {
					MyMesh* mesh_ = myMeshes[meshInfos[j].meshIndex[k]];
					MyMesh* mesh = new MyMesh();
					MyMesh::ConcatMyMesh(mesh, mesh_);
					vector<MyVertex>::iterator it;
					for (it = mesh->vert.begin(); it != mesh->vert.end(); ++it) {

						//it->batch_id = batchId;

						tempPt[0] = it->P()[0];
						tempPt[1] = it->P()[1];
						tempPt[2] = it->P()[2];
						tempPt[3] = 1;

						tempPt = matrix * tempPt;
						it->P()[0] = tempPt[0];
						it->P()[1] = tempPt[1];
						it->P()[2] = tempPt[2];

						if (it->normalExist) {
							it->N() = normalMatrix * it->N();
						}
					}
					mesh->name = name;
					mesh->batchId = batchId;

					//修改 20210614 赵伟宏
					//MyMesh::ConcatMyMesh(mergeNode, mesh);
					tri::UpdateBounding<MyMesh>::Box(*mesh);
					nodeMeshes.push_back(mesh);
					//end
				}
				//修改 20210614 赵伟宏
				/*tri::UpdateBounding<MyMesh>::Box(*mergeNode);
				nodeMeshes.push_back(mergeNode);*/
				//end

			}
			else {
				//修改 赵伟宏 20210614
				//MyMesh* mergeNode = new MyMesh();
				//end
				for (int k = 0; k < meshInfos[i].mNumMeshes; ++k) {
					MyMesh* mesh_ = myMeshes[meshInfos[i].meshIndex[k]];
					MyMesh* mesh = new MyMesh();
					MyMesh::ConcatMyMesh(mesh, mesh_);
					vector<MyVertex>::iterator it;

					//修改20210614 赵伟宏
					//MyMesh::ConcatMyMesh(mergeNode, mesh);
					tri::UpdateBounding<MyMesh>::Box(*mesh);
					nodeMeshes.push_back(mesh);
					//end
				}
				//修改 20210614 赵伟宏
				/*tri::UpdateBounding<MyMesh>::Box(*mergeNode);
				nodeMeshes.push_back(mergeNode);*/
				//end
			}
		}
	}
}

void My3DTilesExporter::export3DTiles()
{
	if (!mScene) {
		std::cout << "scene is empty!" << std::endl;
		return;
	}
	createMyMesh();
	createNodeBox();
	SpatialTree tree(*mScene, nodeMeshes,op);
	tree.Initialize();
	rootTile = tree.GetTilesetInfo();
	MaxError = rootTile->boundingBox->Diag();
	exportTiles(rootTile);
	export3DTilesset(rootTile);
	
}
nlohmann::json My3DTilesExporter::traverseExportTileSetJson(TileInfo* tileInfo)
{
	nlohmann::json parent = nlohmann::json({});
	Point3f center = tileInfo->boundingBox->Center();
	float radius = tileInfo->boundingBox->Diag() * 0.5;
	nlohmann::json boundingSphere = nlohmann::json::array();
	nlohmann::json boundingBox = nlohmann::json::array();
	boundingSphere.push_back(center.X());
	boundingSphere.push_back(center.Z());
	boundingSphere.push_back(center.Y());
	boundingSphere.push_back(radius);
	boundingBox.push_back(center.X());
	boundingBox.push_back(center.Z());
	boundingBox.push_back(center.Y());
	Point3f dim = tileInfo->boundingBox->Dim();
	float x = dim.X() / 2;
	float y = dim.Y() / 2;
	float z = dim.Z() / 2;
	boundingBox.push_back(x);
	boundingBox.push_back(0);
	boundingBox.push_back(0);
	boundingBox.push_back(0);
	boundingBox.push_back(z);
	boundingBox.push_back(0);
	boundingBox.push_back(0);
	boundingBox.push_back(0);
	boundingBox.push_back(y);
	parent["boundingVolume"] = nlohmann::json({});
	parent["boundingVolume"]["sphere"] = boundingSphere;
	parent["boundingVolume"]["box"] = boundingBox;
	parent["geometricError"] = tileInfo->geometryError;
	parent["refine"] = "ADD";

	nlohmann::json content = nlohmann::json({});
	content["uri"] = tileInfo->contentUri;
	/*content["boundingVolume"] = nlohmann::json({});
	content["boundingVolume"]["sphere"] = boundingSphere;*/
	parent["content"] = content;

	if (tileInfo->children.size() > 0)
	{
		nlohmann::json children = nlohmann::json::array();
		for (int i = 0; i < tileInfo->children.size(); ++i)
		{
			nlohmann::json child = traverseExportTileSetJson(tileInfo->children[i]);
			children.push_back(child);
		}
		parent["children"] = children;
	}

	return parent;
}

void My3DTilesExporter::export3DTilesset(TileInfo* rootTile)
{
	nlohmann::json tilesetJson = nlohmann::json({});
	nlohmann::json version = nlohmann::json({});
	version["version"] = "1.0";
	tilesetJson["asset"] = version;
	tilesetJson["geometricError"] = std::to_string(MaxError / 16);
	nlohmann::json root = nlohmann::json({});
	tilesetJson["root"] = traverseExportTileSetJson(rootTile);

	// add a dummy transform to root to make the tileset position at globe surface.
	nlohmann::json dummyTransform = nlohmann::json::array();
	dummyTransform.push_back(1);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0);
	dummyTransform.push_back(1);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0);
	dummyTransform.push_back(-1);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0);
	dummyTransform.push_back(1);
	tilesetJson["root"]["transform"] = dummyTransform;
	char filepath[1024];
	sprintf(filepath, "%s\\tileset.json", "C:\\Users\\14911\\Documents\\dev\\3DTiles\\3DTiles\\build\\output");
	std::ofstream file(filepath);
	file << tilesetJson;

	/*sprintf(filepath, "%s\\batchLength.json", "C:\\Users\\14911\\Documents\\assimp-5.0.1\\build\\output");
	std::ofstream batchLengthJson(filepath);
	m_batchLegnthsJson["batchLength"] = "C:\\Users\\14911\\Documents\\assimp-5.0.1\\build\\output";
	batchLengthJson << m_batchLegnthsJson;*/
}

void My3DTilesExporter::exportTiles(TileInfo* rootTile)
{
	rootTile->level = m_currentTileLevel++;
	for (int i = 0; i < rootTile->children.size(); ++i) {
		exportTiles(rootTile->children[i]);
		rootTile->myMeshInfos.insert(rootTile->myMeshInfos.end(), rootTile->children[i]->myMeshInfos.begin(), rootTile->children[i]->myMeshInfos.end());
	}
	vector<MyMeshInfo> temp;
	//修改202010614 赵伟宏

	if (rootTile->level != 0) {
		sort(rootTile->myMeshInfos.begin(), rootTile->myMeshInfos.end(), myCompareDim);
		int len = rootTile->myMeshInfos.size();

		//修改202010728 赵伟宏
		int vn_count = 0;
		for (int i = 0; i < rootTile->myMeshInfos.size(); ++i) {
			vn_count += rootTile->myMeshInfos[i].myMesh->vn;
		}
		//end
		if (len > 0) {
			int len_ = len * (float)(1.0 / 3.0);
			unsigned int split_point_vn = (unsigned int)(vn_count * (float)(2.0 / 3.0));
			float split_point = rootTile->myMeshInfos[len_].myMesh->bbox.Diag();
			for (int i = len - 1; i >= 0; i--) {
				unsigned int temp_value = vn_count - rootTile->myMeshInfos.back().myMesh->vn;
				if (temp_value > split_point_vn) {
					vn_count = temp_value;
					temp.push_back(rootTile->myMeshInfos.back());
					rootTile->myMeshInfos.pop_back();
				}
				else {
					//rootTile->originalVertexCount = vn_count;
					break;
				}
				/*if (rootTile->myMeshInfos[i].myMesh->bbox.Diag() > split_point) {
					temp.push_back(rootTile->myMeshInfos.back());
					rootTile->myMeshInfos.pop_back();
				}
				else {
					break;
				}*/
			}

		}
	}
	//end
	char buffername[1024];
	int fileIdx = 0;
	if (m_levelAccumMap.count(rootTile->level) > 0) {
		fileIdx = m_levelAccumMap.at(rootTile->level);
		fileIdx++;
		m_levelAccumMap.at(rootTile->level)++;
	}
	else {
		m_levelAccumMap.insert(make_pair(rootTile->level, fileIdx));
	}
	sprintf(buffername, "%d-%d", rootTile->level, fileIdx);
	rootTile->contentUri = std::string(buffername) + ".b3dm";
	simplifyMesh(rootTile, buffername);
	//修改20210614 赵伟宏
	if (rootTile->level != 0) {
		rootTile->myMeshInfos.clear();
		for (auto v : temp) {
			rootTile->myMeshInfos.push_back(v);
		}
	}
	//end
	m_currentTileLevel--;
}

void My3DTilesExporter::simplifyMesh(TileInfo* tileInfo, char* bufferName)
{
	MyMeshOptimizer op(tileInfo->myMeshInfos);
	//op.Domerge();
	//float maxLength = max(max(tileInfo->boundingBox->DimX(), tileInfo->boundingBox->DimY()), tileInfo->boundingBox->DimZ());
	float maxLength = tileInfo->boundingBox->Diag();
	tileInfo->geometryError = op.DoDecemation(maxLength);
	if (tileInfo->children.size() == 0)tileInfo->geometryError = 0.0;
	vector<MyMeshInfo> meshes = op.GetMergeMeshInfos();
	writeGltf(tileInfo, meshes, bufferName, mScene);
}

void My3DTilesExporter::writeGltf(TileInfo* tileInfo, std::vector<MyMeshInfo> meshes, char* bufferName,const aiScene* mScene)
{
	 MyGltfExporter exporter(meshes, bufferName, mScene, op.binary,this->io);
	 exporter.constructAsset();
	 exporter.write();
}
