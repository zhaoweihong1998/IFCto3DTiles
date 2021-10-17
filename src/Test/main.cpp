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
        std::cout << "no imput file" << std::endl;
        return;
    }
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--filename") || !strcmp(argv[i], "-filename")) {
            if (i + 1 == argc) {
                std::cout << "missing filename" << std::endl;
            }
            else {
                op.filename = argv[++i];
            }
        }
        else if (!strcmp(argv[i], "--level") || !strcmp(argv[i], "-level")) {
            if (i + 1 == argc) {
                std::cout << "missing level" << std::endl;
            }
            else {
                std::stringstream s;
                s << argv[++i];
                s >> op.Level;
                op.Level--;
            }
        }
        else if (!strcmp(argv[i], "--binary") || !strcmp(argv[i], "-binary")) {
            if (i + 1 == argc) {
                std::cout << "missing binary" << std::endl;
            }
            else {
                if (!strcmp(argv[i + 1], "true")) {
                    op.binary = true;
                }
                else {
                    op.binary = false;
                }
            }
        }
        else if (!strcmp(argv[i], "--log") || !strcmp(argv[i], "-log")) {
            if (i + 1 == argc) {
                std::cout << "missing log" << std::endl;
            }
            else {
                if (!strcmp(argv[i + 1], "true")) {
                    op.log = true;
                }
                else {
                    op.log = false;
                }
            }
        }
        else if (!strcmp(argv[i], "--maxMeshPerNode") || !strcmp(argv[i], "-maxMeshPerNode")) {
            if (i + 1 == argc) {
                std::cout << "missing maxMeshPerNode" << std::endl;
            }
            else {
                std::stringstream s;
                s << argv[++i];
                s >> op.Max_Mesh_per_Node;
            }
        }
        else if (!strcmp(argv[i], "--minMeshPerNode") || !strcmp(argv[i], "-minMeshPerNode")) {
            if (i + 1 == argc) {
                std::cout << "missing minMeshPerNode" << std::endl;
            }
            else {
                std::stringstream s;
                s << argv[++i];
                s >> op.Min_Mesh_Per_Node;
            }
        }
        else if (!strcmp(argv[i], "--Method") || !strcmp(argv[i], "-Method")) {
            if (i + 1 == argc) {
                std::cout << "missing newMethod" << std::endl;
            }
            else {
                std::stringstream s;
                s << argv[++i];
                s >> op.Method;
            }
        }
    }
    std::string pre = get_current_directory();
    pre += "\\";
    pre += op.outputDir;
    if (access(pre.c_str(), 0)) {
        std::string command = "mkdir " + pre;
        system(command.c_str());
    }
    My3DTilesExporter* handler = new My3DTilesExporter(op);
    handler->export3DTiles();
}