#ifndef EDGE_MESH_H
#define EDGE_MESH_H

#include <mingfx.h>
using namespace mingfx;

#include <vector>


/** This special kind of mesh stores two triangles that form a quadralateral
 along each edge of an existing mesh.  The quad initially has a width=0, but
 when rendered, two of the vertices are extended along the surfaces normal
 direction, which creates a "fin" that can be drawn as a thick border.  This
 can be used to create a silhouette edge renderer if the shader only extends
 the edges that lie along a silhouette of the mesh.
 */
class EdgeMesh {
public:
    EdgeMesh();
    virtual ~EdgeMesh();

    /// Creates two triangles along each edge of the mesh passed in.
    void CreateFromMesh(const Mesh &mesh);

    /// Saves the mesh data to the GPU - must be called with InitOpenGL or Draw.
    void UpdateGPUMemory();
    
    /// Num vertices in the edge mesh
    int num_vertices() const;
    
    /// Num triangles in the edge mesh
    int num_triangles() const;
    
    /// Access to vertex position by vertex number
    Point3 vertex(int vertexID) const;

    /// Access to vertex normal by vertex number
    Vector3 normal(int vertexID) const;

    /// Access to vertex color by vertex number
    Color color(int vertexID) const;

    /// Access to vertex texture coords by vertex number
    Point2 tex_coords(int textureUnit, int vertexID) const;
    

    /// Draws the mesh assuming a shader has already been bound.
    void Draw();
    
private:
    
    // Some routines and variables are needed internally to construct the edge
    // mesh from a regular mesh.
    
    typedef std::map<std::pair<int,int>,int> EdgeMap;
    EdgeMap edgeMap;

    void addEdge(std::vector<Point3> *vertices,
                 std::vector<Vector3> *normals,
                 std::vector<Vector3> *leftNormals,
                 std::vector<Vector3> *rightNormals,
                 std::vector< std::vector<unsigned int> > *triangles,
                 const Mesh &mesh, int v0, int v1, Vector3 n);

    std::vector<float> verts_;          // vertex positions
    std::vector<float> norms_;          // normals
    std::vector<unsigned int> indices_; // indices
    std::vector<float> leftNorms_;      // normals of adjacent triangles
    std::vector<float> rightNorms_;

    GLuint vertexBuffer_;
    GLuint vertexArray_;
    GLuint elementBuffer_;

    bool gpuDirty_;
};

#endif
