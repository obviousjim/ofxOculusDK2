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
//#include "OVR_Kernel.h"
//using namespace OVR;

#include "OVR_CAPI.h"
#include "OVR_CAPI_GL.h"
/*
#include "Util/Util_Render_Stereo.h"
using namespace OVR::Util::Render;

#include "CAPI/CAPI_HMDState.h"
using namespace OVR::CAPI;

#include "Sensors/OVR_DeviceConstants.h"
 */
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
    
    void fullscreenOnRift();
    
	bool setup();
	//bool setup(ofFbo::Settings& render_settings);

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
    void drawSDK();
    
    void dismissSafetyWarning();
    void recenterPose();
    
    float getUserEyeHeight(); // from standing height configured in oculus config user profile
    
    bool getPositionTracking();
    void setPositionTracking(bool state);
    
    bool getNoMirrorToWindow();
    void setNoMirrorToWindow(bool state);
    bool getDisplayOff();
    void setDisplayOff(bool state);
    bool getLowPersistence();
    void setLowPersistence(bool state);
    bool getDynamicPrediction();
    void setDynamicPrediction(bool state);
    bool getNoVsync();
    void setNoVsync(bool state);
    
    bool getTimeWarp();
    void setTimeWarp(bool state);
    bool getVignette();
    void setVignette(bool state);
    bool getSRGB();
    void setSRGB(bool state);
    bool getOverdrive();
    void setOverdrive(bool state);
    bool getHqDistortion();
    void setHqDistortion(bool state);
    bool getTimewarpJitDelay();
    void setTimewarpJitDelay(bool state);

    float getPixelDensity();
    void setPixelDensity(float density);
    
	void reloadShader();

	ofQuaternion getOrientationQuat();
	ofMatrix4x4 getOrientationMat();
    ofVec3f getTranslation();
	
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
    bool bRenderTargetSizeChanged;

    bool bPositionTracking;
    
    // hmd capabilities
    bool bNoMirrorToWindow; // direct mode only - ExtendDesktop is off
    bool bDisplayOff; // direct mode only - ExtendDesktop is off
    bool bLowPersistence;
    bool bDynamicPrediction;
    bool bNoVsync;
    
    // distortion caps
    bool bTimeWarp;
    bool bVignette;
    bool bSRGB;
    bool bOverdrive;
    bool bHqDistortion;
    bool bTimewarpJitDelay;
    
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
    unsigned int frameIndex;
    
    ovrTexture          EyeTexture[2];
    
    ovrVector3f hmdToEyeViewOffsets[2];

	void initializeClientRenderer();

    ovrSizei               windowSize;
    
	ofMesh overlayMesh;
	ofMatrix4x4 orientationMatrix;
	
	ofVboMesh leftEyeMesh;
	ofVboMesh rightEyeMesh;

    ofFbo::Settings renderTargetFboSettings();
    
    float pixelDensity;
    ovrSizei renderTargetSize;
	ofFbo renderTarget;
    ofFbo backgroundTarget;
	ofFbo overlayTarget;
	ofShader distortionShader;
    
    ofShader debugShader;   // XXX mattebb
    ofMesh debugMesh;
    ofImage debugImage;
    
    void setDistortionCap(unsigned int cap, bool state);
    bool getDistortionCap(unsigned int cap);
    void setHmdCap(unsigned int cap, bool state);
    bool getHmdCap(unsigned int cap);
    
	void setupEyeParams(ovrEyeType eye);
	void setupShaderUniforms(ovrEyeType eye);
    
    ofMatrix4x4 getProjectionMatrix(ovrEyeType eye);
    ofMatrix4x4 getViewMatrix(ovrEyeType eye);
	
	void renderOverlay();

    void updateHmdSettings();
    unsigned int setupDistortionCaps();
    unsigned int setupHmdCaps();
};
