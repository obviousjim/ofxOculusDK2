#include "testApp.h"
#include "ofGLProgrammableRenderer.h"

//--------------------------------------------------------------
int main(){
	// set width, height, mode (OF_WINDOW or OF_FULLSCREEN)
    ofSetCurrentRenderer(ofGLProgrammableRenderer::TYPE);
	ofSetupOpenGL(1280, 800, OF_WINDOW);
    //ofSetupOpenGL(1920, 1080, OF_FULLSCREEN);
	ofRunApp(new testApp()); // start the app
}
