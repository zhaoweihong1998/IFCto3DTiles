#pragma once
#include "MyMesh.h"

struct MergeMeshInfo {
	vector<shared_ptr< MyMesh>> meshes;
	unsigned int material;
};
class MyMeshOptimizer
{
private:
	vector<shared_ptr<MyMesh>>* meshes;
	vector<shared_ptr<MyMesh>> mergeMesh;
	tri::TriEdgeCollapseQuadricParameter* m_pParams;
	float length;
private:
	void mergeMeshes(unsigned int material, vector<shared_ptr< MyMesh>> meshes);
public:
	MyMeshOptimizer(vector<shared_ptr<MyMesh>>* meshes);
	~MyMeshOptimizer() {}
	void Domerge();
	void DoDecemation(float targetPercentage);
	vector<shared_ptr<MyMesh>>* GetMergeMeshInfos();

};

