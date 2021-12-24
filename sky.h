#ifndef SKY_H_
#define SKY_H_

#include <mingfx.h>
using namespace mingfx;

/** Creates, holds, and draws all of the strokes on the sky.
 */
class Sky {
public:
    Sky();
    virtual ~Sky();
    
    void Init(ShaderProgram *stroke3d_shaderprog);
    
    /// Projects a 2D normalized screen point (e.g., the mouse position in normalized
    /// device coordinates) to a 3D point on the "sky", which is really a huge sphere
    /// (radius = 1500) that the viewer is inside.  This function should always return
    /// true since any screen point can successfully be projected onto the sphere.
    /// sky_point is set to the resulting 3D point.  Note, this function only checks
    /// to see if the ray passing through the screen point intersects the sphere; it
    /// does not check to see if the ray hits the ground or anything else first.
    bool ScreenPtHitsSky(const Matrix4 &view_matrix, const Matrix4 &proj_matrix,
                       const Point2 &normalized_screen_pt, Point3 *sky_point);

    /// Creates a new sky stroke mesh by projecting each vertex of the 2D mesh
    /// onto the sky dome and saving the result as a new 3D mesh.
    void AddSkyStroke(const Matrix4 &view_matrix, const Matrix4 &proj_matrix,
                      const Mesh &stroke2d_mesh, const Color &stroke_color);
    
    /// Draws all of the sky strokes
    void Draw(const Matrix4 &view_matrix, const Matrix4 &proj_matrix);
    
private:

    // Each stroke has a 3D mesh and a color
    struct SkyStroke {
        Mesh mesh;
        Color color;
    };
    
    // To store a new stroke to draw, add it to this array
    std::vector<SkyStroke> strokes_;

    
    ShaderProgram *stroke3d_shaderprog_;
};

#endif
