#pragma once
#include <MyMesh.h>
#include <assimp/scene.h>
#include <AssetLib/glTF2/glTF2Asset.h>
#include <AssetLib/glTF2/glTF2AssetWriter.h>
#include <AssetLib/glTF2/glTF2Exporter.h>

using namespace Assimp;
using namespace glTF2;

#ifdef GLTF_EXPORT
#define GLTF_API __declspec(dllexport)
#else GLTF_API __declspec(dllimport)
#endif

class GLTF_API MyGltfExporter
{
private:
	unordered_map<MyVertex*, uint32_t> m_vertexUintMap;
	vector<MyMeshInfo> meshes;
	char* bufferName;
	const aiScene* mScene;
	Asset* mAsset;
	bool setBinary;
	map<std::string, unsigned int> mTexturesByPath;
    glTF2Exporter* exporter;

private:
	void exportMeshes();

	void GetTexSampler(const aiMaterial* mat, Ref<Texture> texture, aiTextureType tt, unsigned int slot);
	void GetMatTexProp(const aiMaterial* mat, unsigned int& prop, const char* propName, aiTextureType tt, unsigned int slot);
	void GetMatTexProp(const aiMaterial* mat, float& prop, const char* propName, aiTextureType tt, unsigned int slot);
	void GetMatTex(const aiMaterial* mat, Ref<Texture>& texture, aiTextureType tt, unsigned int slot );
	void GetMatTex(const aiMaterial* mat, TextureInfo& prop, aiTextureType tt, unsigned int slot );
	void GetMatTex(const aiMaterial* mat, NormalTextureInfo& prop, aiTextureType tt, unsigned int slot );
	void GetMatTex(const aiMaterial* mat, OcclusionTextureInfo& prop, aiTextureType tt, unsigned int slot );
	aiReturn GetMatColor(const aiMaterial* mat, vec4& prop, const char* propName, int type, int idx);
	aiReturn GetMatColor(const aiMaterial* mat, vec3& prop, const char* propName, int type, int idx);
	void exportMaterial();


	//void MergeMeshes();
public:
	MyGltfExporter(vector<MyMeshInfo> meshes, char* buffername, const aiScene* mScene, bool setBinary, IOSystem* io=0);
	void constructAsset();
	void write();
};

