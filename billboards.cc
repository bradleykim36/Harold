#include "billboards.h"

Billboards::Billboards() {
    
}


Billboards::~Billboards() {
    
}


void Billboards::Init(ShaderProgram *stroke3d_shaderprog) {
    stroke3d_shaderprog_ = stroke3d_shaderprog;
}


/// Projects a 2D normalized screen point (e.g., the mouse position in normalized
/// device coordinates) to a 3D point on a plane defined by an "origin", which can
/// really be any point coincident with the plane, and the plane normal.  Returns
/// true if the screen point projects onto the plane and stores the result in
/// plane_point.  Returns false if the screen point does not project onto the plane.
bool Billboards::ScreenPtHitsPlane(const Matrix4 &view_matrix, const Matrix4 &proj_matrix,
                                 const Point3 &plane_origin, const Vector3 &plane_normal,
                                 const Point2 &normalized_screen_pt, Point3 *plane_point)
{
    Matrix4 camera_matrix = view_matrix.Inverse();
    Point3 eye = camera_matrix.ColumnToPoint3(3);
    
    Point3 pt3d = GfxMath::ScreenToNearPlane(view_matrix, proj_matrix, normalized_screen_pt);
    Ray ray(eye, (pt3d - eye).ToUnit());
    float t;
    return ray.IntersectPlane(plane_origin, plane_normal, &t, plane_point);
}


/// Checks to see if a ray starting at the eye point and passing through 2D
/// normalized screen point projects onto any of the billboards in the scene. If
/// so, returns the id of the closest billboard intersected.  If not, returns -1.
int Billboards::IntersectBillboard(const Matrix4 &view_matrix, const Matrix4 &proj_matrix,
                                   const Point2 &normalized_screen_pt)
{
    Matrix4 camera_matrix = view_matrix.Inverse();
    Point3 eye = camera_matrix.ColumnToPoint3(3);
    
    int id = -1;
    float best_i_time = -1.0;
    
    Point3 pt3d = GfxMath::ScreenToNearPlane(view_matrix, proj_matrix, normalized_screen_pt);
    Ray ray(eye, (pt3d - eye).ToUnit());
    
    Point3 i_point;
    float i_time;
    int i_tri_id;
    for (int i=0; i<billboards_.size(); i++) {
        Matrix4 to_billboard_space = billboards_[i].transform.Inverse();
        Ray ray_billboard_space = to_billboard_space * ray;
        if ((ray_billboard_space.IntersectAABB(billboards_[i].bounding_box, &i_time)) &&
            (ray_billboard_space.IntersectMesh(billboards_[i].mesh, &i_time, &i_point, &i_tri_id)) &&
            ((best_i_time == -1.0) || (i_time < best_i_time))) {
            id = i;
        }
    }
    
    return id;
}


/// Adds a new stroke as a billboard by projecting it onto a plane parallel
/// to the filmplane that intersects with the base point, which should lie
/// on the ground.
void Billboards::AddBillboardStroke(const Matrix4 &view_matrix, const Matrix4 &proj_matrix,
                                    const std::vector<Point2> &stroke2d, const Mesh &stroke2d_mesh,
                                    const Color &stroke_color, Ground *ground_ptr)
{
    Matrix4 camera_matrix = view_matrix.Inverse();
    Point3 eye = camera_matrix.ColumnToPoint3(3);
    
    // Create a new billboard
    Billboard b;
    b.color = stroke_color;
    b.transform = Matrix4();
    b.mesh = stroke2d_mesh;
    
    // find the base point for the billboard
    ground_ptr->ScreenPtHitsGround(view_matrix, proj_matrix, stroke2d[0], &b.anchor_pt);
    Vector3 norm = (eye - b.anchor_pt);
    norm[1] = 0.0;    // with 0 change in Y so the billboard does not tilt
    norm.Normalize(); // convert to a unit vector
    
    // project the stroke into 3D to lie on the projection plane
    std::vector<Point3> verts;
    for (int i=0; i<b.mesh.num_vertices(); i++) {
        Point2 pscreen = Point2(b.mesh.read_vertex_data(i)[0], b.mesh.read_vertex_data(i)[1]);
        Point3 pplane;
        ScreenPtHitsPlane(view_matrix, proj_matrix, b.anchor_pt, norm, pscreen, &pplane);
        verts.push_back(pplane);
    }
    
    Matrix4 to_canonical_billboard = Matrix4::Align(b.anchor_pt, Vector3::UnitY(), norm,
                                                    Point3::Origin(), Vector3::UnitY(), Vector3::UnitZ());
    
    for (int i=0; i<verts.size(); i++) {
        verts[i] = to_canonical_billboard * verts[i];
    }
    
    b.mesh.SetVertices(verts);
    
    for (int i=0; i<b.mesh.num_vertices(); i++) {
        b.bounding_box = b.bounding_box + AABB(b.mesh.read_vertex_data(i));
    }
    b.bounding_box.set_user_data((int)billboards_.size());
    
    billboards_.push_back(b);
}


/// Edits an existing billboard by adding an additional stroke to it.
void Billboards::AddToBillboard(const Matrix4 &view_matrix, const Matrix4 &proj_matrix,
                                int billboard_id, const Mesh &stroke2d_mesh,
                                const Color &stroke_color)
{
    Matrix4 camera_matrix = view_matrix.Inverse();
    Point3 eye = camera_matrix.ColumnToPoint3(3);
    
    // Really what this does is add a new billboard that lies in the same plane
    // as the one we are "editing" and has the same anchor point so that they
    // will both rotate the same way.
    Billboard b;
    b.color = stroke_color;
    b.transform = Matrix4();
    b.mesh = stroke2d_mesh;
    
    b.anchor_pt = billboards_[billboard_id].anchor_pt;
    
    
    Vector3 norm = (eye - b.anchor_pt);
    norm[1] = 0.0;    // with 0 change in Y so the billboard does not tilt
    norm.Normalize(); // convert to a unit vector
    
    // project the stroke into 3D to lie on the projection plane
    std::vector<Point3> verts;
    for (int i=0; i<b.mesh.num_vertices(); i++) {
        Point2 pscreen = Point2(b.mesh.read_vertex_data(i)[0], b.mesh.read_vertex_data(i)[1]);
        Point3 pplane;
        ScreenPtHitsPlane(view_matrix, proj_matrix, b.anchor_pt, norm, pscreen, &pplane);
        verts.push_back(pplane);
    }
    
    Matrix4 to_canonical_billboard = Matrix4::Align(b.anchor_pt, Vector3::UnitY(), norm,
                                                    Point3::Origin(), Vector3::UnitY(), Vector3::UnitZ());
    
    for (int i=0; i<verts.size(); i++) {
        verts[i] = to_canonical_billboard * verts[i];
    }
    
    b.mesh.SetVertices(verts);
    
    for (int i=0; i<b.mesh.num_vertices(); i++) {
        b.bounding_box = b.bounding_box + AABB(b.mesh.read_vertex_data(i));
    }
    b.bounding_box.set_user_data((int)billboards_.size());
    
    billboards_.push_back(b);
}


void Billboards::UpdateBillboardRotations(const Point3 &current_eye_point) {
    // Whenver the camera moves, we also need to rotate the billboards so
    // that they always face the viewer.
    for (int i=0; i<billboards_.size(); i++) {
        // billboards are stored in a coordinate system where the base is
        // at (0,0,0), rotation happens about the Y axis, and the billboard's
        // normal is +Z.
        Point3 a_p = Point3(0,0,0);        // 'anchor point' in billboard coordinates
        Vector3 a_v1 = Vector3::UnitY();   // 'up' in billboard coordinates
        Vector3 a_v2 = Vector3::UnitZ();   // 'normal' in billboard coordinates
        
        Point3 b_p = billboards_[i].anchor_pt;    // 'anchor point' in world coordinates
        Vector3 b_v1 = Vector3::UnitY();          // 'up' in world coordinates
        Vector3 b_v2 = (current_eye_point - b_p); // 'normal' in world coordinates
        b_v2[1] = 0.0;    // force 0 change in Y so the billboard does not tilt
        b_v2.Normalize(); // convert to a unit vector
        
        billboards_[i].transform = Matrix4::Align(a_p,a_v1,a_v2, b_p,b_v1,b_v2);
    }
}


void Billboards::Draw(const Matrix4 &view_matrix, const Matrix4 &proj_matrix) {
    // Draw billboard meshes
    stroke3d_shaderprog_->UseProgram();
    stroke3d_shaderprog_->SetUniform("projectionMatrix", proj_matrix);
    for (int i=0; i<billboards_.size(); i++) {
        Matrix4 model_matrix = billboards_[i].transform;
        Matrix4 modelview_matrix = view_matrix * model_matrix;
        stroke3d_shaderprog_->SetUniform("modelViewMatrix", modelview_matrix);
        stroke3d_shaderprog_->SetUniform("strokeColor", billboards_[i].color);
        billboards_[i].mesh.Draw();
    }
    stroke3d_shaderprog_->StopProgram();
}
