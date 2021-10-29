#include "MyMeshOptimizer.h"

struct mesh_compare_fn
{
	inline bool operator() (const shared_ptr<MyMesh> myMesh1, const shared_ptr<MyMesh> myMesh2)
	{
		return myMesh1->vn < myMesh2->vn;
	}
};

void MyMeshOptimizer::mergeMeshes(unsigned int material, vector<shared_ptr< MyMesh>> meshes)
{
	int totalVertex = 0;
	int totalFace = 0;
	std::vector<shared_ptr< MyMesh>> meshesToMerge;
	shared_ptr< MyMesh> mergedMesh(new MyMesh());
	for (int i = 0; i < meshes.size(); ++i)
	{
		shared_ptr<MyMesh> myMesh = meshes[i];
		 //FIXME: Maybe not neccessary to limit the vertex number since we will do decimation later anyway.
		if (totalVertex + myMesh->vn > 65536 || (totalVertex + myMesh->vn < 65536 && i == meshes.size() - 1))
		{
			if (totalVertex + myMesh->vn < 65536 && i == meshes.size() - 1)
			{
				MyMesh::ConcatMyMesh(mergedMesh, myMesh);
				totalVertex += myMesh->vn;
				totalFace += myMesh->fn;
			}
			shared_ptr<MyMesh> meshInfo;
			meshInfo->maxterialIndex = material;
			meshInfo = mergedMesh;
			mergedMesh = make_shared<MyMesh>();
			mergeMesh.push_back(meshInfo);
			totalVertex = 0;
			totalFace = 0;
		}

		MyMesh::ConcatMyMesh(mergedMesh, myMesh);

		totalVertex += myMesh->vn;
		totalFace += myMesh->fn;
	}
}

MyMeshOptimizer::MyMeshOptimizer(vector<shared_ptr<MyMesh>>* meshes)
{
	this->meshes = meshes;
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

void MyMeshOptimizer::Domerge()
{
	unordered_map<int, MergeMeshInfo> materialMeshMap;
	for (int i = 0; i < meshes->size(); ++i)
	{
		if (materialMeshMap.count((*meshes)[i]->maxterialIndex) > 0)
		{
			materialMeshMap.at((*meshes)[i]->maxterialIndex).meshes.push_back((*meshes)[i]);
		}
		else
		{
			std::vector<shared_ptr< MyMesh>> myMeshesToMerge;
			myMeshesToMerge.push_back((*meshes)[i]);
			MergeMeshInfo mergeMeshInfo;
			mergeMeshInfo.meshes = myMeshesToMerge;
			mergeMeshInfo.material = (*meshes)[i]->maxterialIndex;
			materialMeshMap.insert(make_pair((*meshes)[i]->maxterialIndex, mergeMeshInfo));
		}
	}

	for (auto it = materialMeshMap.begin(); it != materialMeshMap.end(); ++it) {
		vector<shared_ptr<MyMesh>> myMeshes = it->second.meshes;
		sort(myMeshes.begin(), myMeshes.end(), mesh_compare_fn());
		mergeMeshes(it->second.material,myMeshes);
	}
}

void MyMeshOptimizer::DoDecemation(float targetPercentage,float area)
{
    int totalFaceCount = 0;
    for (int i = 0; i < meshes->size(); i++)
    {
        totalFaceCount += (*meshes)[i]->fn;
		for (auto& face : (*meshes)[i]->face) {
			if (face.IsD())continue;
			if (face.area() < area) {
				face.SetS();
			}
		}
    }

	
    if (totalFaceCount == 0)
    {
		return;
    }
    int totalFaceBeforeSplit = totalFaceCount;
	totalFaceCount = 0;
    for (int i = 0; i < meshes->size(); ++i)
    {
		shared_ptr<MyMesh>  myMesh = (*meshes)[i];


		// select only the vertices having ALL incident faces selected
		tri::UpdateSelection<MyMesh>::VertexFromFaceStrict(*myMesh);

		// Mark not writable un-selected vertices
		for (auto& vi : (*meshes)[i]->vert)
		{
			if (!vi.IsD())
			{
				if (!vi.IsS()) vi.ClearW();
				else vi.SetW();
			}
		}
			
        vcg::LocalOptimization<MyMesh> deciSession(*myMesh, m_pParams);
        deciSession.Init<MyTriEdgeCollapse>();
        int targetFaceCount = myMesh->fn * targetPercentage;
		int faceToDel = myMesh->fn - targetFaceCount;
        deciSession.SetTargetSimplices(targetFaceCount); 
        deciSession.SetTimeBudget(0.1f); 
		while (deciSession.DoOptimization() && myMesh->fn > targetFaceCount) {
			printf("Simplifying... %d%%\n", (int)(100 - 100 * (myMesh->fn - targetFaceCount) / (faceToDel)));
		};
		deciSession.Finalize<MyTriEdgeCollapse>();
		for (auto vi = myMesh->vert.begin(); vi != myMesh->vert.end(); ++vi)
		{
			if (!(*vi).IsD()) (*vi).ClearW();
			if ((*vi).IsS()) (*vi).ClearS();
		}

        tri::UpdateTopology<MyMesh>::FaceFace(*myMesh);
		//tri::Clean<MyMesh>::RemoveDuplicateVertex(*myMesh);
        tri::Clean<MyMesh>::RemoveUnreferencedVertex(*myMesh);
		tri::UpdateBounding<MyMesh>::Box(*myMesh);
		tri::UpdateNormal<MyMesh>::PerVertex(*myMesh);
		tri::UpdateNormal<MyMesh>::NormalizePerVertex(*myMesh);
        totalFaceCount += myMesh->fn;
    }
	cout << "tile totalFaceCount before Decimation:" << totalFaceBeforeSplit << endl;
	cout << "tile totalFaceCount after Decimation:" << totalFaceCount << endl;
}

vector<shared_ptr<MyMesh>>* MyMeshOptimizer::GetMergeMeshInfos()
{
	return meshes;
}
