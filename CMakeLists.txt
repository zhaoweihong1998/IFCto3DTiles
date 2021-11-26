cmake_minimum_required(VERSION 3.20)
project(3DTiles VERSION 2.0.0)

include_directories(${PROJECT_SOURCE_DIR}/include/vcglib ${PROJECT_SOURCE_DIR}/include/vcglib/eigenlib  ${PROJECT_SOURCE_DIR}/include)

# 指定输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
# 指定源码目录
aux_source_directory(${PROJECT_SOURCE_DIR}/src/3DTiles 3dtiles_src_dirs) 
aux_source_directory(${PROJECT_SOURCE_DIR}/src/Application app_src_dirs) 
aux_source_directory(${PROJECT_SOURCE_DIR}/src/gltfHandler gltfHandler_src_dirs) 

add_subdirectory(deps/assimp) # 指定 编译子目录
set_target_properties(assimp PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
      ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
  )

add_library(gltfHandler SHARED ${gltfHandler_src_dirs})
target_include_directories(gltfHandler PUBLIC ${PROJECT_SOURCE_DIR}/lib/gltfHandler ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/deps/assimp/include ${PROJECT_SOURCE_DIR}/deps/assimp/code ${PROJECT_SOURCE_DIR}/deps/assimp/contrib/rapidjson/include ${PROJECT_SOURCE_DIR}/deps/assimp/contrib )
target_link_libraries(gltfHandler assimp)
target_compile_definitions(gltfHandler PUBLIC RAPIDJSON_HAS_STDSTRING=1 GLTF_EXPORT)
# set(BUILD_FLAGS "/DGLTF_EXPORT /DRAPIDJSON_HAS_STDSTRING=1" )
# set_target_properties(gltfHandler PROPERTIES COMPILE_FLAGS ${BUILD_FLAGS})

add_library(3DTiles SHARED ${3dtiles_src_dirs}) # 指定可执行文件名及源码
target_include_directories(3DTiles PUBLIC ${PROJECT_SOURCE_DIR}/lib/3DTiles ${PROJECT_SOURCE_DIR}/lib/gltfHandler ${PROJECT_SOURCE_DIR}/include/nlohmann  ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/deps/assimp/include ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(3DTiles assimp gltfHandler) # 链接外部库
target_compile_definitions(3DTiles PUBLIC TDTILE_EXPORT)
# set(BUILD_FLAGS "/DTDTILE_EXPORT" )
# set_target_properties(3DTiles PROPERTIES COMPILE_FLAGS ${BUILD_FLAGS})


add_library(MyIfc ${app_src_dirs})
target_include_directories(MyIfc PUBLIC ${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(MyIfc 3DTiles)
#target_compile_definitions(test PUBLIC openmp)
# set(BUILD_FLAGS "/DTDTILE_EXPORT" )
# set_target_properties(test PROPERTIES COMPILE_FLAGS ${BUILD_FLAGS})
# if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC") 
# add_compile_options(/Dopenmp)
# elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")  
# add_compile_options(-fopenmp)
# endif()