#include "My3DTilesExporter.h"
#include <MyIfc.h>


static void usage(const char* msg = nullptr) {
    if (msg)
        fprintf(stderr, "IfcTo3DTiles: %s\n\n", msg); 

    fprintf(stderr, R"(usage: IfcTo3DTiles [<options>]
options:
  --help                         Print this help text.
  --filename  <filename.ifc>     Specify input file which is expected to be ifc-formated files
  --nthreads  <num>              Use specified number of threads while choosing HLBVH.
  --log       <true/false>       Specify whether to print infomation about vertices in node while converting
  --maxDepth  <num>              Use specified max depth to build tree
  --maxMesh   <num>              Use sepecified max number of meshes allowed to be contained per node
  --3DTiles   <true/false>       Use specified file format to export model , true for 3DTiles and false for gltf
  --method    <num>              Use specified method blow to split Tree, 3 is default
                                     0    Equalvertices-based BVH
                                     1    Middlepoint-based BVH
                                     2    Equalmeshes-based BVH
                                     3    SAH-based BVH
                                     4    HLBVH 
                                     5    KdTree
)");
    exit(msg ? 1 : 0);
}



enum class Format
{
    GLTF, TILES
};

enum class SplitMethod {
    EQUALVETTICES ,
    MIDDLEPOINTS,
    EQUALELEMENTS,
    SAH,
    HLBVH,
    KDTREE                   
};

class MyIfc
{
public:
    MyIfc() {}
    MyIfc(string file);
    ~MyIfc();
    void convertTo3DTiles() {
        My3DTilesExporter* handler = new My3DTilesExporter(op);
        handler->export3DTiles();
    }
    void setMaxDepth(int depth) { op.Level = depth-1; }
    void setFormat(Format format){
        op.Binary = (bool)format;
    }
    void setMeshNumPerNode(int number) {
        op.MaxMeshPerNode = number;
    }
    void setThreadNum(int number) {
        op.nThreads = number;
    }
    void IsSimplify(bool flag) {
        op.simplify = flag;
    }
    void IsPickBigMesh(bool flag) {
        op.detach = flag;
    }
    void setSplitMethod(SplitMethod method) {
        op.Method = (int)method;
    }
    void setSimplifyTarget(float target) {
        op.simplifyTarget = target;
    }
    void IsLog(bool flag) {
        op.Log = flag;
    }
    void setOutputPath(string path) {
        op.makedir(path);
        op.OutputDir = ".\\"+path+"\\";
    }

    void setFilename(string name) {
        op.Filename = name;
    }
    void setSimplifThreshold(float threshold) {
        op.threshold = threshold;
    }
private:
    Option op;
};

MyIfc::MyIfc(string file)
{
    op.Filename = file;
}

MyIfc::~MyIfc()
{

}

extern "C"
JNIEXPORT jlong JNICALL Java_MyIfc_createNativeIfc
(JNIEnv*, jobject) {
    jlong result;
    result = (jlong) new MyIfc();
    return result;
}

extern "C"
JNIEXPORT void JNICALL Java_MyIfc_readFile
(JNIEnv* env, jobject obj, jlong thiz, jstring filename) {
    const char* name = env->GetStringUTFChars(filename, NULL);
    ((MyIfc*)thiz)->setFilename(name);
}

extern "C"
JNIEXPORT void JNICALL Java_MyIfc_to3DTiles
(JNIEnv* env, jobject obj, jlong thiz) {
    ((MyIfc*)thiz)->convertTo3DTiles();
}


