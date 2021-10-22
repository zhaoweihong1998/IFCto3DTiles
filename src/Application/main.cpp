#include "My3DTilesExporter.h"
#include <sstream>

#if defined(_MSC_VER)
#include <direct.h>
#include <io.h>
#define GetCurrentDir _getcwd
#elif defined(__unix__)
#include <unistd.h>
#define GetCurrentDir getcwd
#else
#endif

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

std::string get_current_directory()
{
    char buff[250];
    GetCurrentDir(buff, 250);
    string current_working_directory(buff);
    return current_working_directory;
}

void main(int argc, const char* argv[]){
    Option op;
    if (argc == 1) {
        std::cout << "No input file" << std::endl;
        return;
    }
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--filename") || !strcmp(argv[i], "-filename")) {
            if (i + 1 == argc) {
                usage("missing value after --filename argument");
            }
            else {
                string name = argv[i+1];
                int pos = name.find_last_of('.');
                string substr = name.substr(pos + 1);
                if (substr.compare("ifc") != 0) {
                    usage((substr + " is not supported!").c_str());
                }
                op.Filename = argv[++i];
            }
        }
        else if (!strcmp(argv[i], "--maxDepth") || !strcmp(argv[i], "-maxDepth")) {
            if (i + 1 == argc) {
                usage("missing value after --maxDepth argument");
            }
            else {
                std::stringstream s;
                s << argv[++i];
                s >> op.Level;
                op.Level--;
            }
        }
        else if (!strcmp(argv[i], "--3DTiles") || !strcmp(argv[i], "-3DTiles")) {
            if (i + 1 == argc) {
                usage("missing value after --3DTiles argument");
            }
            else {
                if (!strcmp(argv[i + 1], "true")) {
                    op.Binary = true;
                }
                else {
                    op.Binary = false;
                }
            }
        }
        else if (!strcmp(argv[i], "--log") || !strcmp(argv[i], "-log")) {
            if (i + 1 == argc) {
                usage("missing value after --log argument");
            }
            else {
                if (!strcmp(argv[i + 1], "true")) {
                    op.Log = true;
                }
                else {
                    op.Log = false;
                }
            }
        }
        else if (!strcmp(argv[i], "--detach") || !strcmp(argv[i], "-detach")) {
            if (i + 1 == argc) {
                usage("missing value after --detach argument");
            }
            else {
                if (!strcmp(argv[i + 1], "true")) {
                    op.detach = true;
                }
                else {
                    op.detach = false;
                }
            }
        }
        else if (!strcmp(argv[i], "--maxMesh") || !strcmp(argv[i], "-maxMesh")) {
            if (i + 1 == argc) {
                usage("missing value after --maxMesh argument");
            }
            else {
                std::stringstream s;
                s << argv[++i];
                s >> op.MaxMeshPerNode;
            }
        }
        else if (!strcmp(argv[i], "--method") || !strcmp(argv[i], "-method")) {
            if (i + 1 == argc) {
                usage("missing value after --method argument");
            }
            else {
                std::stringstream s;
                s << argv[++i];
                s >> op.Method;
            }
        }
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-help") ||
            !strcmp(argv[i], "-h")) {
            usage();
            return;
        }
    }
    std::string pre = get_current_directory();
    pre += "\\";
    pre += op.OutputDir;
    if (access(pre.c_str(), 0)) {
        std::string command = "mkdir " + pre;
        system(command.c_str());
    }
    My3DTilesExporter* handler = new My3DTilesExporter(op);
    handler->export3DTiles();
}