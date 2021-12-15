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
	MyMeshOptimizer(){
		this->meshes = nullptr;
		m_pParams = new tri::TriEdgeCollapseQuadricParameter();

		m_pParams->QualityThr = 1.0f;
		m_pParams->PreserveBoundary = false;
		m_pParams->PreserveTopology = false;
		m_pParams->BoundaryQuadricWeight = 1.0f;
		m_pParams->QualityCheck = false;
		m_pParams->NormalCheck = false;
		m_pParams->OptimalPlacement = true;
		m_pParams->QualityQuadric = true;
		if (m_pParams->PreserveBoundary) {
			m_pParams->FastPreserveBoundary = true;
			m_pParams->PreserveBoundary = false;
		}
		if (m_pParams->NormalCheck)m_pParams->NormalThrRad = M_PI / 4.0;
	}
	~MyMeshOptimizer() {}
	void setMeshes(vector<shared_ptr<MyMesh>>* meshes) {
		this->meshes = meshes;
	}
	void Domerge();
	void DoDecemation(float targetPercentage,float area);
	void DoDecamation(float targetPercentage, vector<shared_ptr<MyMesh>>* meshes);
	void DoDecamation(float targetpercentage, shared_ptr<MyMesh> mesh);
	vector<shared_ptr<MyMesh>>* GetMergeMeshInfos();

};

