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

MyMeshOptimizer::MyMeshOptimizer(vector<shared_ptr<MyMesh>> meshes)
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
	for (int i = 0; i < meshes.size(); ++i)
	{
		if (materialMeshMap.count(meshes[i]->maxterialIndex) > 0)
		{
			materialMeshMap.at(meshes[i]->maxterialIndex).meshes.push_back(meshes[i]);
		}
		else
		{
			std::vector<shared_ptr< MyMesh>> myMeshesToMerge;
			myMeshesToMerge.push_back(meshes[i]);
			MergeMeshInfo mergeMeshInfo;
			mergeMeshInfo.meshes = myMeshesToMerge;
			mergeMeshInfo.material = meshes[i]->maxterialIndex;
			materialMeshMap.insert(make_pair(meshes[i]->maxterialIndex, mergeMeshInfo));
		}
	}

	for (auto it = materialMeshMap.begin(); it != materialMeshMap.end(); ++it) {
		vector<shared_ptr<MyMesh>> myMeshes = it->second.meshes;
		sort(myMeshes.begin(), myMeshes.end(), mesh_compare_fn());
		mergeMeshes(it->second.material,myMeshes);
	}
}

float MyMeshOptimizer::DoDecemation(float tileBoxMaxLength, bool remesh)
{
    int totalFaceCount = 0;
    for (int i = 0; i < meshes.size(); i++)
    {
        totalFaceCount += meshes[i]->fn;
    }
    if (totalFaceCount == 0)
    {
		return tileBoxMaxLength / 16.0f;
    }
	
	return tileBoxMaxLength / 16.0f;
    float decimationRatio = (float)300 / (float)totalFaceCount;
    if (decimationRatio >= 1.0)
    {
		tileBoxMaxLength / 16.0f;
    }
    int totalFaceBeforeSplit = 0;
    for (int i = 0; i < meshes.size(); ++i)
    {
		shared_ptr<MyMesh>  myMesh = meshes[i];

        if (remesh)
        {
            
        }
        vcg::LocalOptimization<MyMesh> deciSession(*myMesh, m_pParams);
        clock_t t1 = std::clock();
        deciSession.Init<MyTriEdgeCollapse>();
        clock_t t2 = std::clock();

        
        int targetFaceCount = myMesh->fn * decimationRatio;
        deciSession.SetTargetSimplices(targetFaceCount); // Target face number;
        deciSession.SetTimeBudget(1.5f); // Time budget for each cycle
        deciSession.SetTargetOperations(10000000000);
        int maxTry = 1000000;
        int currentTry = 0;
        do
        {
            deciSession.DoOptimization();
            currentTry++;
        } while (myMesh->fn > targetFaceCount && currentTry < maxTry);
        clock_t t3 = std::clock();
        tri::UpdateTopology<MyMesh>::FaceFace(*myMesh);
        tri::Clean<MyMesh>::RemoveUnreferencedVertex(*myMesh);
		tri::UpdateBounding<MyMesh>::Box(*myMesh);
        totalFaceBeforeSplit += myMesh->fn;
        totalFaceCount += myMesh->fn;
    }
    printf("tile totalFaceCount before split: %d\n", totalFaceBeforeSplit);
    printf("tile totalFaceCount after Decimation: %d\n", totalFaceCount);
    return tileBoxMaxLength / 16.0f;
}

vector<shared_ptr<MyMesh>> MyMeshOptimizer::GetMergeMeshInfos()
{
	//return mergeMesh;
	return meshes;
}
