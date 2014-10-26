#include "testApp.h"
#include "ofGLProgrammableRenderer.h"

//--------------------------------------------------------------
int main(){
	// set width, height, mode (OF_WINDOW or OF_FULLSCREEN)
    ofSetCurrentRenderer(ofGLProgrammableRenderer::TYPE);
	ofSetupOpenGL(1024, 768, OF_WINDOW);
	ofRunApp(new testApp()); // start the app
}
