#ifndef GROUND_H_
#define GROUND_H_

#include <mingfx.h>
using namespace mingfx;

#include "edge_mesh.h"

/** The ground is represented with a triangle mesh.  2D "screen space" strokes
 are used to modify the vertices based on user input so that the user can create
 a 3D landscape of hills and valleys.
 */
class Ground {
public:
    Ground();
    virtual ~Ground();
    
    /// Call from InitOpenGL() to initialize shaders, etc.
    void Init(const std::vector<std::string> &search_path);
    
    /// Projects a 2D normalized screen point (e.g., the mouse position in normalized
    /// device coordinates) to a 3D point on the ground.  Returns true and sets ground_point
    /// to be equal to the result if the conversion is successful.  Returns false if
    /// the screen point does not project onto the ground.
    bool ScreenPtHitsGround(const Matrix4 &view_matrix, const Matrix4 &proj_matrix,
                          const Point2 &normalized_screen_pt, Point3 *ground_point);

    /// Modifies the vertices of the ground mesh to create a hill or valley based
    /// on the input stroke.  The 2D path of the stroke on the screen is passed
    /// in, this is the centerline of the stroke mesh that is actually drawn on
    /// the screen while the user is drawing.
    void ReshapeGround(const Matrix4 &view_matrix, const Matrix4 &proj_matrix,
                       const std::vector<Point2> &stroke2d);
    
    /// Draws the ground mesh with toon shading
    void Draw(const Matrix4 &view_matrix, const Matrix4 &proj_matrix, const Color &ground_color);
    
    
    Mesh* mesh_ptr();
    
private:
    
    // This is the actual triangle mesh for the ground
    Mesh ground_mesh_;
    // We also maintain a corresponding "edge mesh" in order to do the
    // silhouette outlines like in assignment 5
    EdgeMesh ground_edge_mesh_;
    
    // The ground rendering is based on the artsy shader from assignment 5
    ShaderProgram artsy_shaderprog_;
    ShaderProgram outline_shaderprog_;
    Texture2D diffuse_ramp_;
    Texture2D specular_ramp_;
    Point3 light_pos_;
    
    // for debugging only
    QuickShapes qs_;
};

#endif
