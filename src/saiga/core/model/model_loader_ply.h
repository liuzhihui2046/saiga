/**
 * Copyright (c) 2021 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once
#include "saiga/core/geometry/triangle_mesh.h"
#include "saiga/core/util/color.h"
#include "saiga/core/util/tostring.h"

#include <fstream>
#include <iostream>
#include <vector>

namespace Saiga
{
namespace PLYLoaderDetail
{
template <typename VertexType>
static inline int print(std::vector<std::string>& header);
template <typename VertexType>
static inline void write(char* ptr, VertexType v);
};  // namespace PLYLoaderDetail

class SAIGA_CORE_API PLYLoader
{
   public:
    struct VertexProperty
    {
        std::string name;
        std::string type;
    };



    std::vector<std::pair<int, int>> offsetType;


    int vertexSize;
    int dataStart;
    std::vector<char> data;
    std::vector<VertexProperty> vertexProperties;

    std::string faceVertexCountType;
    std::string faceVertexIndexType;

    TriangleMesh<VertexNC, uint32_t> mesh;

    int vertexCount = -1, faceCount = -1;

    PLYLoader(const std::string& file);


    int sizeoftype(std::string t);

    void parseHeader();

    void parseMeshBinary();


    template <typename VertexType, typename IndexType>
    static void save(std::string file, TriangleMesh<VertexType, IndexType>& mesh)
    {
        std::cout << "Save ply " << file << std::endl;
        std::vector<char> data;

        std::vector<std::string> header;

        header.push_back("ply");

        header.push_back("format binary_little_endian 1.0");
        header.push_back("comment generated by lib saiga");


        header.push_back("element vertex " + to_string(mesh.vertices.size()));

        int vertexSize = PLYLoaderDetail::print<VertexType>(header);

        header.push_back("element face " + to_string(mesh.faces.size()));
        header.push_back("property list uchar int vertex_indices");

        header.push_back("end_header");


        for (auto str : header)
        {
            data.insert(data.end(), str.begin(), str.end());
            data.push_back('\n');
        }

        int dataStart = data.size();

        data.resize(data.size() + vertexSize * mesh.vertices.size());
        char* ptr = data.data() + dataStart;
        for (auto v : mesh.vertices)
        {
            PLYLoaderDetail::write(ptr, v);
            ptr += vertexSize;
        }



        int faceStart = data.size();
        int faceSize  = 3 * sizeof(int) + 1;
        data.resize(data.size() + faceSize * mesh.faces.size());
        ptr = data.data() + faceStart;
        for (auto f : mesh.faces)
        {
            ptr[0]    = 3;
            int* fptr = (int*)(ptr + 1);
            fptr[0]   = f(0);
            fptr[1]   = f(1);
            fptr[2]   = f(2);
            ptr += faceSize;
        }

        std::ofstream stream(file, std::ios::binary);
        if (!stream.is_open())
        {
            std::cerr << "Could not open file " << file << std::endl;
        }

        stream.write(data.data(), data.size());
    }
};

namespace PLYLoaderDetail
{
template <>
inline int print<Vertex>(std::vector<std::string>& header)
{
    header.push_back("property float x");
    header.push_back("property float y");
    header.push_back("property float z");
    return 3 * sizeof(float);
}

template <>
inline int print<VertexN>(std::vector<std::string>& header)
{
    int ret = print<Vertex>(header);
    header.push_back("property float nx");
    header.push_back("property float ny");
    header.push_back("property float nz");
    return ret + 3 * sizeof(float);
}

template <>
inline int print<VertexC>(std::vector<std::string>& header)
{
    int ret = print<Vertex>(header);
    header.push_back("property float red");
    header.push_back("property float green");
    header.push_back("property float blue");
    return ret + 3 * sizeof(float);
}

template <>
inline int print<VertexNC>(std::vector<std::string>& header)
{
    int ret = print<VertexN>(header);
    header.push_back("property float red");
    header.push_back("property float green");
    header.push_back("property float blue");
    return ret + 3 * sizeof(float);
}

template <>
inline void write(char* ptr, Vertex v)
{
    float* f = (float*)ptr;
    f[0]     = v.position.x();
    f[1]     = v.position.y();
    f[2]     = v.position.z();
}

template <>
inline void write(char* ptr, VertexN v)
{
    write(ptr, Vertex{v.position});
    float* f = (float*)ptr + 3;
    f[0]     = v.normal.x();
    f[1]     = v.normal.y();
    f[2]     = v.normal.z();
}

template <>
inline void write(char* ptr, VertexC v)
{
    write(ptr, Vertex{v.position});
    float* f = (float*)ptr + 3;
    f[0]     = v.color.x();
    f[1]     = v.color.y();
    f[2]     = v.color.z();
}

template <>
inline void write(char* ptr, VertexNC v)
{
    write(ptr, VertexN{v.position, v.normal});
    float* f = (float*)ptr + 6;
    f[0]     = v.color.x();
    f[1]     = v.color.y();
    f[2]     = v.color.z();
}
};  // namespace PLYLoaderDetail

}  // namespace Saiga
