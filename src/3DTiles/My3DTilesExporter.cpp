#include "My3DTilesExporter.h"
#include "MyMeshOptimizer.h"
bool myCompareDim(shared_ptr<MyMesh>& a, shared_ptr<MyMesh>& b) {
	return a->bbox.Diag() < b->bbox.Diag();
}

My3DTilesExporter::My3DTilesExporter(const Option& op):
	op(op)
{
	importer = new Assimp::Importer();
	io = new Assimp::DefaultIOSystem();
	this->mScene = importer->ReadFile(this->op.Filename, aiProcess_Triangulate);//aiProcess_JoinIdenticalVertices会影响法线质量
	rootTile = nullptr;
	myMeshes.clear();
	m_currentTileLevel = 0;
	m_levelAccumMap.clear();
	batch_id = 0;
	MaxVolume = 0;
	Distance = 0;
	maxArea = -INFINITY;
	minArea = INFINITY;
	nInfo = new nodeInfo();
}

My3DTilesExporter::~My3DTilesExporter()
{
	delete importer;
	delete io;
	delete mScene;

}
void My3DTilesExporter::export3DTiles()
{
	if (!mScene) {
		std::cout << "scene is empty!" << std::endl;
		return;
	}
	createMyMesh();
	createNodeBox();
	if (op.simplify)simplifyMesh(&nodeMeshes);
	info();
	TreeBuilder tree(MergedMeshes, op);
	//TreeBuilder tree(*mScene, nodeMeshes,op);
	tree.Initialize();
	rootTile = tree.GetTilesetInfo();
	MaxVolume = rootTile->boundingBox->Volume();
	exportTiles(rootTile);
	export3DTilesset(rootTile);
}
void My3DTilesExporter::createMyMesh()
{
	
	for (int i = 0; i < mScene->mNumMeshes; ++i) {

		shared_ptr<MyMesh> mesh(new MyMesh());
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

			if (m_mesh->mNormals == NULL) {
				(*vi).normalExist = false;
			}
			else {
				(*vi).normalExist = true;
				(*vi).N()[0] = m_mesh->mNormals[j][0];
				(*vi).N()[1] = m_mesh->mNormals[j][1];
				(*vi).N()[2] = m_mesh->mNormals[j][2];
			}
			index.push_back(&*vi);
			++vi;
		}

		FaceIterator fi = tri::Allocator<MyMesh>::AddFaces(*mesh, m_mesh->mNumFaces);
		for (int j = 0; j < m_mesh->mNumFaces; ++j) {
			(*fi).V(0) = index[m_mesh->mFaces[j].mIndices[0]];
			(*fi).V(1) = index[m_mesh->mFaces[j].mIndices[1]];
			(*fi).V(2) = index[m_mesh->mFaces[j].mIndices[2]];
			float temp = (*fi).area();
			maxArea = maxArea > temp ? maxArea : temp;
			minArea = minArea < temp ? minArea : temp;
			++fi;
		}
		if (!mesh->vert[0].normalExist)
		{
			tri::UpdateNormal<MyMesh>::PerVertex(*mesh);
			tri::UpdateNormal<MyMesh>::NormalizePerVertex(*mesh);
		}

	}
}
void My3DTilesExporter::getNodeMeshInfos(aiNode* node, vector<MeshInfo>& meshInfos, unsigned int& batch_id, struct nodeInfo* ninfo, Matrix44f* parentMatrix) {
	Matrix44f matrix;
	matrix.SetIdentity();
	ninfo->name = node->mName.C_Str();
	if (!node->mTransformation.IsIdentity()) {
		for (int i = 0; i < 4; ++i) {
			matrix.V()[i * 4 + 0] = node->mTransformation[i][0];
			ninfo->transformation.push_back(node->mTransformation[i][0]);
			matrix.V()[i * 4 + 1] = node->mTransformation[i][1];
			ninfo->transformation.push_back(node->mTransformation[i][1]);
			matrix.V()[i * 4 + 2] = node->mTransformation[i][2];
			ninfo->transformation.push_back(node->mTransformation[i][2]);
			matrix.V()[i * 4 + 3] = node->mTransformation[i][3];
			ninfo->transformation.push_back(node->mTransformation[i][3]);
		}
	}
	if (parentMatrix != nullptr) {
		matrix = (*parentMatrix) * matrix;
	}
	if (node->mNumChildren > 0) {
		ninfo->children.resize(node->mNumChildren);
		for (int i = 0; i < node->mNumChildren; ++i) {
			ninfo->children[i] = new nodeInfo();
			ninfo->children[i]->parent = ninfo;
			string id = ninfo->id + "_";
			id = id + to_string(i);
			ninfo->children[i]->id = id;
			getNodeMeshInfos(node->mChildren[i], meshInfos, batch_id, ninfo->children[i],&matrix);
		}
	}

	if (node->mNumMeshes > 0) {
		MeshInfo meshInfo;
		meshInfo.matrix = new Matrix44f(matrix);
		meshInfo.meshIndex = node->mMeshes;
		meshInfo.mNumMeshes = node->mNumMeshes;
		meshInfo.batchId = batch_id;
		meshInfo.name = (string)node->mName.C_Str();
		meshInfo.myInfo = ninfo;
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
		getNodeMeshInfos(node, meshInfos, batch_id, nInfo);
		for (int j = 0; j < meshInfos.size(); j++) {

			unsigned int  batchId = meshInfos[j].batchId;
			struct nodeInfo* ninfo = meshInfos[j].myInfo;
			string name = meshInfos[j].name;
			string id = string("N")+ninfo->id;
			id = id + "_M_";
			vector<shared_ptr<MyMesh>> tempMeshes;
			shared_ptr<MyMesh> mergedMesh(new MyMesh());
			if (meshInfos[j].matrix != nullptr) {
				Matrix44f matrix = *meshInfos[j].matrix;
				Matrix44f temp_ = Inverse(matrix).transpose();
				Matrix33f normalMatrix = Matrix33f(temp_, 3);
				for (int k = 0; k < meshInfos[j].mNumMeshes; ++k) {
					shared_ptr<MyMesh> mesh_ = myMeshes[meshInfos[j].meshIndex[k]];
					shared_ptr<MyMesh> mesh(new MyMesh());
					MyMesh::ConcatMyMesh(mesh, mesh_);
					vector<MyVertex>::iterator it;
					for (it = mesh->vert.begin(); it != mesh->vert.end(); ++it) {
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
					mesh->id = id + to_string(k);
					ninfo->meshes.push_back(id + to_string(k));
					tri::UpdateBounding<MyMesh>::Box(*mesh);
					mergedMesh->originMesh.push_back(nodeMeshes.size());
					nodeMeshes.push_back(mesh);
					tempMeshes.push_back(mesh);
				}
			}
			else {
				for (int k = 0; k < meshInfos[i].mNumMeshes; ++k) {
					shared_ptr<MyMesh> mesh_ = myMeshes[meshInfos[i].meshIndex[k]];
					shared_ptr<MyMesh> mesh(new MyMesh());
					MyMesh::ConcatMyMesh(mesh, mesh_);
					mesh->name = name + to_string(k);
					mesh->batchId = batchId;
					ninfo->meshes.push_back(mesh->name);
					tri::UpdateBounding<MyMesh>::Box(*mesh);
					mergedMesh->originMesh.push_back(nodeMeshes.size());
					nodeMeshes.push_back(mesh);
					tempMeshes.push_back(mesh);
				}
			}
			MyMesh::MergeMyMesh(mergedMesh, tempMeshes);
			tri::UpdateBounding<MyMesh>::Box(*mergedMesh);
			MergedMeshes.push_back(mergedMesh);
			ninfo->box = &mergedMesh->bbox;
		}
	}
	myMeshes.clear();
}
nlohmann::json My3DTilesExporter::traverseExportTileSetJson(TileInfo* tileInfo)
{
	nlohmann::json parent = nlohmann::json({});
	Point3f center = tileInfo->boundingBox->Center();
	float radius = tileInfo->boundingBox->Diag() * 0.5;
	nlohmann::json boundingSphere = nlohmann::json::array();
	nlohmann::json boundingBox = nlohmann::json::array();
	boundingSphere.push_back(center.X());
	boundingSphere.push_back(center.Y());
	boundingSphere.push_back(center.Z());
	boundingSphere.push_back(radius);
	boundingBox.push_back(center.X());
	boundingBox.push_back(center.Y());
	boundingBox.push_back(center.Z());
	Point3f dim = tileInfo->boundingBox->Dim();
	float x = dim.X() / 2;
	float y = dim.Y() / 2;
	float z = dim.Z() / 2;
	boundingBox.push_back(x);
	boundingBox.push_back(0);
	boundingBox.push_back(0);
	boundingBox.push_back(0);
	boundingBox.push_back(y);
	boundingBox.push_back(0);
	boundingBox.push_back(0);
	boundingBox.push_back(0);
	boundingBox.push_back(z);
	parent["boundingVolume"] = nlohmann::json({});
	parent["boundingVolume"]["sphere"] = boundingSphere;
	parent["boundingVolume"]["box"] = boundingBox;
	parent["geometricError"] = tileInfo->geometryError;
	parent["refine"] = "ADD";

	nlohmann::json content = nlohmann::json({});
	content["uri"] = tileInfo->contentUri;
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
	tilesetJson["geometricError"] = to_string(MaxVolume);
	nlohmann::json root = nlohmann::json({});
	tilesetJson["root"] = traverseExportTileSetJson(rootTile);
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
	dummyTransform.push_back(1);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0);
	dummyTransform.push_back(0);
	dummyTransform.push_back(-1);
	tilesetJson["root"]["transform"] = dummyTransform;
	char* filepath = ".\\output\\tileset.json";
	std::ofstream file(filepath);
	file << tilesetJson;
}
void My3DTilesExporter::exportTiles(TileInfo* rootTile)
{
	rootTile->level = ++m_currentTileLevel;
	char buffername[1024];
	int fileIdx = 1;
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

	for (int i = 0; i < rootTile->children.size(); ++i) {
		exportTiles(rootTile->children[i]);
		rootTile->myMeshInfos.insert(rootTile->myMeshInfos.end(), rootTile->children[i]->myMeshInfos.begin(), rootTile->children[i]->myMeshInfos.end());
	}

	int vn_count = 0;
	for (int i = 0; i < rootTile->myMeshInfos.size(); ++i) {
		vn_count += rootTile->myMeshInfos[i]->vn;
	}
	rootTile->originalVertexCount = vn_count;

	if (op.Log) {
		std::cout << "[before siftting]\t" <<"Level-Node:" <<buffername<< "\tsize of meshes:" << rootTile->myMeshInfos.size() <<"\tnumber of vertices:" << rootTile->originalVertexCount<< std::endl;
	}
	vector<shared_ptr<MyMesh>> temp;
	if (rootTile->level != 1) {
		sort(rootTile->myMeshInfos.begin(), rootTile->myMeshInfos.end(), myCompareDim);
		int len = rootTile->myMeshInfos.size();
		if (len > 0) {
			unsigned int split_point_vn = (unsigned int)(vn_count * (float)(2.0 / 3.0));
			for (int i = len - 1; i >= 0; i--) {
				unsigned int temp_value = vn_count - rootTile->myMeshInfos.back()->vn;
				if (temp_value > split_point_vn) {
					vn_count = temp_value;
					temp.push_back(rootTile->myMeshInfos.back());
					rootTile->myMeshInfos.pop_back();
				}
				else {
					break;
				}
				
			}
		}
	}
	if (op.Log) {
		vn_count = 0;
		for (int i = 0; i < rootTile->myMeshInfos.size(); ++i) {
			vn_count += rootTile->myMeshInfos[i]->vn;
		}
		rootTile->originalVertexCount = vn_count;
		std::cout << "[after siftting]\t" <<"Level-Node:" << buffername << "\tsize of meshes:" << rootTile->myMeshInfos.size() << "\tnumber of vertices:" << rootTile->originalVertexCount << std::endl;
	}
	rootTile->geometryError = rootTile->boundingBox->Volume();
	writeGltf(rootTile, &rootTile->myMeshInfos, buffername, mScene);
	if (rootTile->level != 1) {
		rootTile->myMeshInfos.clear();
		for (auto v : temp) {
			rootTile->myMeshInfos.push_back(v);
		}
	}
	m_currentTileLevel--;
}
void My3DTilesExporter::simplifyMesh(vector<shared_ptr<MyMesh>>* meshes)
{
	MyMeshOptimizer optimiazer(meshes);
	optimiazer.DoDecemation(op.simplifyTarget, minArea + (maxArea - minArea) * pow(0.8, m_currentTileLevel));
}
void My3DTilesExporter::writeGltf(TileInfo* tileInfo, std::vector<shared_ptr<MyMesh>>* meshes, char* bufferName,const aiScene* mScene)
{
	vector<shared_ptr<MyMesh >> tempMeshes;
	unordered_map<unsigned int,vector<float>> boxs;
	for (auto mesh : *meshes) {
		boxs[nodeMeshes[mesh->originMesh[0]]->batchId] = { mesh->bbox.min[0],mesh->bbox.min[1],mesh->bbox.min[2],mesh->bbox.max[0],mesh->bbox.max[1],mesh->bbox.max[2] };
		for (unsigned int index : mesh->originMesh) {
			tempMeshes.push_back(nodeMeshes[index]);
		}
	}
	bufferName = (char*)(op.OutputDir + "\\" + string(bufferName)).c_str();
	 MyGltfExporter exporter(&tempMeshes, bufferName, mScene, op.Binary,boxs,this->io);
	 exporter.constructAsset();
	 exporter.write();
}
void My3DTilesExporter::info() {
	/*vector<double> counter;
	for (auto& mesh : nodeMeshes) {
		for (auto& face : mesh->face) {
			if (face.IsD())continue;
			double temp = face.area() / 2;
			counter.push_back(temp);
		}
	}*/
	nlohmann::json scene = nlohmann::json({});
	string name = op.Filename.substr(op.Filename.find_last_of('\\') + 1);
	name = name.substr(0, name.find_last_of('.'));
	scene["name"] = name;
	scene["scene"] = GetInfo(nInfo);
	/*faceArea["data"] = counter;
	faceArea["maxArea"] = maxArea / 2.0;
	faceArea["minArea"] = minArea / 2.0;*/
	name = name + ".json";
	name = op.OutputDir+"\\"+ name;
	char* filepath = (char*)name.c_str();
	std::ofstream file(filepath);
	file << scene;
}
nlohmann::json My3DTilesExporter::GetInfo(struct nodeInfo* ninfo) {
	nlohmann::json info = nlohmann::json({});
	info["name"] = ninfo->name;
	
	if (ninfo->transformation.size() > 0) {
		nlohmann::json Transform = nlohmann::json::array();
		for (auto data : ninfo->transformation) {
			Transform.push_back(data);
		}
		info["transformation"] = Transform;
	}
	
	/*if (ninfo->box != nullptr) {
		nlohmann::json min = nlohmann::json::array();
		nlohmann::json max = nlohmann::json::array();
		min.push_back(ninfo->box->min[0]);
		min.push_back(ninfo->box->min[1]);
		min.push_back(ninfo->box->min[2]);
		max.push_back(ninfo->box->max[0]);
		max.push_back(ninfo->box->max[1]);
		max.push_back(ninfo->box->max[2]);
		info["boundingBox"]["min"] = min;
		info["boundingBox"]["max"] = max;
	}*/
	if (ninfo->children.size() > 0) {
		nlohmann::json children = nlohmann::json::array();
		for (auto child : ninfo->children) {
			children.push_back(GetInfo(child));
		}
		info["children"] = children;
	}
	if (ninfo->meshes.size() > 0) {
		nlohmann::json meshes = nlohmann::json::array();
		for (auto mesh : ninfo->meshes) {
			meshes.push_back(mesh);
		}
		info["meshes"] = meshes;
	}
	return info;
}