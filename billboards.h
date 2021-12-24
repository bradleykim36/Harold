#ifndef BILLBOARDS_H_
#define BILLBOARDS_H_

#include <mingfx.h>
using namespace mingfx;

#include "ground.h"

/** Billboards are strokes planted in the ground that rotate automatically to
 face the viewer.  This class holds the entire collection of billboard strokes.
 */
class Billboards {
public:
    Billboards();
    virtual ~Billboards();
    
    void Init(ShaderProgram *stroke3d_shaderprog);
    
    /// Projects a 2D normalized screen point (e.g., the mouse position in normalized
    /// device coordinates) to a 3D point on a plane defined by an "origin", which can
    /// really be any point coincident with the plane, and the plane normal.  Returns
    /// true if the screen point projects onto the plane and stores the result in
    /// plane_point.  Returns false if the screen point does not project onto the plane.
    bool ScreenPtHitsPlane(const Matrix4 &view_matrix, const Matrix4 &proj_matrix,
                         const Point3 &plane_origin, const Vector3 &plane_normal,
                         const Point2 &normalized_screen_pt, Point3 *plane_point);
    
    
    /// Checks to see if a ray starting at the eye point and passing through 2D
    /// normalized screen point projects onto any of the billboards in the scene. If
    /// so, returns the id of the closest billboard intersected.  If not, returns -1.
    int IntersectBillboard(const Matrix4 &view_matrix, const Matrix4 &proj_matrix,
                           const Point2 &normalized_screen_pt);
    
    
    /// Adds a new stroke as a billboard by projecting it onto a plane parallel
    /// to the filmplane that intersects with the anchor point, which should lie
    /// on the ground.
    void AddBillboardStroke(const Matrix4 &view_matrix, const Matrix4 &proj_matrix,
                            const std::vector<Point2> &stroke2d, const Mesh &stroke2d_mesh,
                            const Color &stroke_color, Ground *ground_ptr);
    
    /// Edits an existing billboard by adding an additional stroke to it.
    void AddToBillboard(const Matrix4 &view_matrix, const Matrix4 &proj_matrix,
                        int billboard_id, const Mesh &stroke2d_mesh,
                        const Color &stroke_color);
    
    
    /// Adjusts the transformation matrix used to draw each billboard so that the
    /// billboard will face the camera.  Needs to be called each time the camera
    /// is moved within the scene.
    void UpdateBillboardRotations(const Point3 &current_eye_point);
    
    
    /// Draws all of the billboards
    void Draw(const Matrix4 &view_matrix, const Matrix4 &proj_matrix);
    
private:

    // each billboard stores the following
    struct Billboard {
        Mesh mesh;
        Color color;
        Point3 anchor_pt;
        AABB bounding_box;
        Matrix4 transform;
    };

    // the array of all active billboards
    std::vector<Billboard> billboards_;
    
    ShaderProgram *stroke3d_shaderprog_;
};

#endif
