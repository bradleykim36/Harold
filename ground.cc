#include "ground.h"


Ground::Ground() : diffuse_ramp_(GL_CLAMP_TO_EDGE),
    specular_ramp_(GL_CLAMP_TO_EDGE), light_pos_(30,30,30)
{
    
}

Ground::~Ground() {
    
}

Mesh* Ground::mesh_ptr() { return &ground_mesh_; }


void Ground::Init(const std::vector<std::string> &search_path) {
    // init ground geometry, a simple grid is used.  if it is running too slow,
    // you can turn down the resolution by decreasing nx and ny, but this will
    // make the hills look more jaggy.
    const int nx = 150;
    const int ny = 150;
    const float size = 100.0;
    std::vector<Point3> verts;
    std::vector<Vector3> norms;
    for (int j = 0; j <= ny; j++) {
        for (int i = 0; i <= nx; i++) {
            float x = size*(float)j/nx - size/2.0f;
            float y = size*(float)i/ny - size/2.0f;
            verts.push_back(Point3(x, 0, y));
            norms.push_back(Vector3(0,1,0));
        }
    }
    std::vector<unsigned int> indices;
    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
            // L\ triangle
            indices.push_back((i+0)+(j+0)*(nx+1));
            indices.push_back((i+1)+(j+0)*(nx+1));
            indices.push_back((i+0)+(j+1)*(nx+1));
            // \7 triangle
            indices.push_back((i+1)+(j+0)*(nx+1));
            indices.push_back((i+1)+(j+1)*(nx+1));
            indices.push_back((i+0)+(j+1)*(nx+1));
        }
    }
    ground_mesh_.SetIndices(indices);
    ground_mesh_.SetVertices(verts);
    ground_mesh_.SetNormals(norms);
    ground_mesh_.UpdateGPUMemory();
    ground_edge_mesh_.CreateFromMesh(ground_mesh_);

    
    // load textures and shaders
    diffuse_ramp_.InitFromFile(Platform::FindFile("toonDiffuse.png", search_path));
    specular_ramp_.InitFromFile(Platform::FindFile("toonSpecular.png", search_path));
    
    artsy_shaderprog_.AddVertexShaderFromFile(Platform::FindFile("artsy.vert", search_path));
    artsy_shaderprog_.AddFragmentShaderFromFile(Platform::FindFile("artsy.frag", search_path));
    artsy_shaderprog_.LinkProgram();
    
    outline_shaderprog_.AddVertexShaderFromFile(Platform::FindFile("outline.vert", search_path));
    outline_shaderprog_.AddFragmentShaderFromFile(Platform::FindFile("outline.frag", search_path));
    outline_shaderprog_.LinkProgram();
}




// Projects a 2D normalized screen point (e.g., the mouse position in normalized
// device coordinates) to a 3D point on the ground.  Returns true and sets ground_point
// to be equal to the result if the conversion is successful.  Returns false if
// the screen point does not project onto the ground.
bool Ground::ScreenPtHitsGround(const Matrix4 &view_matrix, const Matrix4 &proj_matrix,
                              const Point2 &normalized_screen_pt, Point3 *ground_point)
{
    Matrix4 camera_matrix = view_matrix.Inverse();
    Point3 eye = camera_matrix.ColumnToPoint3(3);
    
    Point3 pt3d = GfxMath::ScreenToNearPlane(view_matrix, proj_matrix, normalized_screen_pt);
    Ray ray(eye, (pt3d - eye).ToUnit());
    float i_time;
    int i_tri;
    return ray.FastIntersectMesh(&ground_mesh_, &i_time, ground_point, &i_tri);
}




/** This implements the "h" term used in the equations described in section 4.5 of the
 paper. Three arguments are needed:
 1. projection_plane_normal: We need to know where the projection plane is in 3-space
    Since a plane can be defined by a point within the plane and a normal, we use
    this normal together with the 3rd argument to the function to define the projection
    plane described in the paper.
 2. silhouette_curve: As described in the paper, the silhouette curve is a 3D version
    of the curve the user draws with the mouse.  It is formed by projecting the
    original 2D screen-space curve onto the 3D projection plane.
 3. closest_pt_in_plane: As described in the paper, this is the closest point within
    the projection plane to the vertex of the mesh that we want to modify.  In other
    words, it is the perpendicular projection of the vertex we want to modify onto
    the projection plane.
 */
float hfunc(const Vector3 projection_plane_normal, const std::vector<Point3> &silhouette_curve, const Point3 &closest_pt_in_plane) {
    // define the y axis for a "plane space" coordinate system as a world space vector
    Vector3 plane_y = Vector3(0,1,0);
    // define the x axis for a "plane space" coordinate system as a world space vector
    Vector3 plane_x = plane_y.Cross(projection_plane_normal).ToUnit();
    // define the origin for a "plane space" coordinate system as the first point in the curve
    Point3 origin = silhouette_curve[0];
    
    // loop over line segments in the curve, find the one that lies over the point by
    // comparing the "plane space" x value for the start and end of the line segment
    // to the "plane space" x value for the closest point to the vertex that lies
    // in the projection plane.
    float x_target = (closest_pt_in_plane - origin).Dot(plane_x);
    for (int i=1; i<silhouette_curve.size(); i++) {
        float x_start = (silhouette_curve[(size_t)i-1] - origin).Dot(plane_x);
        float x_end = (silhouette_curve[i] - origin).Dot(plane_x);
        if ((x_start <= x_target) && (x_target <= x_end)) {
            float alpha = (x_target - x_start) / (x_end - x_start);
            float y_curve = silhouette_curve[(size_t)i-1][1] + alpha*(silhouette_curve[i][1] - silhouette_curve[(size_t)i-1][1]);
            return y_curve - closest_pt_in_plane[1];
        }
        else if ((x_end <= x_target) && (x_target <= x_start)) {
            float alpha = (x_target - x_end) / (x_start - x_end);
            float y_curve = silhouette_curve[i][1] + alpha*(silhouette_curve[(size_t)i-1][1] - silhouette_curve[i][1]);
            return y_curve - closest_pt_in_plane[1];
        }
    }
    
    // here return 0 because the point does not lie under the curve.
    return 0.0;
}




/// Modifies the vertices of the ground mesh to create a hill or valley based
/// on the input stroke.  The 2D path of the stroke on the screen is passed
/// in, this is the centerline of the stroke mesh that is actually drawn on
/// the screen while the user is drawing.
void Ground::ReshapeGround(const Matrix4 &view_matrix, const Matrix4 &proj_matrix,
                           const std::vector<Point2> &stroke2d)
{
    // TODO: Deform the 3D ground mesh according to the algorithm described in the
    // Cohen et al. Harold paper.
    
    // You might need the eye point and the look vector, these can be determined
    // from the view matrix as follows:
    Matrix4 camera_matrix = view_matrix.Inverse();
    Point3 eye = camera_matrix.ColumnToPoint3(3);
    Vector3 look = -camera_matrix.ColumnToVector3(2);
    
    // There are 3 major steps to the algorithm, outlined here:
    
    // 1. Define a plane to project the stroke onto.  The first and last points
    // of the stroke are guaranteed to project onto the ground plane.  The plane
    // should pass through these two points on the ground.  The plane should also
    // have a normal vector that points toward the camera and is parallel to the
    // ground plane.

    Point3 start;
    Point3 end;
    ScreenPtHitsGround(view_matrix, proj_matrix, stroke2d[0], &start);
    ScreenPtHitsGround(view_matrix, proj_matrix, stroke2d[stroke2d.size() - 1], &end);

    Vector3 plane_x = Vector3::Normalize(start - end);
    Vector3 plane_y = Vector3(0, 1, 0);
    Vector3 normal = Vector3::Normalize(-look);

    // 2. Project the 2D stroke into 3D so that it lies on the "projection plane"
    // defined in step 1.

    std::vector<Point3> projection_plane;
    for (int i = 0; i < stroke2d.size() - 1; i++) {
        float temp;
        Point3 plane;

        Point3 mouseIn3d = GfxMath::ScreenToNearPlane(view_matrix, proj_matrix, stroke2d[i]);
        Ray eyeThroughMouse(eye, (mouseIn3d - eye).ToUnit());

        eyeThroughMouse.IntersectPlane(start, normal, &temp, &plane);
        projection_plane.push_back(plane);
    }

    // 3. Loop through all of the vertices of the ground mesh, and adjust the
    // height of each based on the equations in section 4.5 of the paper, also
    // repeated in the assignment handout.  The equations rely upon a function
    // h(), and we have implemented that for you as hfunc() defined above in
    // this file.  The basic structure of the loop you will need is here:
    std::vector<Point3> vertices;
    for (int i = 0; i < ground_mesh_.num_vertices(); i++) {
        Point3 temp = ground_mesh_.read_vertex_data(i);
        float h = hfunc(normal, projection_plane, temp.ClosestPointOnPlane(start, normal));
        float wd = 0;
        float d = temp.DistanceToPlane(start, normal);
        if ((1 - pow(d / 5, 2)) > 0) {
            wd = 1 - pow(d / 5, 2);
        }
        if (h != 0) {
            temp = Point3(temp[0], (1 - wd) * temp[1] + wd * h, temp[2]);
        }
        vertices.push_back(temp);
    }
    ground_mesh_.SetVertices(vertices);
    ground_mesh_.CalcPerVertexNormals();
    ground_mesh_.UpdateGPUMemory();
    ground_edge_mesh_.CreateFromMesh(ground_mesh_);
}




/// Draws the ground mesh with toon shading
void Ground::Draw(const Matrix4 &view_matrix, const Matrix4 &proj_matrix, const Color &ground_color) {
    // Lighting parameters
    Color Ia(1.0f, 1.0f, 1.0f, 1.0f);
    Color Id(1.0f, 1.0f, 1.0f, 1.0f);
    Color Is(1.0f, 1.0f, 1.0f, 1.0f);
    
    // Material parameters
    Color ka = ground_color;
    Color kd(0.4f, 0.4f, 0.4f, 1.0f);
    Color ks(0.6f, 0.6f, 0.6f, 1.0f);
    float s = 50.0f;
    
    // Precompute matrices needed in the shader
    Matrix4 model_matrix; // identity
    Matrix4 modelview_matrix = view_matrix * model_matrix;
    Matrix4 normal_matrix = modelview_matrix.Inverse().Transpose();
    Point3 light_in_eye_space = view_matrix * light_pos_;
    
    // Make sure the default option to only draw front facing triangles is set
    glEnable(GL_CULL_FACE);
    
    
    // Draw the ground using the artsy shader
    artsy_shaderprog_.UseProgram();
    artsy_shaderprog_.SetUniform("modelViewMatrix", modelview_matrix);
    artsy_shaderprog_.SetUniform("normalMatrix", normal_matrix);
    artsy_shaderprog_.SetUniform("projectionMatrix", proj_matrix);
    artsy_shaderprog_.SetUniform("ka", ka);
    artsy_shaderprog_.SetUniform("kd", kd);
    artsy_shaderprog_.SetUniform("ks", ks);
    artsy_shaderprog_.SetUniform("s", s);
    artsy_shaderprog_.SetUniform("lightPosition", light_in_eye_space);
    artsy_shaderprog_.SetUniform("Ia", Ia);
    artsy_shaderprog_.SetUniform("Id", Id);
    artsy_shaderprog_.SetUniform("Is", Is);
    artsy_shaderprog_.BindTexture("diffuseRamp", diffuse_ramp_);
    artsy_shaderprog_.BindTexture("specularRamp", specular_ramp_);
    ground_mesh_.Draw();
    artsy_shaderprog_.StopProgram();
    
    // And, draw silhouette edges for the ground using the outline shader
    glDisable(GL_CULL_FACE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1,1);
    static const float thickness = 0.2f;
    outline_shaderprog_.UseProgram();
    outline_shaderprog_.SetUniform("modelViewMatrix", modelview_matrix);
    outline_shaderprog_.SetUniform("normalMatrix", normal_matrix);
    outline_shaderprog_.SetUniform("projectionMatrix", proj_matrix);
    outline_shaderprog_.SetUniform("thickness", thickness);
    ground_edge_mesh_.Draw();
    outline_shaderprog_.StopProgram();
    

    
    // This can be useful for debugging, but it is extremely slow to draw.
    // Before uncommenting this, it's recommended to turn down the resolution
    // of the ground mesh by adjusting the nx and ny constants inside Init().
    /**
     // draw lines around each triangle
     for (int t=0; t<ground_mesh_.num_triangles(); t++) {
         std::vector<unsigned int> indices = ground_mesh_.triangle_vertices(t);
         std::vector<Point3> loop;
         loop.push_back(ground_mesh_.vertex(indices[0]));
         loop.push_back(ground_mesh_.vertex(indices[1]));
         loop.push_back(ground_mesh_.vertex(indices[2]));
         qs_.DrawLines(model_matrix, view_matrix, proj_matrix, Color(0.7,0.7,0.7), loop, QuickShapes::LinesType::LINE_LOOP, 0.01);
     }
     
     // draw normals
     for (int i=0; i<ground_mesh_.num_vertices(); i++) {
         Point3 p1 = ground_mesh_.vertex(i);
         Point3 p2 = p1 + 0.5*ground_mesh_.normal(i);
         qs_.DrawLineSegment(model_matrix, view_matrix, proj_matrix, Color(0.7,0.7,0.7), p1, p2, 0.01);
     }
    **/
}
