//
//  ofxOculusRift.cpp
//  OculusRiftRendering
//
//  Created by Andreas Müller on 30/04/2013.
//  Updated by James George September 27th 2013
//  Updated by Jason Walters October 22nd 2013
//

#pragma once 

#include "ofMain.h"

//#include "OVR.h"
#include "OVR_Kernel.h"
using namespace OVR;

#include "OVR_CAPI.h"

#include "Util/Util_Render_Stereo.h"
using namespace OVR::Util::Render;

#include "CAPI/CAPI_HMDState.h"
using namespace OVR::CAPI;

#include "Sensors/OVR_DeviceConstants.h"
#include <iostream>



class ofxOculusDK2
{
  public:
		
	ofxOculusDK2();
	~ofxOculusDK2();
	
	//set a pointer to the camera you want as the base perspective
	//the oculus rendering will create a stereo pair from this camera
	//and mix in the head transformation
	ofCamera* baseCamera;
	
	bool setup();
	bool setup(ofFbo::Settings& render_settings);

	bool isSetup();
	void reset();
	bool lockView;

	//draw background, before rendering eyes
    void beginBackground();
    void endBackground();

	//draw overlay, before rendering eyes
	void beginOverlay(float overlayZDistance = -150, float width = 256, float height = 256);
    void endOverlay();

	void beginLeftEye();
	void endLeftEye();
	
	void beginRightEye();
	void endRightEye();
	
	void draw();
    
    void setUsePredictedOrientation(bool usePredicted);
	bool getUsePredictiveOrientation();
	
	void reloadShader();

	ofQuaternion getOrientationQuat();
	ofMatrix4x4 getOrientationMat();
	
	//default 1 has more constrained mouse movement,
	//while turning it up increases the reach of the mouse
	float oculusScreenSpaceScale;
	
	//projects a 3D point into 2D, optionally accounting for the head orientation
	ofVec3f worldToScreen(ofVec3f worldPosition, bool considerHeadOrientation = false);
	ofVec3f screenToWorld(ofVec3f screenPt, bool considerHeadOrientation = false);
    ofVec3f screenToOculus2D(ofVec3f screenPt, bool considerHeadOrientation = false);
	
    //returns a 3d position of the mouse projected in front of the camera, at point z
	ofVec3f mousePosition3D(float z = 0, bool considerHeadOrientation = false);
	
    ofVec2f gazePosition2D();
    
	//sets up the view so that things drawn in 2D are billboarded to the caemra,
	//centered at the mouse
	//Good way to draw custom cursors. don't forget to push/pop matrix around the call
	void multBillboardMatrix();
	void multBillboardMatrix(ofVec3f objectPosition, ofVec3f upDirection = ofVec3f(0,1,0) );

	float distanceFromMouse(ofVec3f worldPoint);
	float distanceFromScreenPoint(ofVec3f worldPoint, ofVec2f screenPoint);
	
	ofRectangle getOverlayRectangle() {
		return ofRectangle(0,0,
						   overlayTarget.getWidth(),
						   overlayTarget.getHeight());
	}
	ofFbo& getOverlayTarget(){
		return overlayTarget;
	}
	ofFbo& getBackgroundTarget(){
		return backgroundTarget;
	}
	ofFbo& getRenderTarget(){
        return renderTarget;
    }

	ofRectangle getOculusViewport();
	bool isHD();
	//allows you to disable moving the camera based on inner ocular distance
	bool applyTranslation;

  private:
	bool bSetup;
    bool insideFrame;
    bool bUsingDebugHmd;
    unsigned startTrackingCaps;
    
    bool bHmdSettingsChanged;
    bool bPositionTrackingEnabled;
    bool bLowPersistence;
    bool bDynamicPrediction;
    
	bool bUsePredictedOrientation;
	bool bUseBackground;
	bool bUseOverlay;
	float overlayZDistance;

    ovrHmd              hmd;
	ovrFovPort			eyeFov[2];
	ovrEyeRenderDesc	eyeRenderDesc[2];
	ovrRecti			eyeRenderViewport[2];
	ovrVector2f			UVScaleOffset[2][2];
	ofVboMesh			eyeMesh[2];
	ovrPosef headPose[2];
	ovrFrameTiming frameTiming;// = ovrHmd_BeginFrameTiming(hmd, 0);

	void initializeClientRenderer();

    Sizei               windowSize;
    
	OVR::Util::Render::StereoConfig stereo;
	float renderScale;
	ofMesh overlayMesh;
	ofMatrix4x4 orientationMatrix;
	
	ofVboMesh leftEyeMesh;
	ofVboMesh rightEyeMesh;

    Sizei renderTargetSize;
	ofFbo renderTarget;
    ofFbo backgroundTarget;
	ofFbo overlayTarget;
	ofShader distortionShader;

	void setupEyeParams(ovrEyeType eye);
	void setupShaderUniforms(ovrEyeType eye);
	
	void renderOverlay();
};
