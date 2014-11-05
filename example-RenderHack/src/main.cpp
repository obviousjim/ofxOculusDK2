#include "ofAppGLFWWindow.h"
#include "testApp.h"

//--------------------------------------------------------------
int main(){

	int x = 0;
	int y = 0;

    bool debug = false;

	ovr_Initialize();
 
    hmd = ovrHmd_Create(0);
 
    if (hmd == NULL)
    {
            hmd = ovrHmd_CreateDebug(ovrHmd_DK1);
 
            debug = true;
    }
 
    if (debug == false)
    {
            x = hmd->WindowsPos.x;
            y = hmd->WindowsPos.y;
            //flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
 
    int w = hmd->Resolution.w;
    int h = hmd->Resolution.h;

	// set width, height, mode (OF_WINDOW or OF_FULLSCREEN)
	//ofSetCurrentRenderer(ofGLProgrammableRenderer::TYPE);

	//ofAppGLFWWindow window;
	//window.setDoubleBuffering(false);
	//ofSetupOpenGL(&window, w, h, OF_WINDOW);
	//ofSetWindowPosition(x, y);
	ofSetupOpenGL(w, h, OF_WINDOW);

	ofRunApp(new testApp()); // start the app

	ovrHmd_Destroy(hmd);
	ovr_Shutdown();
}
