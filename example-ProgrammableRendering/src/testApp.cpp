#include "testApp.h"


//--------------------------------------------------------------
void testApp::setup()
{
	ofSetLogLevel( OF_LOG_VERBOSE );
    ofEnableDepthTest();
	ofSetVerticalSync( false );

	showOverlay = false;
	
	oculusRift.baseCamera = &cam;
	oculusRift.setup();
    oculusRift.fullscreenOnRift();
    
    // needed for programmable renderer -- 0.5.0.1 SDK no longer needed?
    //ofViewport(ofGetNativeViewport());
    
	for(int i = 0; i < 80; i++){
		DemoBox d;
		demos.push_back(d);
	}
    setupBoxes();
	
    if (ofIsGLProgrammableRenderer())
        bshader.load("Shaders_GL3/simple.vert", "Shaders_GL3/simple.frag");
    
    // ofBox uses texture coordinates from 0-1, so you can load whatever
    // sized images you want and still use them to texture your box
    // but we have to explicitly normalize our tex coords here
    ofEnableNormalizedTexCoords();
    
    // loads the OF logo from disk
    ofLogo.loadImage("of.png");
    
	//enable mouse;
    cam.setAutoDistance(false);
	cam.begin();
	cam.end();
    
    // set camera y to user eye height
    cam.setGlobalPosition(0, oculusRift.getUserEyeHeight(), 3);
}


//--------------------------------------------------------------
void testApp::update()
{
    for(int i = 0; i < demos.size(); i++){
        demos[i].floatPos.y = 4 * ofSignedNoise(ofGetElapsedTimef()/10.0,
                                                demos[i].pos.x/1.0,
                                                demos[i].pos.y/1.0,
                                                demos[i].pos.z/1.0);
        
    }
    
    if(oculusRift.isSetup()){
        ofRectangle viewport = oculusRift.getOculusViewport();
        for(int i = 0; i < demos.size(); i++){
            // mouse selection
			float mouseDist = oculusRift.distanceFromMouse(demos[i].floatPos);
            demos[i].bMouseOver = (mouseDist < 100);
            
            // gaze selection
            ofVec3f screenPos = oculusRift.worldToScreen(demos[i].floatPos, true);
            float gazeDist = ofDist(screenPos.x, screenPos.y,
                                    viewport.getCenter().x, viewport.getCenter().y);
            demos[i].bGazeOver = (gazeDist < 25);
        }
    }
    
}

void testApp::setupBoxes() {
    
    for(int i = 0; i < demos.size(); i++) {
        demos[i].color = ofFloatColor(ofRandom(1.0),
                                 ofRandom(1.0),
                                 ofRandom(1.0));
        
        demos[i].pos = ofVec3f(ofRandom(-8, 8),0,ofRandom(-8,8));
        
        demos[i].box = ofBoxPrimitive();
        demos[i].box.set(ofRandom(0.2,0.8));
        demos[i].box.enableTextures();
        
        demos[i].floatPos.x = demos[i].pos.x;
		demos[i].floatPos.z = demos[i].pos.z;
        
        demos[i].bMouseOver = false;
        demos[i].bGazeOver  = false;
	}
}


//--------------------------------------------------------------
void testApp::draw()
{
	if(oculusRift.isSetup()){
		if(showOverlay){
			
			oculusRift.beginOverlay(-230, 320,240);
			ofRectangle overlayRect = oculusRift.getOverlayRectangle();
			
			ofPushStyle();
			ofEnableAlphaBlending();
			ofFill();
			ofSetColor(255, 40, 10, 200);
			
			ofRect(overlayRect);
			
			ofSetColor(255,255);
			ofFill();
		  
            ofSetColor(0, 255, 0);
            ofNoFill();
            ofCircle(overlayRect.getCenter(), 20);
			
			ofPopStyle();
			oculusRift.endOverlay();
		}
        
		oculusRift.beginLeftEye();
		drawScene();
		oculusRift.endLeftEye();
		
		oculusRift.beginRightEye();
		drawScene();
		oculusRift.endRightEye();
		
        oculusRift.draw();
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

    ofPushMatrix();
    ofRotate(90, 0, 0, -1);
    ofSetColor(30);
    ofDrawGridPlane(12.0f, 8.0f, false );
	ofPopMatrix();

    if(ofIsGLProgrammableRenderer()) bshader.begin();
    ofLogo.getTextureReference().bind();

    for(int i = 0; i < demos.size(); i++){
		ofPushMatrix();
		ofTranslate(demos[i].floatPos);

        ofFloatColor col;
        if (demos[i].bMouseOver)
            col = ofColor::white;
        else
            col = demos[i].color;
        
        if(ofIsGLProgrammableRenderer()) {
            bshader.setUniform3f("color", col.r, col.g, col.b);
        } else {
            ofSetColor(col);
        }
        
        demos[i].box.draw();
        
		ofPopMatrix();
	}
    
    ofLogo.getTextureReference().unbind();
    if(ofIsGLProgrammableRenderer()) bshader.end();
    
	//billboard and draw the mouse
    
	if(oculusRift.isSetup()){
		
		ofPushMatrix();
		oculusRift.multBillboardMatrix();
		ofSetColor(255, 0, 0);
		ofCircle(0,0,.5);
		ofPopMatrix();

	}
    

}

//--------------------------------------------------------------
void testApp::keyPressed(int key)
{
    // dismiss safety warning on any key
    oculusRift.dismissSafetyWarning();
    
	if( key == 'f' )
	{
		//gotta toggle full screen for it to be right
		ofToggleFullscreen();
	}
    
	
    if(key == 'a'){
        cout << "FPS " << ofGetFrameRate() << " TARGET " << ofGetTargetFrameRate() << endl;
    }
    
    
	if(key == 's'){
		oculusRift.reloadShader();
	}
    if(key == 'v'){
        oculusRift.setVignette( !oculusRift.getVignette() );
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
    if(key == 'c'){
        oculusRift.recenterPose();
    }
    if(key == 'z'){
		setupBoxes();
	}
	if(key == 'h'){
		ofHideCursor();
	}
	if(key == 'H'){
		ofShowCursor();
	}
	
    if(key == 'd'){
        oculusRift.setPixelDensity( oculusRift.getPixelDensity()-0.1 );
    }
    if(key == 'D'){
        oculusRift.setPixelDensity( oculusRift.getPixelDensity()+0.1 );
    }

    if(key == 'p'){
        oculusRift.setPositionTracking( !oculusRift.getPositionTracking() );
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
