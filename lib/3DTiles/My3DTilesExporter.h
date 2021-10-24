#pragma once
#include "TreeBuilder.h"
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/DefaultIOSystem.h>
#include <json.hpp>

#include <MyGltfExporter.h>

#if defined(_MSC_VER)
#ifdef TDTILE_EXPORT
#define MY_API __declspec(dllexport)
#else 
#define MY_API __declspec(dllimport)
#endif // TDTILE_EXPORT
#else
#define MY_API
#endif//_MSC_VER


class MY_API My3DTilesExporter
{
private:
	Assimp::IOSystem* io;
	vector<shared_ptr<MyMesh>> myMeshes;
	vector<shared_ptr<MyMesh>> nodeMeshes;
	TileInfo* rootTile;
	const aiScene* mScene;
	int m_currentTileLevel;
	unordered_map<int,int> m_levelAccumMap;
	nlohmann::json m_batchLegnthsJson;
	float MaxVolume;
	float Distance;
	unsigned int batch_id;
	Assimp::Importer* importer;
	const Option op;
public:
	My3DTilesExporter(const Option& op);
	~My3DTilesExporter();
	void createMyMesh();
	void createNodeBox();
	void export3DTiles();
	void getNodeMeshInfos(aiNode* node, vector<MeshInfo>& meshInfos, unsigned int& batch_id, Matrix44f* parentMatrix = nullptr);
	nlohmann::json traverseExportTileSetJson(TileInfo* tileInfo);
	void export3DTilesset(TileInfo* rootTile);
	void exportTiles(TileInfo* rootTile);
	void simplifyMesh(TileInfo* tileInfo, char* bufferName);
	void writeGltf(TileInfo* tileInfo, std::vector<shared_ptr<MyMesh>> meshes, char* bufferName,const aiScene* mScene);
};

