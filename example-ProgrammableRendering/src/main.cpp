#include "testApp.h"
#include "ofGLProgrammableRenderer.h"

//--------------------------------------------------------------
int main(){
    ofGLWindowSettings settings;
    settings.width = 1280;
    settings.height = 800;
    settings.setGLVersion(4, 1);
    ofCreateWindow(settings);
	ofRunApp(new testApp()); // start the app
}
