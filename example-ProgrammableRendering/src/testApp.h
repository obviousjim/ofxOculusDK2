#pragma once

#include "ofMain.h"
#include "ofxOculusDK2.h"

typedef struct{
    ofBoxPrimitive box;
	ofFloatColor color;
	ofVec3f pos;
	ofVec3f floatPos;
    bool bMouseOver;
    bool bGazeOver;
} DemoBox;

class testApp : public ofBaseApp
{
  public:
	
	void setup();
	void update();
	void draw();
	
    void setupBoxes();
    
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

	ofxOculusDK2		oculusRift;

    ofImage ofLogo; // the OF logo
	ofLight light;
	ofEasyCam cam;
	bool showOverlay;

	vector<DemoBox> demos;
    ofShader bshader;
    
    ofVec3f cursor2D;
    ofVec3f cursor3D;
    
    ofVec3f cursorRift;
    ofVec3f demoRift;
    
    ofVec3f cursorGaze;
};
