/*
 * tinyply 2.3.4 (https://github.com/ddiakopoulos/tinyply)
 *
 * A single-header, zero-dependency (except the C++ STL) public domain implementation
 * of the PLY mesh file format. Requires C++11; errors are handled through exceptions.
 *
 * This software is in the public domain. Where that dedication is not
 * recognized, you are granted a perpetual, irrevocable license to copy,
 * distribute, and modify this file as you see fit.
 *
 * Authored by Dimitri Diakopoulos (http://www.dimitridiakopoulos.com)
 *
 * tinyply.h may be included in many files, however in a single compiled file,
 * the implementation must be created with the following defined prior to header inclusion
 * #define TINYPLY_IMPLEMENTATION
 *
 */

////////////////////////
//   tinyply header   //
////////////////////////

#ifndef tinyply_h
#define tinyply_h

#include <vector>
#include <string>
#include <stdint.h>
#include <cstddef>
#include <sstream>
#include <memory>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <functional>

#include <thread>
#include <chrono>
#include <fstream>
#include <iostream>
#include <cstring>
#include <iterator>

namespace tinyply
{

    enum class Type : uint8_t
    {
        INVALID,
        INT8,
        UINT8,
        INT16,
        UINT16,
        INT32,
        UINT32,
        FLOAT32,
        FLOAT64
    };

    struct PropertyInfo
    {
        PropertyInfo() {};
        PropertyInfo(int stride, std::string str)
            : stride(stride), str(str) {}
        int stride {0};
        std::string str;
    };

    static std::map<Type, PropertyInfo> PropertyTable
    {
        { Type::INT8,    PropertyInfo(1, std::string("char")) },
        { Type::UINT8,   PropertyInfo(1, std::string("uchar")) },
        { Type::INT16,   PropertyInfo(2, std::string("short")) },
        { Type::UINT16,  PropertyInfo(2, std::string("ushort")) },
        { Type::INT32,   PropertyInfo(4, std::string("int")) },
        { Type::UINT32,  PropertyInfo(4, std::string("uint")) },
        { Type::FLOAT32, PropertyInfo(4, std::string("float")) },
        { Type::FLOAT64, PropertyInfo(8, std::string("double")) },
        { Type::INVALID, PropertyInfo(0, std::string("INVALID"))}
    };

    class Buffer
    {
        uint8_t * alias{ nullptr };
        struct delete_array { void operator()(uint8_t * p) { delete[] p; } };
        std::unique_ptr<uint8_t, decltype(Buffer::delete_array())> data;
        size_t size {0};
    public:
        Buffer() {};
        Buffer(const size_t size) : data(new uint8_t[size], delete_array()), size(size) { alias = data.get(); } // allocating
        Buffer(const uint8_t * ptr): alias(const_cast<uint8_t*>(ptr)) { } // non-allocating, todo: set size?
        uint8_t * get() { return alias; }
        const uint8_t * get_const() const {return alias; }
        size_t size_bytes() const { return size; }
    };

    struct PlyData
    {
        Type t;
        Buffer buffer;
        size_t count {0};
        bool isList {false};
    };

    struct PlyProperty
    {
        PlyProperty(std::istream & is);
        PlyProperty(Type type, std::string & _name) : name(_name), propertyType(type) {}
        PlyProperty(Type list_type, Type prop_type, std::string & _name, size_t list_count)
            : name(_name), propertyType(prop_type), isList(true), listType(list_type), listCount(list_count) {}
        std::string name;
        Type propertyType{ Type::INVALID };
        bool isList{ false };
        Type listType{ Type::INVALID };
        size_t listCount {0};
    };

    struct PlyElement
    {
        PlyElement(std::istream & istream);
        PlyElement(const std::string & _name, size_t count) : name(_name), size(count) {}
        std::string name;
        size_t size {0};
        std::vector<PlyProperty> properties;
    };

    struct PlyFile
    {
        struct PlyFileImpl;
        std::unique_ptr<PlyFileImpl> impl;

        PlyFile();
        ~PlyFile();

        /*
         * The ply format requires an ascii header. This can be used to determine at
         * runtime which properties or elements exist in the file. Limited validation of the
         * header is performed; it is assumed the header correctly reflects the contents of the
         * payload. This function may throw. Returns true on success, false on failure.
         */
        bool parse_header(std::istream & is);

        /*
         * Execute a read operation. Data must be requested via `request_properties_from_element(...)`
         * prior to calling this function.
         */
        void read(std::istream & is);

        /*
         * `write` performs no validation and assumes that the data passed into
         * `add_properties_to_element` is well-formed.
         */
        void write(std::ostream & os, bool isBinary);

        /*
         * These functions are valid after a call to `parse_header(...)`. In the case of
         * writing, get_comments() reference may also be used to add new comments to the ply header.
         */
        std::vector<PlyElement> get_elements() const;
        std::vector<std::string> get_info() const;
        std::vector<std::string> & get_comments();
        bool is_binary_file() const;

        /*
         * In the general case where |list_size_hint| is zero, `read` performs a two-pass
         * parse to support variable length lists. The most general use of the
         * ply format is storing triangle meshes. When this fact is known a-priori, we can pass
         * an expected list length that will apply to this element. Doing so results in an up-front
         * memory allocation and a single-pass import, a 2x performance optimization.
         */
        std::shared_ptr<PlyData> request_properties_from_element(const std::string & elementKey,
            const std::vector<std::string> propertyKeys, const uint32_t list_size_hint = 0);

        void add_properties_to_element(const std::string & elementKey,
            const std::vector<std::string> propertyKeys,
            const Type type,
            const size_t count,
            const uint8_t * data,
            const Type listType,
            const size_t listCount);
    };

    // ---------------------------------------------------
    // Example usage
    // ---------------------------------------------------

    inline std::vector<uint8_t> read_file_binary(const std::string & pathToFile)
    {
        std::ifstream file(pathToFile, std::ios::binary);
        std::vector<uint8_t> fileBufferBytes;

        if (file.is_open())
        {
            file.seekg(0, std::ios::end);
            size_t sizeBytes = file.tellg();
            file.seekg(0, std::ios::beg);
            fileBufferBytes.resize(sizeBytes);
            if (file.read((char*)fileBufferBytes.data(), sizeBytes)) return fileBufferBytes;
        }
        else throw std::runtime_error("could not open binary ifstream to path " + pathToFile);
        return fileBufferBytes;
    }

    struct memory_buffer : public std::streambuf
    {
        char * p_start {nullptr};
        char * p_end {nullptr};
        size_t size;

        memory_buffer(char const * first_elem, size_t size)
            : p_start(const_cast<char*>(first_elem)), p_end(p_start + size), size(size)
        {
            setg(p_start, p_start, p_end);
        }

        pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) override
        {
            if (dir == std::ios_base::cur) gbump(static_cast<int>(off));
            else setg(p_start, (dir == std::ios_base::beg ? p_start : p_end) + off, p_end);
            return gptr() - p_start;
        }

        pos_type seekpos(pos_type pos, std::ios_base::openmode which) override
        {
            return seekoff(pos, std::ios_base::beg, which);
        }
    };

    struct memory_stream : virtual memory_buffer, public std::istream
    {
        memory_stream(char const * first_elem, size_t size)
            : memory_buffer(first_elem, size), std::istream(static_cast<std::streambuf*>(this)) {}
    };

    class manual_timer
    {
        std::chrono::high_resolution_clock::time_point t0;
        double timestamp{ 0.0 };
    public:
        void start() { t0 = std::chrono::high_resolution_clock::now(); }
        void stop() { timestamp = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count() * 1000.0; }
        const double & get() { return timestamp; }
    };

    struct float2 { float x, y; };
    struct float3 { float x, y, z; };
    struct double3 { double x, y, z; };
    struct uint3 { uint32_t x, y, z; };
    struct uint4 { uint32_t x, y, z, w; };

    struct geometry
    {
        std::vector<float3> vertices;
        std::vector<float3> normals;
        std::vector<float2> texcoords;
        std::vector<uint3> triangles;
    };

    inline geometry make_cube_geometry()
    {
        geometry cube;

        const struct CubeVertex { float3 position, normal; float2 texCoord; } verts[] = {
        { { -1, -1, -1 },{ -1, 0, 0 },{ 0, 0 } },{ { -1, -1, +1 },{ -1, 0, 0 },{ 1, 0 } },{ { -1, +1, +1 },{ -1, 0, 0 },{ 1, 1 } },{ { -1, +1, -1 },{ -1, 0, 0 },{ 0, 1 } },
        { { +1, -1, +1 },{ +1, 0, 0 },{ 0, 0 } },{ { +1, -1, -1 },{ +1, 0, 0 },{ 1, 0 } },{ { +1, +1, -1 },{ +1, 0, 0 },{ 1, 1 } },{ { +1, +1, +1 },{ +1, 0, 0 },{ 0, 1 } },
        { { -1, -1, -1 },{ 0, -1, 0 },{ 0, 0 } },{ { +1, -1, -1 },{ 0, -1, 0 },{ 1, 0 } },{ { +1, -1, +1 },{ 0, -1, 0 },{ 1, 1 } },{ { -1, -1, +1 },{ 0, -1, 0 },{ 0, 1 } },
        { { +1, +1, -1 },{ 0, +1, 0 },{ 0, 0 } },{ { -1, +1, -1 },{ 0, +1, 0 },{ 1, 0 } },{ { -1, +1, +1 },{ 0, +1, 0 },{ 1, 1 } },{ { +1, +1, +1 },{ 0, +1, 0 },{ 0, 1 } },
        { { -1, -1, -1 },{ 0, 0, -1 },{ 0, 0 } },{ { -1, +1, -1 },{ 0, 0, -1 },{ 1, 0 } },{ { +1, +1, -1 },{ 0, 0, -1 },{ 1, 1 } },{ { +1, -1, -1 },{ 0, 0, -1 },{ 0, 1 } },
        { { -1, +1, +1 },{ 0, 0, +1 },{ 0, 0 } },{ { -1, -1, +1 },{ 0, 0, +1 },{ 1, 0 } },{ { +1, -1, +1 },{ 0, 0, +1 },{ 1, 1 } },{ { +1, +1, +1 },{ 0, 0, +1 },{ 0, 1 } }};

        std::vector<uint4> quads = { { 0, 1, 2, 3 },{ 4, 5, 6, 7 },{ 8, 9, 10, 11 },{ 12, 13, 14, 15 },{ 16, 17, 18, 19 },{ 20, 21, 22, 23 } };

        for (auto & q : quads)
        {
            cube.triangles.push_back({ q.x,q.y,q.z });
            cube.triangles.push_back({ q.x,q.z,q.w });
        }

        for (int i = 0; i < 24; ++i)
        {
            cube.vertices.push_back(verts[i].position);
            cube.normals.push_back(verts[i].normal);
            cube.texcoords.push_back(verts[i].texCoord);
        }

        return cube;
    }


    inline void read(const std::string & filepath, std::vector<double>& v, std::vector<int>& f, bool debug = false)
    {
        std::cout << "........................................................................\n";
        std::cout << "Now Reading: " << filepath << std::endl;

        std::unique_ptr<std::istream> file_stream;
        std::vector<uint8_t> byte_buffer;

        bool preload_into_memory = true;
        
    try
        {
            // For most files < 1gb, pre-loading the entire file upfront and wrapping it into a 
            // stream is a net win for parsing speed, about 40% faster. 
            if (preload_into_memory)
            {
                byte_buffer = read_file_binary(filepath);
                file_stream.reset(new memory_stream((char*)byte_buffer.data(), byte_buffer.size()));
            }
            else
            {
                file_stream.reset(new std::ifstream(filepath, std::ios::binary));
            }

            if (!file_stream || file_stream->fail()) throw std::runtime_error("file_stream failed to open " + filepath);

            file_stream->seekg(0, std::ios::end);
            const float size_mb = file_stream->tellg() * float(1e-6);
            file_stream->seekg(0, std::ios::beg);

            PlyFile file;
            file.parse_header(*file_stream);

            std::cout << "\t[ply_header] Type: " << (file.is_binary_file() ? "binary" : "ascii") << std::endl;
            for (const auto & c : file.get_comments()) std::cout << "\t[ply_header] Comment: " << c << std::endl;
            for (const auto & c : file.get_info()) std::cout << "\t[ply_header] Info: " << c << std::endl;

            for (const auto & e : file.get_elements())
            {
                std::cout << "\t[ply_header] element: " << e.name << " (" << e.size << ")" << std::endl;
                for (const auto & p : e.properties)
                {
                    std::cout << "\t[ply_header] \tproperty: " << p.name << " (type=" << tinyply::PropertyTable[p.propertyType].str << ")";
                    if (p.isList) std::cout << " (list_type=" << tinyply::PropertyTable[p.listType].str << ")";
                    std::cout << std::endl;
                }
            }

            // Because most people have their own mesh types, tinyply treats parsed data as structured/typed byte buffers. 
            // See examples below on how to marry your own application-specific data structures with this one. 
            std::shared_ptr<PlyData> vertices, normals, colors, texcoords, faces, tripstrip;

            // The header information can be used to programmatically extract properties on elements
            // known to exist in the header prior to reading the data. For brevity of this sample, properties 
            // like vertex position are hard-coded: 
            try { vertices = file.request_properties_from_element("vertex", { "x", "y", "z" }); }
            catch (const std::exception & e) { std::cerr << "tinyply exception: " << e.what() << std::endl; }

            try { normals = file.request_properties_from_element("vertex", { "nx", "ny", "nz" }); }
            catch (const std::exception & e) { std::cerr << "tinyply exception: " << e.what() << std::endl; }

            try { colors = file.request_properties_from_element("vertex", { "red", "green", "blue", "alpha" }); }
            catch (const std::exception & e) { std::cerr << "tinyply exception: " << e.what() << std::endl; }

            try { colors = file.request_properties_from_element("vertex", { "r", "g", "b", "a" }); }
            catch (const std::exception & e) { std::cerr << "tinyply exception: " << e.what() << std::endl; }

            try { texcoords = file.request_properties_from_element("vertex", { "u", "v" }); }
            catch (const std::exception & e) { std::cerr << "tinyply exception: " << e.what() << std::endl; }

            // Providing a list size hint (the last argument) is a 2x performance improvement. If you have 
            // arbitrary ply files, it is best to leave this 0. 
            try { faces = file.request_properties_from_element("face", { "vertex_indices" }, 3); }
            catch (const std::exception & e) { std::cerr << "tinyply exception: " << e.what() << std::endl; }

            // Tristrips must always be read with a 0 list size hint (unless you know exactly how many elements
            // are specifically in the file, which is unlikely); 
            try { tripstrip = file.request_properties_from_element("tristrips", { "vertex_indices" }, 0); }
            catch (const std::exception & e) { std::cerr << "tinyply exception: " << e.what() << std::endl; }

            manual_timer read_timer;

            read_timer.start();
            file.read(*file_stream);
            read_timer.stop();

            const float parsing_time = static_cast<float>(read_timer.get()) / 1000.f;
            std::cout << "\tparsing " << size_mb << "mb in " << parsing_time << " seconds [" << (size_mb / parsing_time) << " MBps]" << std::endl;

            if (vertices)   std::cout << "\tRead " << vertices->count  << " total vertices "<< std::endl;
            if (normals)    std::cout << "\tRead " << normals->count   << " total vertex normals " << std::endl;
            if (colors)     std::cout << "\tRead " << colors->count << " total vertex colors " << std::endl;
            if (texcoords)  std::cout << "\tRead " << texcoords->count << " total vertex texcoords " << std::endl;
            if (faces)      std::cout << "\tRead " << faces->count     << " total faces (triangles) " << std::endl;
            if (tripstrip)  std::cout << "\tRead " << (tripstrip->buffer.size_bytes() / tinyply::PropertyTable[tripstrip->t].stride) << " total indices (tristrip) " << std::endl;

            // Example One: converting to your own application types
            {
                const size_t numVerticesBytes = vertices->buffer.size_bytes();
                std::vector<float> tempVertices(vertices->count * 3); // Assuming each vertex has 3 components (x, y, z)
                std::memcpy(tempVertices.data(), vertices->buffer.get(), numVerticesBytes);

                // Convert std::vector<float> to std::vector<double>
                v = std::vector<double>(tempVertices.begin(), tempVertices.end());

                if (debug)
                    for (size_t i = 0; i < v.size(); i += 3)
                        std::cout << "X: " << v[i] << " Y: " << v[i + 1] << " Z: " << v[i + 2] << std::endl;

                // Convert faces buffer to a flat vector of integers
                const size_t numFacesBytes = faces->buffer.size_bytes();
                f = std::vector<int>(faces->count * 3); // Assuming each face has 3 indices
                std::memcpy(f.data(), faces->buffer.get(), numFacesBytes);

                if (debug)
                    for (size_t i = 0; i < f.size(); i += 3)
                        std::cout << "X: " << f[i] << " Y: " << f[i + 1] << " Z: " << f[i + 2] << std::endl;
                
            }


        }
        catch (const std::exception & e)
        {
            std::cerr << "Caught tinyply exception: " << e.what() << std::endl;
        }
    }


} // end namespace tinyply

#endif // end tinyply_h
