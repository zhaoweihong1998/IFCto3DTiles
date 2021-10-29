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
private:
	void mergeMeshes(unsigned int material, vector<shared_ptr< MyMesh>> meshes);
public:
	MyMeshOptimizer(vector<shared_ptr<MyMesh>>* meshes);
	~MyMeshOptimizer() {}
	void Domerge();
	void DoDecemation(float targetPercentage,float area);
	vector<shared_ptr<MyMesh>>* GetMergeMeshInfos();

};

