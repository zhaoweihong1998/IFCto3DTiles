#pragma once
#include <vcg/complex/complex.h>
#include <vcg/complex/algorithms/local_optimization/tri_edge_collapse.h>
#include <vcg/complex/algorithms/local_optimization/tri_edge_collapse_quadric.h>
#include <vcg/complex/append.h>
#include <wrap/io_trimesh/import.h>
#include <wrap/io_trimesh/export.h>
#include <vcg/math/quadric.h>
#include <vcg/math/quadric5.h>


using namespace vcg;
using namespace std;


class MyVertex;
class MyEdge;
class MyFace;

struct MyUsedTypes : public UsedTypes<
    Use<MyVertex>::AsVertexType,
    Use<MyEdge>::AsEdgeType,
    Use<MyFace>::AsFaceType> {};


class MyVertex : public Vertex< MyUsedTypes,
    vertex::VFAdj,
    vertex::Coord3f,
    vertex::Color4b,
    vertex::Normal3f,
    vertex::Mark,
    vertex::Qualityf,
    vertex::BitFlags> {
public:
    math::Quadric<double>& Qd() { return q; }
    bool normalExist = false;
private:
    math::Quadric<double> q;

};
typedef tri::BasicVertexPair<MyVertex>  VertexPair;


class MyEdge : public Edge<MyUsedTypes, vertex::Mark,edge::VertexRef,edge::BitFlags, edge::EEAdj> {};


class MyFace : public Face < MyUsedTypes,
    face::VFAdj,
    face::FFAdj,
    face::Mark,
    face::Normal3f,
    face::WedgeRealNormal3f,
    face::VertexRef,
    face::BitFlags > {
public:
    float area() {
        return vcg::DoubleArea(*this);
    }
};

class MyMesh : public tri::TriMesh<
    vector<MyVertex>,
    vector<MyEdge>,
    vector<MyFace> >
{
public:
    bool hasUV;
    unsigned int maxterialIndex;
    unsigned int batchId;
    string name;
    vector<unsigned int> originMesh;
    string id;
    static void ConcatMyMesh(shared_ptr<MyMesh> dest, shared_ptr<MyMesh> src)
    {
        
        VertexIterator vi = Allocator<MyMesh>::AddVertices(*dest, src->vn);
        std::unordered_map<VertexPointer, VertexPointer> vertexMap;
        dest->maxterialIndex = src->maxterialIndex;
        for (int j = 0; j < src->vn; ++j)
        {
            (*vi).P()[0] = src->vert[j].P()[0];
            (*vi).P()[1] = src->vert[j].P()[1];
            (*vi).P()[2] = src->vert[j].P()[2];

            (*vi).normalExist = true;
            (*vi).N()[0] = src->vert[j].N()[0];
            (*vi).N()[1] = src->vert[j].N()[1];
            (*vi).N()[2] = src->vert[j].N()[2];

            vertexMap.insert(make_pair(&(src->vert[j]), &*vi));
            
            ++vi;
        }

        int faceNum = src->fn;
        FaceIterator fi = Allocator<MyMesh>::AddFaces(*dest, faceNum);

        for (int j = 0; j < faceNum; ++j)
        {
            fi->V(0) = vertexMap.at(src->face[j].V(0));
            fi->V(1) = vertexMap.at(src->face[j].V(1));
            fi->V(2) = vertexMap.at(src->face[j].V(2));
            ++fi;
        }
    }
    static void MergeMyMesh(shared_ptr<MyMesh> dest, vector<shared_ptr<MyMesh>>& srcs) {
        for (auto& src : srcs) {
            VertexIterator vi = Allocator<MyMesh>::AddVertices(*dest, src->vn);
            std::unordered_map<VertexPointer, VertexPointer> vertexMap;
            dest->maxterialIndex = src->maxterialIndex;
            for (int j = 0; j < src->vn; ++j)
            {
                (*vi).P()[0] = src->vert[j].P()[0];
                (*vi).P()[1] = src->vert[j].P()[1];
                (*vi).P()[2] = src->vert[j].P()[2];

                (*vi).normalExist = true;
                (*vi).N()[0] = src->vert[j].N()[0];
                (*vi).N()[1] = src->vert[j].N()[1];
                (*vi).N()[2] = src->vert[j].N()[2];

                vertexMap.insert(make_pair(&(src->vert[j]), &*vi));

                ++vi;
            }

            int faceNum = src->fn;
            FaceIterator fi = Allocator<MyMesh>::AddFaces(*dest, faceNum);

            for (int j = 0; j < faceNum; ++j)
            {
                fi->V(0) = vertexMap.at(src->face[j].V(0));
                fi->V(1) = vertexMap.at(src->face[j].V(1));
                fi->V(2) = vertexMap.at(src->face[j].V(2));
                ++fi;
            }
        }
    }
    static float BoxArea(Box3f box) {
        float x = box.DimX();
        float y = box.DimY();
        float z = box.DimZ();
        return 2 * x * y + 2 * x * z + 2 * y * z;
    }
};

class MyTriEdgeCollapse : public tri::TriEdgeCollapseQuadric<MyMesh, VertexPair, MyTriEdgeCollapse, tri::QInfoStandard<MyVertex>> {
public:
    typedef tri::TriEdgeCollapseQuadric<MyMesh, VertexPair, MyTriEdgeCollapse, tri::QInfoStandard<MyVertex>> TECQ;
    typedef MyMesh::EdgeType EdgeType;
    inline MyTriEdgeCollapse(const VertexPair& p, int i, BaseParameterClass* pp) : TECQ(p, i, pp) {}
};

struct nodeInfo
{
    string name;
    string id;
    vector<float> transformation;
    vector<string> meshes;
    vector<struct nodeInfo*> children;
    struct nodeInfo* parent = nullptr;
    Box3f* box = nullptr;
};

class MeshInfo
{
public:
    Matrix44f* matrix;
    unsigned int mNumMeshes;
    unsigned int* meshIndex;
    unsigned int batchId;
    string name;
    struct nodeInfo* myInfo;
public:
    MeshInfo() {
        matrix = nullptr;
        mNumMeshes = 0;
        meshIndex = nullptr;
        name = "";
        myInfo = nullptr;
    }
    MeshInfo(const MeshInfo& meshInfo) {
        if (meshInfo.matrix != nullptr) {
            matrix = new Matrix44f(*(meshInfo.matrix));
        }
        else {
            matrix = nullptr;
        }
        meshIndex = meshInfo.meshIndex;
        mNumMeshes = meshInfo.mNumMeshes;
        batchId = meshInfo.batchId;
        name = meshInfo.name;
        myInfo = meshInfo.myInfo;
    }

    ~MeshInfo() {
        if (matrix != nullptr) {
            delete matrix;
            matrix = nullptr;
        }
    }
};

class MyMeshInfo {
public:
    MyMeshInfo() {};
    ~MyMeshInfo() {};
    MyMeshInfo(const MyMeshInfo& meshInfo) {
        this->material = meshInfo.material;
        this->myMesh = meshInfo.myMesh;
    }
    shared_ptr<MyMesh>  myMesh;
    unsigned int material;
};

class TileInfo {
public:
    int level;
    float geometryError;
    std::vector<shared_ptr<MyMesh>> myMeshInfos;
    std::vector<TileInfo* > children;
    Box3f* boundingBox;
    TileInfo* parent;
    int originalVertexCount;
    std::string contentUri;
    TileInfo() {
        boundingBox = new Box3f();
        parent = nullptr;
        geometryError = 0.0f;
        level = 0;
        originalVertexCount = 0;
    }
};

typedef typename MyMesh::VertexPointer VertexPointer;
typedef typename MyMesh::VertexIterator VertexIterator;
typedef typename MyMesh::FaceIterator FaceIterator;
typedef typename MyMesh::EdgeIterator EdgeIterator;
typedef typename MyMesh::ScalarType ScalarType;
typedef typename MyMesh::VertexType VertexType;
typedef typename MyMesh::FaceType FaceType;

template<typename MeshType>
class myLocalOptimization : public LocalOptimization<MeshType> {
public:
    myLocalOptimization(float len,MeshType& mm, BaseParameterClass* _pp) :LocalOptimization<MeshType>(mm, _pp) {
        length = len;
    }
private:
    float length;
};



