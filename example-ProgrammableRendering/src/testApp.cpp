#include "testApp.h"


//--------------------------------------------------------------
void testApp::setup()
{
	ofBackground(0);
	ofSetLogLevel( OF_LOG_VERBOSE );
	ofSetVerticalSync( true );
    ofEnableDepthTest();
   
    //    ofSetWindowPosition(1920, 0);
    //    ofToggleFullscreen();
	showOverlay = false;
	predictive = true;
	
	//ofHideCursor();
    
	oculusRift.baseCamera = &cam;
	oculusRift.setup();
	
    // needed for programmable renderer
    ofViewport(ofGetNativeViewport());
    
	for(int i = 0; i < 20; i++){
		DemoSphere d;
		demos.push_back(d);
	}
    setupSpheres();
	
    if (ofIsGLProgrammableRenderer())
        sphereshader.load("Shaders_GL3/simple.vert", "Shaders_GL3/simple.frag");
    
	//enable mouse;
    cam.setAutoDistance(false);
	cam.begin();
	cam.end();
    //cam.setGlobalPosition(0, 1.9, -3);
    //cam.lookAt(ofVec3f(0));
}


//--------------------------------------------------------------
void testApp::update()
{
    for(int i = 0; i < demos.size(); i++){
		demos[i].floatPos.y = 4 * ofSignedNoise(ofGetElapsedTimef()/10.0,
									  demos[i].pos.x/1.0,
									  demos[i].pos.z/1.0,
									  demos[i].radius*1.0) * demos[i].radius*2;
		
	}
    
    if(oculusRift.isSetup()){
        ofRectangle viewport = oculusRift.getOculusViewport();
        for(int i = 0; i < demos.size(); i++){
            // mouse selection
			float mouseDist = oculusRift.distanceFromMouse(demos[i].floatPos);
            demos[i].bMouseOver = (mouseDist < 50);
            
            // gaze selection
            ofVec3f screenPos = oculusRift.worldToScreen(demos[i].floatPos, true);
            float gazeDist = ofDist(screenPos.x, screenPos.y,
                                    viewport.getCenter().x, viewport.getCenter().y);
            demos[i].bGazeOver = (gazeDist < 25);
        }
    }
    
}

void testApp::setupSpheres() {
    
    for(int i = 0; i < demos.size(); i++) {
        demos[i].color = ofFloatColor(ofRandom(1.0),
                                 ofRandom(1.0),
                                 ofRandom(1.0));
        
        demos[i].pos = ofVec3f(ofRandom(-10, 10),0,ofRandom(-10,10));
        
        demos[i].floatPos.x = demos[i].pos.x;
		demos[i].floatPos.z = demos[i].pos.z;
        
        demos[i].radius = 0.5; //ofRandom(0.04, 0.6);
        
        demos[i].bMouseOver = false;
        demos[i].bGazeOver  = false;
	}
}


//--------------------------------------------------------------
void testApp::draw()
{
	if(oculusRift.isSetup()){
/*
		if(0 && showOverlay){
			
			oculusRift.beginOverlay(-230, 320,240);
			ofRectangle overlayRect = oculusRift.getOverlayRectangle();
			
			ofPushStyle();
			ofEnableAlphaBlending();
			ofFill();
			ofSetColor(255, 40, 10, 200);
			
			ofRect(overlayRect);
			
			ofSetColor(255,255);
			ofFill();
			ofDrawBitmapString("ofxOculusRift by\nAndreas Muller\nJames George\nJason Walters\nElie Zananiri\nFPS:"+ofToString(ofGetFrameRate())+"\nPredictive Tracking " + (oculusRift.getUsePredictiveOrientation() ? "YES" : "NO"), 40, 40);
            
            ofSetColor(0, 255, 0);
            ofNoFill();
            ofCircle(overlayRect.getCenter(), 20);
			
			ofPopStyle();
			oculusRift.endOverlay();
		}
 */

        cam.begin();
        cam.end();
        
        ofSetColor(255);
		glEnable(GL_DEPTH_TEST);


		oculusRift.beginLeftEye();
		drawScene();
		oculusRift.endLeftEye();
		
		oculusRift.beginRightEye();
		drawScene();
		oculusRift.endRightEye();
		
		oculusRift.draw();
		
		glDisable(GL_DEPTH_TEST);
        
        //cam.end();
    }
	else{
		cam.begin();
		drawScene();
		cam.end();
	}
	
}

//--------------------------------------------------------------
void testApp::drawScene()
{
	
    //ofPushMatrix();
	//ofRotate(90, 0, 0, -1);
// ofDrawBox(5.0f, 0.1f, 5.0f);
	//ofPopMatrix();

    ofPushMatrix();
	ofRotate(90, 0, 0, -1);
    ofDrawGridPlane(10.0f, 2.0f, false );
	ofPopMatrix();

    
	ofPushStyle();
	//ofNoFill();
	if(ofIsGLProgrammableRenderer()) sphereshader.begin();
    for(int i = 0; i < demos.size(); i++){
		ofPushMatrix();
		ofTranslate(demos[i].floatPos);

        ofFloatColor col;
        /*if (demos[i].bMouseOver)
            col = ofColor::white.getLerped(ofColor::red, sin(ofGetElapsedTimef()*10.0)*.5+.5);
        else if (demos[i].bGazeOver)
            col = ofColor::white.getLerped(ofColor::green, sin(ofGetElapsedTimef()*10.0)*.5+.5);
        else
         */
            col = demos[i].color * demos[i].floatPos.y;

        if(ofIsGLProgrammableRenderer()) {
            sphereshader.setUniform3f("color", col.r, col.g, col.b);
        } else {
            ofSetColor(col);
        }
        
		ofSphere(demos[i].radius);

		ofPopMatrix();
	}
    
    if(ofIsGLProgrammableRenderer()) sphereshader.setUniform3f("color", 0.0, 0.6, 0.3);
    ofSphere(600);
    
    if(ofIsGLProgrammableRenderer()) sphereshader.end();
    
	//billboard and draw the mouse
    
	if(oculusRift.isSetup()){
		
		ofPushMatrix();
		oculusRift.multBillboardMatrix();
		ofSetColor(255, 0, 0);
		ofCircle(0,0,.5);
		ofPopMatrix();

	}
	
	ofPopStyle();
    
}

//--------------------------------------------------------------
void testApp::keyPressed(int key)
{
	if( key == 'f' )
	{
		//gotta toggle full screen for it to be right
		ofToggleFullscreen();
	}
	
	if(key == 's'){
		oculusRift.reloadShader();
	}
	
	if(key == 'l'){
		oculusRift.lockView = !oculusRift.lockView;
	}
	
	if(key == 'o'){
		showOverlay = !showOverlay;
	}
	if(key == 'r'){
		oculusRift.reset();
	}
    if(key == 'z'){
		setupSpheres();
	}
	if(key == 'h'){
		ofHideCursor();
	}
	if(key == 'H'){
		ofShowCursor();
	}
	
	if(key == 'p'){
		predictive = !predictive;
		oculusRift.setUsePredictedOrientation(predictive);
	}
}

//--------------------------------------------------------------
void testApp::keyReleased(int key)
{

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y)
{
 //   cursor2D.set(x, y, cursor2D.z);
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button)
{
//    cursor2D.set(x, y, cursor2D.z);
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button)
{

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button)
{

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h)
{

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg)
{

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo)
{

}
