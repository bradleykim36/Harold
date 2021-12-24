#ifndef HAROLD_APP_H_
#define HAROLD_APP_H_

#include <mingfx.h>
using namespace mingfx;

#include "billboards.h"
#include "edge_mesh.h"
#include "ground.h"
#include "sky.h"

#include <string>
#include <vector>


/** Main application class for the Harold assignment, which is a partial
 implementation of the Cohen et al. paper.
 */
class HaroldApp : public GraphicsApp {
public:
    
    HaroldApp();
    virtual ~HaroldApp();
    
    // These are used for drawing
    void OnLeftMouseDown(const Point2 &pos);
    void OnLeftMouseDrag(const Point2 &pos, const Vector2 &delta);
    void OnLeftMouseUp(const Point2 &pos);
    
    // These are used for walking around and looking (i.e., camera movement)
    void OnRightMouseDrag(const Point2 &pos, const Vector2 &delta);
    void UpdateSimulation(double dt);
    
    // Adds the 2D point to stroke2d_, which holds the centerline of the user's
    // drawn stroke.  Also updates stroke2d_mesh_, which is a 2D triangle strip
    // used to actually draw the stroke on the screen.
    void AddToStroke(const Point2 &normalized_screen_pt);

    void InitNanoGUI();
    void InitOpenGL();
    
    void DrawUsingOpenGL();
    
private:
    
    // user input from the mouse is interpreted differently based upon where the
    // stroke originates and where it ends.  we can model this as a simple state
    // machine
    enum DrawingState {
        // left mouse button is up, not currently drawing
        DRAWING_NONE,
        
        // stroke started in the sky, currently adding a sky stroke
        DRAWING_SKY,
        
        // stroke started on a billboard, currently editing that billboard
        DRAWING_BILLBOARD_EDIT,

        // temporary state for between mouse down and mouse up -- here, we know
        // the stroke started on the ground, but not yet sure if it will end on
        // the ground or in the sky
        DRAWING_GROUND_OR_BILLBOARD,
 
        // stroke ended on the ground, currently editing the ground
        DRAWING_GROUND,
        
        // stroke ended in the sky, currently creating a new billboard
        DRAWING_BILLBOARD,
    };
    DrawingState drawing_state_;
    
    // the centerline of the stroke dragged out by the mouse on the screen
    std::vector<Point2> stroke2d_;

    // a 2d triangle strip mesh for drawing the stroke on the filmplane
    Mesh stroke2d_mesh_;
    
    // The ground mesh and functionality to draw and edit the ground.
    Ground ground_;
    
    // Sky strokes and functionality to add more strokes to the sky
    Sky sky_;
    
    // Billboard strokes and functionality to add more billboards, including
    // editing them by adding on to an existing billboard
    Billboards billboards_;
    
    // when in the DRAWING_BILLBOARD_EDIT state, this holds the id of the
    // billboard that is currently being edited.
    int edit_billboard_id_;
    

    // Stroke shaders do not use any lighting, just transform vertices and then
    // render the triangle strip for the stroke in the appropriate color. The
    // 2D shader program is used in this class.  The 3D is created here so that
    // there is only one copy for the whole application, but it is used both the
    // sky and billboard classes.
    ShaderProgram stroke2d_shaderprog_;
    ShaderProgram stroke3d_shaderprog_;

    // This implements something like the camera controls in minecraft.  You can
    // walk around using the arrow keys or ASWZ keys.  You can also move your
    // head to look around by dragging with the right mouse button.  Internally,
    // the camera creates a view matrix.
    CraftCam cam_;
    Matrix4 proj_matrix_;
    
    // A list of paths to search for data files (images and shaders)
    std::vector<std::string> search_path_;
    
    // Current colors set by the GUI
    Color sky_color_;
    Color ground_color_;
    Color crayon_color_;
};

#endif
