#include "My3DTilesExporter.h"
#include <sstream>
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
        else if (!strcmp(argv[i], "--maxVerticesPerNode") || !strcmp(argv[i], "-maxVerticesPerNode")) {
            if (i + 1 == argc) {
                std::cout << "missing maxVerticesPerNode" << std::endl;
            }
            else {
                std::stringstream s;
                s << argv[++i];
                s >> op.Max_Vertices_per_Node;
            }
        }
        else if (!strcmp(argv[i], "--minMeshPerNode") || !strcmp(argv[i], "-minMeshPerNode")) {
            if (i + 1 == argc) {
                std::cout << "missing minMeshPerNode" << std::endl;
            }
            else {
                std::stringstream s;
                s << argv[i++];
                s >> op.Min_Mesh_Per_Node;
            }
        }
    }
    My3DTilesExporter* handler = new My3DTilesExporter(op);
    handler->export3DTiles();
    std::cin.get();
}