#include "harold_app.h"
#include "config.h"

#include <iostream>
#include <sstream>


HaroldApp::HaroldApp() : GraphicsApp(1024,768, "Harold"),
    drawing_state_(DRAWING_NONE),
    sky_color_(1,1,1), ground_color_(0.25, 0, 0.25), crayon_color_(0.5,0,0.5)
{
    // Define a search path for finding data files (images and shaders)
    search_path_.push_back(".");
    search_path_.push_back("./data");
    search_path_.push_back("./shaders");
    search_path_.push_back(DATA_DIR_INSTALL);
    search_path_.push_back(DATA_DIR_BUILD);
    search_path_.push_back(SHADERS_DIR_INSTALL);
    search_path_.push_back(SHADERS_DIR_BUILD);
}


HaroldApp::~HaroldApp() {
}


void HaroldApp::InitNanoGUI() {
    // Setup the GUI window
    nanogui::Window *window = new nanogui::Window(screen(), "Harold's Crayons");
    window->setPosition(Eigen::Vector2i(10, 10));
    window->setSize(Eigen::Vector2i(200,100));
    window->setLayout(new nanogui::GroupLayout());

    new nanogui::Label(window, "Crayon Color", "sans-bold");
    auto cp1 = new nanogui::ColorPicker(window,
        nanogui::Color((int)(255.0*crayon_color_[0]),
                       (int)(255.0*crayon_color_[1]),
                       (int)(255.0*crayon_color_[2]), 255)
    );
    cp1->setFixedSize({100, 20});
    cp1->setFinalCallback([this](const nanogui::Color &c) {
        crayon_color_ = Color(c.r(), c.g(), c.b(), c.w());
    });

    new nanogui::Label(window, "Sky Color", "sans-bold");
    auto cp2 = new nanogui::ColorPicker(window,
        nanogui::Color((int)(255.0*sky_color_[0]),
                       (int)(255.0*sky_color_[1]),
                       (int)(255.0*sky_color_[2]), 255)
    );
    cp2->setFixedSize({100, 20});
    cp2->setFinalCallback([this](const nanogui::Color &c) {
        sky_color_ = Color(c.r(), c.g(), c.b(), c.w());
    });

    new nanogui::Label(window, "Ground Color", "sans-bold");
    auto cp3 = new nanogui::ColorPicker(window,
        nanogui::Color((int)(255.0*ground_color_[0]),
                       (int)(255.0*ground_color_[1]),
                       (int)(255.0*ground_color_[2]), 255)
    );
    cp3->setFixedSize({100, 20});
    cp3->setFinalCallback([this](const nanogui::Color &c) {
        ground_color_ = Color(c.r(), c.g(), c.b(), c.w());
    });

    screen()->performLayout();
}


void HaroldApp::InitOpenGL() {
    // Set up the camera in a good position to see the entire field
    cam_.set_view_matrix(Matrix4::LookAt(Point3(0,2,10), Point3(0,2,0), Vector3(0,1,0)));
    proj_matrix_ = Matrix4::Perspective(60, aspect_ratio(), 0.1f, 1600.0f);
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    
    stroke2d_shaderprog_.AddVertexShaderFromFile(Platform::FindFile("stroke2d.vert", search_path_));
    stroke2d_shaderprog_.AddFragmentShaderFromFile(Platform::FindFile("stroke2d.frag", search_path_));
    stroke2d_shaderprog_.LinkProgram();
    
    stroke3d_shaderprog_.AddVertexShaderFromFile(Platform::FindFile("stroke3d.vert", search_path_));
    stroke3d_shaderprog_.AddFragmentShaderFromFile(Platform::FindFile("stroke3d.frag", search_path_));
    stroke3d_shaderprog_.LinkProgram();
    
    ground_.Init(search_path_);
    sky_.Init(&stroke3d_shaderprog_);
    billboards_.Init(&stroke3d_shaderprog_);
}



void HaroldApp::AddToStroke(const Point2 &normalized_screen_pt) {
    // the stroke2d_ array stores the raw 2D screen coordinates of the
    // centerline of the stroke
    stroke2d_.push_back(normalized_screen_pt);
    
    // the mesh is a triangle strip that follows the centerline
    // we need at least 2 samples before we can create triangles
    if (stroke2d_.size() >= 2) {
        const float half_stroke_width = 0.01f;
        std::vector<Point3> verts;
        std::vector<unsigned int> indices;
        Point3 last_pt = Point3(stroke2d_[0][0], stroke2d_[0][1], 0);
        Point3 pt = Point3(stroke2d_[1][0], stroke2d_[1][1], 0);
        Vector3 tangent = (pt - last_pt).ToUnit();
        Vector3 cotangent = tangent.Cross(Vector3::UnitZ());
        verts.push_back(last_pt - half_stroke_width*cotangent);
        verts.push_back(last_pt + half_stroke_width*cotangent);
        for (int i=1; i<stroke2d_.size(); i++) {
            pt = Point3(stroke2d_[i][0], stroke2d_[i][1], 0);
            tangent = (pt - last_pt).ToUnit();
            cotangent = tangent.Cross(Vector3::UnitZ());
            
            verts.push_back(pt - half_stroke_width*cotangent);
            verts.push_back(pt + half_stroke_width*cotangent);
            
            indices.push_back((int)verts.size()-4);
            indices.push_back((int)verts.size()-3);
            indices.push_back((int)verts.size()-2);

            indices.push_back((int)verts.size()-3);
            indices.push_back((int)verts.size()-1);
            indices.push_back((int)verts.size()-2);
            
            last_pt = pt;
        }
        stroke2d_mesh_.SetVertices(verts);
        stroke2d_mesh_.SetIndices(indices);
        stroke2d_mesh_.UpdateGPUMemory();
    }
}



// This function is called at the start of each new crayon stroke
void HaroldApp::OnLeftMouseDown(const Point2 &mouse_in_pixels) {
    // Add to the stroke_mesh_, which is a 2D triangle strip used to draw the user's
    // crayon stroke on the screen.
    Point2 mouse_in_ndc = PixelsToNormalizedDeviceCoords(mouse_in_pixels);
    AddToStroke(mouse_in_ndc);
    
    
    // Next, try to figure out what we are drawing based on where the stroke originated.
    Point3 i_point;
    edit_billboard_id_ = billboards_.IntersectBillboard(cam_.view_matrix(), proj_matrix_, mouse_in_ndc);
    if (edit_billboard_id_ >= 0) {
        // If the mouse starts on an existing billboard, then we are editing the billboard.
        drawing_state_ = DrawingState::DRAWING_BILLBOARD_EDIT;
    }
    else if (ground_.ScreenPtHitsGround(cam_.view_matrix(), proj_matrix_, mouse_in_ndc, &i_point)) {
        // If the mouse starts on the ground, then we could be about to edit the
        // ground, OR we might be creating a new billboard.  We won't know for sure
        // until the user releases the mouse and we can check to see whether the
        // stroke also ends on the ground.
        drawing_state_ = DrawingState::DRAWING_GROUND_OR_BILLBOARD;
    }
    else {
        // Otherwise, we must be drawing a stroke in the sky.
        drawing_state_ = DrawingState::DRAWING_SKY;
    }
}


// This function is called once each frame while the user is drawing with the crayon
void HaroldApp::OnLeftMouseDrag(const Point2 &mouse_in_pixels, const Vector2 &delta_in_pixels) {
    // Add to the stroke_mesh_, which is a 2D triangle strip used to draw the user's
    // crayon stroke on the screen.
    Point2 mouse_in_ndc = PixelsToNormalizedDeviceCoords(mouse_in_pixels);
    AddToStroke(mouse_in_ndc);
}


// This function is called at the end of each stroke
void HaroldApp::OnLeftMouseUp(const Point2 &mouse_in_pixels) {
    
    // If we are in the temporary drawing_ground_or_billboard state, then we need
    // to do a final check now to see if the stroke ended on the ground or not.
    // If it did, then we interpret the stroke as drawing_ground.  Otherwise, we
    // treat it as creating a new billboard.
    if (drawing_state_ == DrawingState::DRAWING_GROUND_OR_BILLBOARD) {
        // The stroke was started on the ground, does it also end on the ground?
        Point2 mouse_in_ndc = PixelsToNormalizedDeviceCoords(mouse_in_pixels);
        Point3 i_point;
        if (ground_.ScreenPtHitsGround(cam_.view_matrix(), proj_matrix_, mouse_in_ndc, &i_point)) {
            drawing_state_ = DrawingState::DRAWING_GROUND;
        }
        else {
            drawing_state_ = DrawingState::DRAWING_BILLBOARD;
        }
    }
    
    
    // Now, the action to take in terms of what geometry to add or modify in
    // the scene depends entirely on the drawing state:
    if (drawing_state_ == DrawingState::DRAWING_SKY) {
        sky_.AddSkyStroke(cam_.view_matrix(), proj_matrix_, stroke2d_mesh_, crayon_color_);
    }
    else if (drawing_state_ == DrawingState::DRAWING_BILLBOARD) {
        billboards_.AddBillboardStroke(cam_.view_matrix(), proj_matrix_, stroke2d_, stroke2d_mesh_, crayon_color_, &ground_);
    }
    else if (drawing_state_ == DrawingState::DRAWING_BILLBOARD_EDIT) {
        billboards_.AddToBillboard(cam_.view_matrix(), proj_matrix_, edit_billboard_id_, stroke2d_mesh_, crayon_color_);
    }
    else if (drawing_state_ == DrawingState::DRAWING_GROUND) {
        if (stroke2d_.size() < 6) {
            std::cout << "Stroke is too short, try again." << std::endl;
        }
        else {
            ground_.ReshapeGround(cam_.view_matrix(), proj_matrix_, stroke2d_);
        }
    }
    
    
    // Done with this stroke.  Clear the 2d stroke and its mesh and reset the drawing state
    stroke2d_.clear();
    stroke2d_mesh_ = Mesh();
    drawing_state_ = DrawingState::DRAWING_NONE;
}


// You can look around, like in minecraft, by dragging with the right mouse button.
void HaroldApp::OnRightMouseDrag(const Point2 &mouse_in_pixels, const Vector2 &delta_in_pixels) {
    Vector2 delta_in_ndc = PixelsToNormalizedDeviceCoords(delta_in_pixels);
    cam_.OnMouseMove(delta_in_ndc);
}


void HaroldApp::UpdateSimulation(double dt)  {
    if (drawing_state_ == DrawingState::DRAWING_NONE) {
        // When walking around using the arrow keys we need to adjust the height
        // of the virtual camera when we walk up a hill.  To do that, we shoot
        // a ray straight down from the eye point to the ground, find the point
        // of intersection on the ground, and then set the camera height to be
        // 2.0 meters above this.
        Ray ray(cam_.eye(), -Vector3::UnitY());
        float i_time;
        Point3 i_pt;
        int i_tri;
        if (ray.FastIntersectMesh(ground_.mesh_ptr(), &i_time, &i_pt, &i_tri)) {
            float height = 2.0f + i_pt[1]; // 2 meters above the gound
            cam_.UpdateHeight(height);
        }
        cam_.UpdateSimulation(dt, window());
        
        
        // The billboards also need to be updated to face the current camera
        billboards_.UpdateBillboardRotations(cam_.eye());
    }
}


void HaroldApp::DrawUsingOpenGL() {
    // Clear the screen using the current sky color
    glClearColor(sky_color_[0], sky_color_[1], sky_color_[2], 1);
    
    // Draw the sky strokes
    sky_.Draw(cam_.view_matrix(), proj_matrix_);
    
    // Draw the ground mesh
    ground_.Draw(cam_.view_matrix(), proj_matrix_, ground_color_);
    
    // Draw the billboards
    billboards_.Draw(cam_.view_matrix(), proj_matrix_);

    
    // If we are currently drawing (indicated by the stroke mesh containing >0
    // triangles), then draw the 2D triangle strip mesh for the crayon stroke
    if (stroke2d_mesh_.num_triangles() > 0) {
        stroke2d_shaderprog_.UseProgram();
        stroke2d_shaderprog_.SetUniform("strokeColor", crayon_color_);
        stroke2d_mesh_.Draw();
        stroke2d_shaderprog_.StopProgram();
    }
}
