#include "edge_mesh.h"

EdgeMesh::EdgeMesh() : gpuDirty_(true) {
    
}

EdgeMesh::~EdgeMesh() {
}



void EdgeMesh::CreateFromMesh(const Mesh &mesh) {
    verts_.clear();
    norms_.clear();
    indices_.clear();
    leftNorms_.clear();
    rightNorms_.clear();
    
    // temp data holders for use while constructing the mesh in an indexed primitives mode
    std::vector<Point3> vertices;                   // vertex positions
    std::vector<Vector3> normals;                   // vertex normals
    std::vector<Vector3> leftNormals, rightNormals; // normals of adj. faces
    std::vector< std::vector<unsigned int> > triangles;      // fin triangles

    for (int t = 0; t < mesh.num_triangles(); t++) {
        std::vector<unsigned int> tri = mesh.read_triangle_indices_data(t);
        Point3 a = mesh.read_vertex_data(tri[0]);
        Point3 b = mesh.read_vertex_data(tri[1]);
        Point3 c = mesh.read_vertex_data(tri[2]);
        Vector3 e1 = b-a;
        e1.Normalize();
        Vector3 e2 = c-a;
        e2.Normalize();
        Vector3 n = e1.Cross(e2);
        
        //Vector3 n = ((b-a).cross(c-a)).to_unit();
        //std::cout << n << std::endl;
        addEdge(&vertices, &normals, &leftNormals, &rightNormals, &triangles, mesh, tri[0], tri[1], n);
        addEdge(&vertices, &normals, &leftNormals, &rightNormals, &triangles, mesh, tri[1], tri[2], n);
        addEdge(&vertices, &normals, &leftNormals, &rightNormals, &triangles, mesh, tri[2], tri[0], n);
    }
    
    // now transfer data to the member variable data holders
    for (int i=0; i<vertices.size(); i++) {
        verts_.push_back(vertices[i][0]);
        verts_.push_back(vertices[i][1]);
        verts_.push_back(vertices[i][2]);
        norms_.push_back(normals[i][0]);
        norms_.push_back(normals[i][1]);
        norms_.push_back(normals[i][2]);
        leftNorms_.push_back(leftNormals[i][0]);
        leftNorms_.push_back(leftNormals[i][1]);
        leftNorms_.push_back(leftNormals[i][2]);
        rightNorms_.push_back(rightNormals[i][0]);
        rightNorms_.push_back(rightNormals[i][1]);
        rightNorms_.push_back(rightNormals[i][2]);
    }
    
    for (int i=0; i<triangles.size(); i++) {
        indices_.push_back(triangles[i][0]);
        indices_.push_back(triangles[i][1]);
        indices_.push_back(triangles[i][2]);
    }
    gpuDirty_ = true;
}


void EdgeMesh::addEdge(std::vector<Point3> *vertices,
                       std::vector<Vector3> *normals,
                       std::vector<Vector3> *leftNormals,
                       std::vector<Vector3> *rightNormals,
                       std::vector< std::vector<unsigned int> > *triangles,
                       const Mesh &mesh, int v0, int v1, Vector3 n)
{
    // edgeMap contains the index of the first of four consecutive vertices
    // that form the quad fin

    EdgeMap::iterator it = edgeMap.find(std::make_pair(v1, v0));
    if (it != edgeMap.end()) { // found
        int v = it->second;
        (*rightNormals)[v+0]
            = (*rightNormals)[v+1]
            = (*rightNormals)[v+2]
            = (*rightNormals)[v+3]
            = n;
    }
    else {
        int v = (int)vertices->size();
        edgeMap[std::make_pair(v0, v1)] = v;
        
        vertices->push_back(mesh.read_vertex_data(v0));
        vertices->push_back(mesh.read_vertex_data(v0));
        vertices->push_back(mesh.read_vertex_data(v1));
        vertices->push_back(mesh.read_vertex_data(v1));
        
        normals->push_back(Vector3());
        normals->push_back(mesh.read_normal_data(v0));
        normals->push_back(Vector3());
        normals->push_back(mesh.read_normal_data(v1));
        
        leftNormals->push_back(n);
        leftNormals->push_back(n);
        leftNormals->push_back(n);
        leftNormals->push_back(n);
        
        rightNormals->push_back(-n);
        rightNormals->push_back(-n);
        rightNormals->push_back(-n);
        rightNormals->push_back(-n);

        std::vector<unsigned int> tri1;
        tri1.push_back(v+0);
        tri1.push_back(v+2);
        tri1.push_back(v+3);
        triangles->push_back(tri1);
        
        std::vector<unsigned int> tri2;
        tri2.push_back(v+0);
        tri2.push_back(v+3);
        tri2.push_back(v+1);
        triangles->push_back(tri2);
    }
}



void EdgeMesh::UpdateGPUMemory() {
    if (gpuDirty_) {
        // sanity check -- for each attribute that is added make sure the number
        // of triangles is equal to the number of tris in the verts array.
        if (norms_.size() / 3 != num_vertices()) {
            std::cerr << "EdgeMesh::UpdateGPUMemory() -- warning: the number of per vertex normals in the mesh is not equal to the number vertices in the mesh. (N = " << norms_.size() / 3 << ", V = " << num_vertices() << ")" << std::endl;
        }
        if (leftNorms_.size() / 3 != num_vertices()) {
            std::cerr << "EdgeMesh::UpdateGPUMemory() -- warning: the number of per vertex left-normals in the mesh is not equal to the number vertices in the mesh. (LN = " << leftNorms_.size() / 3 << ", V = " << num_vertices() << ")" << std::endl;
        }
        if (rightNorms_.size() / 3 != num_vertices()) {
            std::cerr << "EdgeMesh::UpdateGPUMemory() -- warning: the number of per vertex right-normals in the mesh is not equal to the number vertices in the mesh. (RN = " << rightNorms_.size() / 3 << ", V = " << num_vertices() << ")" << std::endl;
        }
        
        GLsizeiptr totalMemSize = 0;
        
        GLsizeiptr vertsMemSize = verts_.size() * sizeof(float);
        GLsizeiptr vertsMemOffset = 0;
        totalMemSize += vertsMemSize;
        
        GLsizeiptr normsMemSize = norms_.size() * sizeof(float);
        GLsizeiptr normsMemOffset = totalMemSize;
        totalMemSize += normsMemSize;
        
        GLsizeiptr leftNormsMemSize = leftNorms_.size() * sizeof(float);
        GLsizeiptr leftNormsMemOffset = totalMemSize;
        totalMemSize += leftNormsMemSize;

        GLsizeiptr rightNormsMemSize = rightNorms_.size() * sizeof(float);
        GLsizeiptr rightNormsMemOffset = totalMemSize;
        totalMemSize += rightNormsMemSize;

        glGenBuffers(1, &vertexBuffer_);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
        glBufferData(GL_ARRAY_BUFFER, totalMemSize, NULL, GL_STATIC_DRAW);
        
        glBufferSubData(GL_ARRAY_BUFFER, vertsMemOffset, vertsMemSize, &verts_[0]);
        glBufferSubData(GL_ARRAY_BUFFER, normsMemOffset, normsMemSize, &norms_[0]);
        glBufferSubData(GL_ARRAY_BUFFER, leftNormsMemOffset, leftNormsMemSize, &leftNorms_[0]);
        glBufferSubData(GL_ARRAY_BUFFER, rightNormsMemOffset, rightNormsMemSize, &rightNorms_[0]);
        
        glGenVertexArrays(1, &vertexArray_);
        glBindVertexArray(vertexArray_);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
        
        // attribute 0 = vertices (required)
        int attribID = 0;
        int nComponents = 3;
        glEnableVertexAttribArray(attribID);
        glVertexAttribPointer(attribID, nComponents, GL_FLOAT, GL_FALSE, nComponents*sizeof(GLfloat), (char*)0 + vertsMemOffset);
        
        // attribute 1 = normals (required)
        attribID = 1;
        nComponents = 3;
        glEnableVertexAttribArray(attribID);
        glVertexAttribPointer(attribID, nComponents, GL_FLOAT, GL_TRUE, nComponents*sizeof(GLfloat), (char*)0 + normsMemOffset);

        // attribute 2 = left normals (required)
        attribID = 2;
        nComponents = 3;
        glEnableVertexAttribArray(attribID);
        glVertexAttribPointer(attribID, nComponents, GL_FLOAT, GL_TRUE, nComponents*sizeof(GLfloat), (char*)0 + leftNormsMemOffset);

        // attribute 3 = right normals (required)
        attribID = 3;
        nComponents = 3;
        glEnableVertexAttribArray(attribID);
        glVertexAttribPointer(attribID, nComponents, GL_FLOAT, GL_TRUE, nComponents*sizeof(GLfloat), (char*)0 + rightNormsMemOffset);
        
        glBindVertexArray(0);
        
        
        glGenBuffers(1, &elementBuffer_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(unsigned int), &indices_[0], GL_STATIC_DRAW);
        
        
        gpuDirty_ = false;
    }
}


void EdgeMesh::Draw() {
    if (gpuDirty_) {
        UpdateGPUMemory();
    }
    
    // set defaults to pass to shaders any for optional attribs
    glVertexAttrib3f(1, 0.0, 0.0, 1.0);      // normal = +Z
    glVertexAttrib3f(2, 0.0, 0.0, 1.0);      // left-normal = +Z
    glVertexAttrib3f(3, 0.0, 0.0, 1.0);      // right-normal = +Z
    
    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer_);
    glDrawElements(GL_TRIANGLES, (GLsizei)indices_.size(), GL_UNSIGNED_INT, (void*)0);
    glBindVertexArray(0);
}


int EdgeMesh::num_vertices() const {
    return (int)verts_.size()/3;
}

int EdgeMesh::num_triangles() const {
    return (int)indices_.size()/3;
}
