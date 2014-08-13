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

//#include "../CommonSrc/Platform/Platform_Default.h"
//#include "../CommonSrc/Render/Render_Device.h"
//#include "../CommonSrc/Render/Render_XmlSceneLoader.h"
//#include "../CommonSrc/Platform/Gamepad.h"
//#include "../CommonSrc/Util/OptionMenu.h"
//#include "../CommonSrc/Util/RenderProfiler.h"

#include "Util/Util_Render_Stereo.h"
using namespace OVR::Util::Render;

#include "Sensors/OVR_DeviceConstants.h"

//using namespace OVR::OvrPlatform;
//using namespace OVR::Render;

#ifdef TARGET_WIN32
//#include "
#endif

#include <iostream>

//#define STD_GRAV 9.81 // What SHOULD work with Rift, but off by 1000
//#define STD_GRAV 0.00981  // This gives nice 1.00G on Z with Rift face down !!!

//#define DTR = 0.0174532925f



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
	bool isSetup();
	void reset();
	bool lockView;
    
//	void calculateHmdValues();
//	pRendertargetTexture;
    //Ptr<DistortionRenderer> pRenderer;
	//RenderDevice pRenderer;

	//JG NEW SDK VARS

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
	
	ofRectangle getOculusViewport();


  private:
	bool bSetup;
    
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
   //ovrEyeRenderDesc    eyeRenderDesc[2];
    //Matrix4f            eyeProjection[2];    // Projection matrix for eye.
    //Matrix4f            orthoProjection[2];  // Projection for 2D.
    //ovrPosef            eyeRenderPose[2];    // Poses we used for rendering.
    //ovrTexture          eyeTexture[2];
    //Sizei               eyeRenderSize[2];    // Saved render eye sizes; base for dynamic sizing.
	ovrFovPort			eyeFov[2];
	ovrEyeRenderDesc	eyeRenderDesc[2];
	ovrRecti			eyeRenderViewport[2];
	ovrVector2f			UVScaleOffset[2][2];
	ofVboMesh			eyeMesh[2];

	void initializeClientRenderer();


    Sizei               windowSize;
    
//	Ptr<DeviceManager>	pManager;
//	Ptr<HMDDevice>		pHMD;
//	Ptr<SensorDevice>	pSensor;
//	SensorFusion*       pFusionResult;
//	HMDInfo				hmdInfo;

	OVR::Util::Render::StereoConfig stereo;
	float renderScale;
	ofMesh overlayMesh;
	ofMatrix4x4 orientationMatrix;
	
	ofVboMesh leftEyeMesh;
	ofVboMesh rightEyeMesh;

	ofFbo renderTarget;
    ofFbo backgroundTarget;
	ofFbo overlayTarget;
	ofShader distortionShader;

	void setupEyeParams(ovrEyeType eye);
	void setupShaderUniforms(ovrEyeType eye);
	
	void renderOverlay();
};
