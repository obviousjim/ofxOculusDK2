#pragma once

#include "ofMain.h"

#if defined(TARGET_OSX)
    #define OVR_OS_MAC
#elif defined(TARGET_WIN32)
    #define OVR_OS_WIN32
#endif

#include "OVR_CAPI_GL.h"
#include "Kernel/OVR_Math.h"
using namespace OVR;

//#include "ofxOculusDK2.h"

extern ovrHmd hmd;
extern GLuint frameBuffer;
extern GLuint MVPMatrixLocation;
extern GLuint positionLocation;
extern ovrEyeRenderDesc eyeRenderDesc[2];
extern ovrRecti eyeRenderViewport[2];
extern GLuint vertexShader;
extern GLuint vertexArray;
extern ovrGLTexture eyeTexture[2];

typedef struct{
	ofColor color;
	ofVec3f pos;
	ofVec3f floatPos;
	float radius;
    bool bMouseOver;
    bool bGazeOver;
} DemoSphere;

class testApp : public ofBaseApp
{
  public:
	
	void setup();
	void exit();

	void update();
	void draw();
	
	void drawScene();
	
	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

	//ofxOculusDK2		oculusRift;

	ofLight				light;
	ofEasyCam			cam;
	bool showOverlay;
	bool predictive;
	vector<DemoSphere> demos;
    
    ofVec3f cursor2D;
    ofVec3f cursor3D;
    
    ofVec3f cursorRift;
    ofVec3f demoRift;
    
    ofVec3f cursorGaze;
};
