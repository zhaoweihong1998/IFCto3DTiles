#pragma once
#include "MyMesh.h"

struct MergeMeshInfo {
	vector<shared_ptr< MyMesh>> meshes;
	unsigned int material;
};
class MyMeshOptimizer
{
private:
	vector<MyMeshInfo> meshes;
	vector<MyMeshInfo> mergeMesh;
	tri::TriEdgeCollapseQuadricParameter* m_pParams;
private:
	void mergeMeshes(unsigned int material, vector<shared_ptr< MyMesh>> meshes);
public:
	MyMeshOptimizer(vector<MyMeshInfo> meshes);
	~MyMeshOptimizer() {}
	void Domerge();
	float DoDecemation(float targetPercentage, bool remesh = false);
	vector<MyMeshInfo> GetMergeMeshInfos();

};

