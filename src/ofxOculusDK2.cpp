//
//  ofxOculusRift.cpp
//  OculusRiftRendering
//
//  Created by Andreas Müller on 30/04/2013.
//  Updated by James George September 27th 2013
//  Updated by Jason Walters October 22 2013
//

#include "ofxOculusDK2.h"

#define GLSL(version, shader)  "#version " #version "\n#extension GL_ARB_texture_rectangle : enable\n" #shader
static const char* OculusWarpVert = GLSL(120,
//uniform vec2 dimensions;
//varying vec2 oTexCoord;
// 
//void main()
//{
//	oTexCoord = gl_MultiTexCoord0.xy / dimensions;
//	gl_FrontColor = gl_Color;
//	gl_Position = ftransform();
//});
uniform mat4 View;
uniform mat4 Texm;

attribute vec4 Position;
attribute vec2 TexCoord;

varying vec2 oTexCoord;

void main()
{
    gl_Position = View * Position;
    oTexCoord = vec2(Texm * vec4(TexCoord,0,1));
}
);
                                         
static const char* OculusWarpFrag = GLSL(120,
//uniform vec2 LensCenter;
//uniform vec2 ScreenCenter;
//uniform vec2 Scale;
//uniform vec2 ScaleIn;
//uniform vec4 HmdWarpParam;
//uniform vec4 ChromAbParam;
//uniform sampler2DRect Texture0;
//uniform vec2 dimensions;
//varying vec2 oTexCoord;
//
//void main()
//{
//	vec2  theta = (oTexCoord - LensCenter) * ScaleIn; // Scales to [-1, 1]
//	float rSq = theta.x * theta.x + theta.y * theta.y;
//    vec2  theta1 = theta * (HmdWarpParam.x +
//                            HmdWarpParam.y * rSq +
//							HmdWarpParam.z * rSq * rSq +
//                            HmdWarpParam.w * rSq * rSq * rSq);
//    
//    // Detect whether blue texture coordinates are out of range
//    // since these will scale out the furthest.
//    vec2 thetaBlue = theta1 * (ChromAbParam.z + ChromAbParam.w * rSq);
//    vec2 tcBlue = LensCenter + Scale * thetaBlue;
//    if (!all(equal(clamp(tcBlue, ScreenCenter - vec2(0.25, 0.5), ScreenCenter + vec2(0.25, 0.5)), tcBlue))) {
//        gl_FragColor = vec4(0);
//    }
//    else {
//        // Now do blue texture lookup.
//        float blue = texture2DRect(Texture0, tcBlue * dimensions).b;
//        
//        // Do green lookup (no scaling).
//        vec2 tcGreen = LensCenter + Scale * theta1;
//        vec4 center = texture2DRect(Texture0, tcGreen * dimensions);
//        
//        // Do red scale and lookup.
//        vec2 thetaRed = theta1 * (ChromAbParam.x + ChromAbParam.y * rSq);
//        vec2 tcRed = LensCenter + Scale * thetaRed;
//        float red = texture2DRect(Texture0, tcRed * dimensions).r;
//        
//        gl_FragColor = vec4(red, center.g, blue, center.a) * gl_Color;
//    }
//}
uniform sampler2D Texture;
uniform vec3 DistortionClearColor;
uniform float EdgeFadeScale;
uniform vec2 EyeToSourceUVScale;
uniform vec2 EyeToSourceUVOffset;
uniform vec2 EyeToSourceNDCScale;
uniform vec2 EyeToSourceNDCOffset;
uniform vec2 TanEyeAngleScale;
uniform vec2 TanEyeAngleOffset;
uniform vec4 HmdWarpParam;
uniform vec4 ChromAbParam;

varying vec4 oPosition;
varying vec2 oTexCoord;

void main()
{
    // Input oTexCoord is [-1,1] across the half of the screen used for a single eye.
    vec2 TanEyeAngleDistorted = oTexCoord * TanEyeAngleScale + TanEyeAngleOffset;  // Scales to tan(thetaX),tan(thetaY), but still distorted (i.e. only the center is correct)
    float  RadiusSq = TanEyeAngleDistorted.x * TanEyeAngleDistorted.x + TanEyeAngleDistorted.y * TanEyeAngleDistorted.y;
    float Distort = 1.0 / ( 1.0 + RadiusSq * ( HmdWarpParam.y + RadiusSq * ( HmdWarpParam.z + RadiusSq * ( HmdWarpParam.w ) ) ) );
    float DistortR = Distort * ( ChromAbParam.x + RadiusSq * ChromAbParam.y );
    float DistortG = Distort;
    float DistortB = Distort * ( ChromAbParam.z + RadiusSq * ChromAbParam.w );
    vec2 TanEyeAngleR = DistortR * TanEyeAngleDistorted;
    vec2 TanEyeAngleG = DistortG * TanEyeAngleDistorted;
    vec2 TanEyeAngleB = DistortB * TanEyeAngleDistorted;

    // These are now in "TanEyeAngle" space.
    // The vectors (TanEyeAngleRGB.x, TanEyeAngleRGB.y, 1.0) are real-world vectors pointing from the eye to where the components of the pixel appear to be.
    // If you had a raytracer, you could just use them directly.

    // Scale them into ([0,0.5],[0,1]) or ([0.5,0],[0,1]) UV lookup space (depending on eye)
    vec2 SourceCoordR = TanEyeAngleR * EyeToSourceUVScale + EyeToSourceUVOffset;
        SourceCoordR.y = 1.0 - SourceCoordR.y;
    vec2 SourceCoordG = TanEyeAngleG * EyeToSourceUVScale + EyeToSourceUVOffset;
        SourceCoordG.y = 1.0 - SourceCoordG.y;
    vec2 SourceCoordB = TanEyeAngleB * EyeToSourceUVScale + EyeToSourceUVOffset;
        SourceCoordB.y = 1.0 - SourceCoordB.y;

    // Find the distance to the nearest edge.
    vec2 NDCCoord = TanEyeAngleG * EyeToSourceNDCScale + EyeToSourceNDCOffset;
    float EdgeFadeIn = clamp ( EdgeFadeScale, 0.0, 1e5 ) * ( 1.0 - max ( abs ( NDCCoord.x ), abs ( NDCCoord.y ) ) );
    if ( EdgeFadeIn < 0.0 )
    {
        gl_FragColor = vec4(DistortionClearColor.r, DistortionClearColor.g, DistortionClearColor.b, 1.0);
        return;
    }
    EdgeFadeIn = clamp ( EdgeFadeIn, 0.0, 1.0 );

    // Actually do the lookups.
    float ResultR = texture2D(Texture, SourceCoordR).r;
    float ResultG = texture2D(Texture, SourceCoordG).g;
    float ResultB = texture2D(Texture, SourceCoordB).b;

    gl_FragColor = vec4(ResultR * EdgeFadeIn, ResultG * EdgeFadeIn, ResultB * EdgeFadeIn, 1.0);
}
);

ofQuaternion toOf(const Quatf& q){
	return ofQuaternion(q.x, q.y, q.z, q.w);
}

ofMatrix4x4 toOf(const Matrix4f& m){
	return ofMatrix4x4(m.M[0][0],m.M[1][0],m.M[2][0],m.M[3][0],
					   m.M[0][1],m.M[1][1],m.M[2][1],m.M[3][1],
					   m.M[0][2],m.M[1][2],m.M[2][2],m.M[3][2],
					   m.M[0][3],m.M[1][3],m.M[2][3],m.M[3][3]);
}

Matrix4f toOVR(const ofMatrix4x4& m){
	const float* cm = m.getPtr();
	return Matrix4f(cm[ 0],cm[ 1],cm[ 2],cm[ 3],
					cm[ 4],cm[ 5],cm[ 6],cm[ 7],
					cm[ 8],cm[ 9],cm[10],cm[11],
					cm[12],cm[13],cm[14],cm[15]);
}

ofRectangle toOf(const ovrRecti vp){
	return ofRectangle(vp.Pos.x,vp.Pos.y,vp.Size.w,vp.Size.h);
}

ofxOculusDK2::ofxOculusDK2(){
    hmd = 0;
    
    bUsingDebugHmd = false;
    startTrackingCaps = 0;
    
	bHmdSettingsChanged = false;
    bPositionTrackingEnabled = true;
    bLowPersistence = true;
    bDynamicPrediction = true;
    
    baseCamera = NULL;
	bSetup = false;
	lockView = false;
	bUsePredictedOrientation = true;
	bUseOverlay = false;
	bUseBackground = false;
	overlayZDistance = -200;
	oculusScreenSpaceScale = 2;
}

ofxOculusDK2::~ofxOculusDK2(){
	if(bSetup){
//		pSensor.Clear();
//        pHMD.Clear();
//		pManager.Clear();
//        
//        delete pFusionResult;
//                
//		System::Destroy();
        
        if (hmd) {
            ovrHmd_Destroy(hmd);
            hmd = 0;
        }
        
        ovr_Shutdown();
        
		bSetup = false;
	}
}


bool ofxOculusDK2::setup(){
	
	if(bSetup){
		ofLogError("ofxOculusDK2::setup") << "Already set up";
		return false;
	}
	
//	System::Init();
//    
//    pFusionResult = new SensorFusion();
//	pManager = *DeviceManager::Create();
//	pHMD = *pManager->EnumerateDevices<HMDDevice>().CreateDevice();
//    
//	if (pHMD == NULL){
//		ofLogError("ofxOculusRift::setup") << "HMD not found";
//		return false;
//	}
//	
//	if(!pHMD->GetDeviceInfo(&hmdInfo)){
//		ofLogError("ofxOculusRift::setup") << "HMD Info not loaded";
//		return false;
//	}
//	
//	pSensor = *pHMD->GetSensor();
//	if (pSensor == NULL){
//		ofLogError("ofxOculusRift::setup") << "No sensor returned";
//		return false;
//	}
//	ovr_Initialize();
//	if(!pFusionResult->AttachToSensor(pSensor)){
//		ofLogError("ofxOculusRift::setup") << "Sensor Fusion failed";
//		return false;
//	}
    
    // Oculus HMD & Sensor Initialization
    
    ovr_Initialize();
    
	hmd = ovrHmd_Create(0);
    
	if(!hmd){
		// If we didn't detect an Hmd, create a simulated one for debugging.
		hmd = ovrHmd_CreateDebug(ovrHmd_DK1);
		if (!hmd) {   // Failed Hmd creation.
            ofLogError("ofxOculusDK2::setup") << "HMD not found";
			return false;
		}
        else {
            ofLogNotice("ofxOculusDK2::setup") << "HMD not found, creating simulated device.";
            bUsingDebugHmd = true;
        }
	}
    
    if (hmd->HmdCaps & ovrHmdCap_ExtendDesktop) {
        windowSize = hmd->Resolution;
    }
    else {
        // In Direct App-rendered mode, we can use smaller window size,
        // as it can have its own contents and isn't tied to the buffer.
        windowSize = Sizei(ofGetWidth(), ofGetHeight()); //Sizei(960, 540); avoid rotated output bug.
    }
    
    // Configure HMD Stereo Settings
    //calculateHmdValues();

	// Start the sensor which provides the Rift’s pose and motion.
	ovrHmd_ConfigureTracking(hmd, 
		ovrTrackingCap_Orientation | 
		ovrTrackingCap_MagYawCorrection | 
		ovrTrackingCap_Position, 0);
	// Query the HMD for the current tracking state.

    // Setup System Window & Rendering
    
//	stereo.SetFullViewport(OVR::Util::Render::Viewport(0,0, hmdInfo.HResolution, hmdInfo.VResolution));
//	stereo.SetStereoMode(OVR::Util::Render::Stereo_LeftRight_Multipass);
//	stereo.SetHMDInfo(hmdInfo);
//    if (hmdInfo.HScreenSize > 0.0f)
//    {
//        if (hmdInfo.HScreenSize > 0.140f) // 7"
//            stereo.SetDistortionFitPointVP(-1.0f, 0.0f);
//        else
//            stereo.SetDistortionFitPointVP(0.0f, 1.0f);
//    }
//
//	renderScale = stereo.GetDistortionScale();
	
	//account for render scale?
	float w = windowSize.w;
	float h = windowSize.h;
	
	renderTarget.allocate(w, h, GL_RGB);
    backgroundTarget.allocate(w/2, h);
//    overlayTarget.allocate(256, 256, GL_RGBA);
	
	backgroundTarget.begin();
    ofClear(0.0, 0.0, 0.0);
	backgroundTarget.end();
	
//	overlayTarget.begin();
//	ofClear(0.0, 0.0, 0.0);
//	overlayTarget.end();
	
	//left eye
	leftEyeMesh.addVertex(ofVec3f(0,0,0));
	leftEyeMesh.addTexCoord(ofVec2f(0,h));
	
	leftEyeMesh.addVertex(ofVec3f(0,h,0));
	leftEyeMesh.addTexCoord(ofVec2f(0,0));
	
	leftEyeMesh.addVertex(ofVec3f(w/2,0,0));
	leftEyeMesh.addTexCoord(ofVec2f(w/2,h));

	leftEyeMesh.addVertex(ofVec3f(w/2,h,0));
	leftEyeMesh.addTexCoord(ofVec2f(w/2,0));
	
	leftEyeMesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
	
	//Right eye
	rightEyeMesh.addVertex(ofVec3f(w/2,0,0));
	rightEyeMesh.addTexCoord(ofVec2f(w/2,h));

	rightEyeMesh.addVertex(ofVec3f(w/2,h,0));
	rightEyeMesh.addTexCoord(ofVec2f(w/2,0));

	rightEyeMesh.addVertex(ofVec3f(w,0,0));
	rightEyeMesh.addTexCoord(ofVec2f(w,h));
	
	rightEyeMesh.addVertex(ofVec3f(w,h,0));
	rightEyeMesh.addTexCoord(ofVec2f(w,0));
	
	rightEyeMesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
    
    bPositionTrackingEnabled = (hmd->TrackingCaps & ovrTrackingCap_Position)? true : false;
    
	reloadShader();
	
	bSetup = true;
	return true;
}

bool ofxOculusDK2::isSetup(){
	return bSetup;
}

void ofxOculusDK2::reset(){
	if(bSetup){
//		pFusionResult->Reset();
	}
}

void ofxOculusDK2::calculateHmdValues()
{
    // Initialize eye rendering information for ovrHmd_Configure.
    // The viewport sizes are re-computed in case RenderTargetSize changed due to HW limitations.
    ovrFovPort eyeFov[2];
    eyeFov[0] = hmd->DefaultEyeFov[0];
    eyeFov[1] = hmd->DefaultEyeFov[1];
    
    // Clamp Fov based on our dynamically adjustable FovSideTanMax.
    // Most apps should use the default, but reducing Fov does reduce rendering cost.
    static const float fovSideTanMax = 1.0f;
    eyeFov[0] = FovPort::Min(eyeFov[0], FovPort(fovSideTanMax));
    eyeFov[1] = FovPort::Min(eyeFov[1], FovPort(fovSideTanMax));
    
    // Configure Stereo settings. Default pixel density is 1.0f.
    static const float desiredPixelDensity = 1.0f;
    Sizei recommenedTex0Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Left,  eyeFov[0], desiredPixelDensity);
    Sizei recommenedTex1Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Right, eyeFov[1], desiredPixelDensity);
        
//    if (RendertargetIsSharedByBothEyes) {
    if (true) {
            Sizei rtSize(recommenedTex0Size.w + recommenedTex1Size.w,
                         Alg::Max(recommenedTex0Size.h, recommenedTex1Size.h));
            
            // Use returned size as the actual RT size may be different due to HW limits.
//            rtSize = EnsureRendertargetAtLeastThisBig(Rendertarget_BothEyes, rtSize);
        
            // Don't draw more then recommended size; this also ensures that resolution reported
            // in the overlay HUD size is updated correctly for FOV/pixel density change.
            eyeRenderSize[0] = Sizei::Min(Sizei(rtSize.w/2, rtSize.h), recommenedTex0Size);
            eyeRenderSize[1] = Sizei::Min(Sizei(rtSize.w/2, rtSize.h), recommenedTex1Size);
            
            // Store texture pointers that will be passed for rendering.
            // Same texture is used, but with different viewports.
//            eyeTexture[0]                       = renderTarget.getTextureReference().getTextureData().textureID;
            eyeTexture[0].Header.TextureSize    = rtSize;
            eyeTexture[0].Header.RenderViewport = Recti(Vector2i(0), eyeRenderSize[0]);
//            eyeTexture[1]                       = RenderTargets[Rendertarget_BothEyes].Tex;
            eyeTexture[1].Header.TextureSize    = rtSize;
            eyeTexture[1].Header.RenderViewport = Recti(Vector2i((rtSize.w+1)/2, 0), eyeRenderSize[1]);
    }
//    else {
//        Sizei tex0Size = EnsureRendertargetAtLeastThisBig(Rendertarget_Left,  recommenedTex0Size);
//        Sizei tex1Size = EnsureRendertargetAtLeastThisBig(Rendertarget_Right, recommenedTex1Size);
//        
//        EyeRenderSize[0] = Sizei::Min(tex0Size, recommenedTex0Size);
//        EyeRenderSize[1] = Sizei::Min(tex1Size, recommenedTex1Size);
//        
//        // Store texture pointers and viewports that will be passed for rendering.
//        EyeTexture[0]                       = RenderTargets[Rendertarget_Left].Tex;
//        EyeTexture[0].Header.TextureSize    = tex0Size;
//        EyeTexture[0].Header.RenderViewport = Recti(EyeRenderSize[0]);
//        EyeTexture[1]                       = RenderTargets[Rendertarget_Right].Tex;
//        EyeTexture[1].Header.TextureSize    = tex1Size;
//        EyeTexture[1].Header.RenderViewport = Recti(EyeRenderSize[1]);
//    }
    
    // Hmd caps.
//    unsigned hmdCaps = (VsyncEnabled ? 0 : ovrHmdCap_NoVSync);
    unsigned hmdCaps = 0;  // EZ: Assume vsync is on.
    if (bLowPersistence) {
        hmdCaps |= ovrHmdCap_LowPersistence;
    }
    
    // ovrHmdCap_DynamicPrediction - enables internal latency feedback
    if (bDynamicPrediction) {
        hmdCaps |= ovrHmdCap_DynamicPrediction;
    }
    
    // ovrHmdCap_DisplayOff - turns off the display
//    if (DisplaySleep)
//        hmdCaps |= ovrHmdCap_DisplayOff;
    
//    if (!MirrorToWindow)
//        hmdCaps |= ovrHmdCap_NoMirrorToWindow;
    
    // If using our driver, display status overlay messages.
//    if (!(Hmd->HmdCaps & ovrHmdCap_ExtendDesktop) && (NotificationTimeout != 0.0f))
//    {
//        GetPlatformCore()->SetNotificationOverlay(0, 28, 8,
//                                                  "Rendering to the Hmd - Please put on your Rift");
//        GetPlatformCore()->SetNotificationOverlay(1, 24, -8,
//                                                  MirrorToWindow ? "'M' - Mirror to Window [On]" : "'M' - Mirror to Window [Off]");
//    }
    
    // EZ: Let's render manually for now...
//    ovrHmd_SetEnabledCaps(hmd, hmdCaps);
//    
//    
//	ovrRenderAPIConfig config         = pRender->Get_ovrRenderAPIConfig();
//    unsigned           distortionCaps = ovrDistortionCap_Chromatic |
//    ovrDistortionCap_Vignette;
//    if (SupportsSrgb)
//        distortionCaps |= ovrDistortionCap_SRGB;
//	if(PixelLuminanceOverdrive)
//		distortionCaps |= ovrDistortionCap_Overdrive;
//    if (TimewarpEnabled)
//        distortionCaps |= ovrDistortionCap_TimeWarp;
//    if(TimewarpNoJitEnabled)
//        distortionCaps |= ovrDistortionCap_ProfileNoTimewarpSpinWaits;
//    
//    if (!ovrHmd_ConfigureRendering( Hmd, &config, distortionCaps,
//                                   eyeFov, EyeRenderDesc ))
//    {
//        // Fail exit? TBD
//        return;
//    }
    
//    if (ForceZeroIpd)
//    {
//        // Remove IPD adjust
//        EyeRenderDesc[0].ViewAdjust = Vector3f(0);
//        EyeRenderDesc[1].ViewAdjust = Vector3f(0);
//    }
    
    unsigned sensorCaps = ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection;
    if (bPositionTrackingEnabled) {
        sensorCaps |= ovrTrackingCap_Position;
    }
    
    if (startTrackingCaps != sensorCaps) {
        ovrHmd_ConfigureTracking(hmd, sensorCaps, 0);
        startTrackingCaps = sensorCaps;
    }
    
    // EZ: eyeRenderDesc is never set...
    
    // Calculate projections
    eyeProjection[0] = ovrMatrix4f_Projection(eyeRenderDesc[0].Fov,  0.01f, 10000.0f, true);
    eyeProjection[1] = ovrMatrix4f_Projection(eyeRenderDesc[1].Fov,  0.01f, 10000.0f, true);
    
    float orthoDistance = 0.8f; // 2D is 0.8 meter from camera
    Vector2f orthoScale0   = Vector2f(1.0f) / Vector2f(eyeRenderDesc[0].PixelsPerTanAngleAtCenter);
    Vector2f orthoScale1   = Vector2f(1.0f) / Vector2f(eyeRenderDesc[1].PixelsPerTanAngleAtCenter);
    
    orthoProjection[0] = ovrMatrix4f_OrthoSubProjection(eyeProjection[0], orthoScale0, orthoDistance,
                                                        eyeRenderDesc[0].ViewAdjust.x);
    orthoProjection[1] = ovrMatrix4f_OrthoSubProjection(eyeProjection[1], orthoScale1, orthoDistance,
                                                        eyeRenderDesc[1].ViewAdjust.x);
    
    // all done
    bHmdSettingsChanged = false;
}

ofQuaternion ofxOculusDK2::getOrientationQuat(){
//	return toOf(pFusionResult->GetPredictedOrientation());

	ovrTrackingState ts = ovrHmd_GetTrackingState(hmd, ovr_GetTimeInSeconds());
	if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked)){
		return toOf(ts.HeadPose.ThePose.Orientation);
	}
	 return ofQuaternion();
}

ofMatrix4x4 ofxOculusDK2::getOrientationMat(){
	
	//return toOf(Matrix4f(pFusionResult->GetPredictedOrientation()));
	
	ovrTrackingState ts = ovrHmd_GetTrackingState(hmd, ovr_GetTimeInSeconds());
	if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked)){
		return toOf( Matrix4f(ts.HeadPose.ThePose.Orientation));
	}
    return ofMatrix4x4();
}

void ofxOculusDK2::setupEyeParams(ovrEyeType eye){
	
//    OVR::Util::Render::StereoEyeParams eyeRenderParams = stereo.GetEyeRenderParams( eye );
//	OVR::Util::Render::Viewport VP = eyeRenderParams.VP;

	if(bUseBackground){
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		backgroundTarget.getTextureReference().draw(toOf(eyeTexture[eye].Header.RenderViewport));
		glPopAttrib();
	}
	
	ofSetMatrixMode(OF_MATRIX_PROJECTION);
	ofLoadIdentityMatrix();
	
	ofMatrix4x4 projectionMatrix = toOf(eyeProjection[eye]);

	ofLoadMatrix( projectionMatrix );
	
	ofSetMatrixMode(OF_MATRIX_MODELVIEW);
	ofLoadIdentityMatrix();
	
	
//	if(bUsePredictedOrientation){
//		orientationMatrix = toOf(Matrix4f(pFusionResult->GetPredictedOrientation()));
//	}
//	else{
//		orientationMatrix = toOf(Matrix4f(pFusionResult->GetOrientation()));
//	}
	
	ofMatrix4x4 headRotation = orientationMatrix;
	if(baseCamera != NULL){
		headRotation = headRotation * baseCamera->getGlobalTransformMatrix();
		baseCamera->begin();
		baseCamera->end();
	}
	
	// lock the camera when enabled...
	if (!lockView) {
		ofLoadMatrix( ofMatrix4x4::getInverseOf( headRotation ));
	}
	
	ofViewport(toOf(eyeTexture[eye].Header.RenderViewport));
//	ofMatrix4x4 viewAdjust = toOf(eyeRenderParams.ViewAdjust);
//	ofMultMatrix(viewAdjust);
	
}

ofRectangle ofxOculusDK2::getOculusViewport(){
//	OVR::Util::Render::StereoEyeParams eyeRenderParams = stereo.GetEyeRenderParams( OVR::Util::Render::StereoEye_Left );
//	return toOf(eyeRenderParams.VP);
    return toOf(eyeTexture[0].Header.RenderViewport);
}

void ofxOculusDK2::reloadShader(){
	//this allows you to hack on the shader if you'd like
	if(ofFile("Shaders/HmdWarp.vert").exists() && ofFile("Shaders/HmdWarp.frag").exists()){
		distortionShader.load("Shaders/HmdWarp");
	}
	//otherwise we load the hardcoded one
	else{
		distortionShader.setupShaderFromSource(GL_VERTEX_SHADER, OculusWarpVert);
		distortionShader.setupShaderFromSource(GL_FRAGMENT_SHADER, OculusWarpFrag);
		distortionShader.linkProgram();
	}
}

void ofxOculusDK2::beginBackground(){
	bUseBackground = true;
    backgroundTarget.begin();
    ofClear(0.0, 0.0, 0.0);
    ofPushView();
    ofPushMatrix();
    ofViewport(getOculusViewport());
    
}

void ofxOculusDK2::endBackground(){
    ofPopMatrix();
    ofPopView();
    backgroundTarget.end();
}


void ofxOculusDK2::beginOverlay(float overlayZ, float width, float height){
	bUseOverlay = true;
	overlayZDistance = overlayZ;
	
	if(overlayTarget.getWidth() != width || overlayTarget.getHeight() != height){
		overlayTarget.allocate(width, height, GL_RGBA, 4);
	}
	
	overlayMesh.clear();
	ofRectangle overlayrect = ofRectangle(-width/2,-height/2,width,height);
	overlayMesh.addVertex( ofVec3f(overlayrect.getMinX(), overlayrect.getMinY(), overlayZ) );
	overlayMesh.addVertex( ofVec3f(overlayrect.getMaxX(), overlayrect.getMinY(), overlayZ) );
	overlayMesh.addVertex( ofVec3f(overlayrect.getMinX(), overlayrect.getMaxY(), overlayZ) );
	overlayMesh.addVertex( ofVec3f(overlayrect.getMaxX(), overlayrect.getMaxY(), overlayZ) );

	overlayMesh.addTexCoord( ofVec2f(0, height ) );
	overlayMesh.addTexCoord( ofVec2f(width, height) );
	overlayMesh.addTexCoord( ofVec2f(0,0) );
	overlayMesh.addTexCoord( ofVec2f(width, 0) );
	
	overlayMesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
	
	overlayTarget.begin();
    ofClear(0.0, 0.0, 0.0, 0.0);
	
    ofPushView();
    ofPushMatrix();
}

void ofxOculusDK2::endOverlay(){
    ofPopMatrix();
    ofPopView();
    overlayTarget.end();
}

void ofxOculusDK2::beginLeftEye(){
	
	if(!bSetup) return;
	
	renderTarget.begin();
	ofClear(0,0,0);
	ofPushView();
	ofPushMatrix();
	setupEyeParams(ovrEye_Left);
}

void ofxOculusDK2::endLeftEye(){
	if(!bSetup) return;
	
	if(bUseOverlay){
		renderOverlay();
	}
	
	ofPopMatrix();
	ofPopView();
}

void ofxOculusDK2::beginRightEye(){
	if(!bSetup) return;
	
	ofPushView();
	ofPushMatrix();
	
	setupEyeParams(ovrEye_Right);
}

void ofxOculusDK2::endRightEye(){
	if(!bSetup) return;

	if(bUseOverlay){
		renderOverlay();
	}

	ofPopMatrix();
	ofPopView();
	renderTarget.end();	
}

void ofxOculusDK2::renderOverlay(){

//	cout << "renering overlay!" << endl;
	
	ofPushStyle();
	ofPushMatrix();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	
	if(baseCamera != NULL){
		ofTranslate(baseCamera->getPosition());
		ofMatrix4x4 baseRotation;
		baseRotation.makeRotationMatrix(baseCamera->getOrientationQuat());
		if(lockView){
			ofMultMatrix(baseRotation);
		}
		else {
			ofMultMatrix(orientationMatrix*baseRotation);
		}
	}
	
	ofEnableAlphaBlending();
	overlayTarget.getTextureReference().bind();
	overlayMesh.draw();
	overlayTarget.getTextureReference().unbind();
	
	glPopAttrib();
	ofPopMatrix();
	ofPopStyle();

}

ofVec3f ofxOculusDK2::worldToScreen(ofVec3f worldPosition, bool considerHeadOrientation){

	if(baseCamera == NULL){
		return ofVec3f(0,0,0);
	}

    ofRectangle viewport = getOculusViewport();

    if (considerHeadOrientation) {
        // We'll combine both left and right eye projections to get a midpoint.
//        OVR::Util::Render::StereoEyeParams eyeRenderParams = stereo.GetEyeRenderParams(OVR::Util::Render::StereoEye_Left);
//        ofMatrix4x4 projectionMatrixLeft = toOf(eyeRenderParams.Projection);
//        eyeRenderParams = stereo.GetEyeRenderParams(OVR::Util::Render::StereoEye_Right);
//        ofMatrix4x4 projectionMatrixRight = toOf(eyeRenderParams.Projection);
        ofMatrix4x4 projectionMatrixLeft = toOf(eyeProjection[ovrEye_Left]);
        ofMatrix4x4 projectionMatrixRight = toOf(eyeProjection[ovrEye_Right]);
        
        ofMatrix4x4 modelViewMatrix = orientationMatrix;
        modelViewMatrix = modelViewMatrix * baseCamera->getGlobalTransformMatrix();
        baseCamera->begin();
        baseCamera->end();
        modelViewMatrix = modelViewMatrix.getInverse();
    
        ofVec3f cameraXYZ = worldPosition * (modelViewMatrix * projectionMatrixLeft);
        cameraXYZ.interpolate(worldPosition * (modelViewMatrix * projectionMatrixRight), 0.5f);

        ofVec3f screenXYZ((cameraXYZ.x + 1.0f) / 2.0f * viewport.width + viewport.x,
                          (1.0f - cameraXYZ.y) / 2.0f * viewport.height + viewport.y,
                          cameraXYZ.z);        
        return screenXYZ;
    }
    
	return baseCamera->worldToScreen(worldPosition, viewport);
}

//TODO head orientation not considered
ofVec3f ofxOculusDK2::screenToWorld(ofVec3f screenPt, bool considerHeadOrientation) {

	if(baseCamera == NULL){
		return ofVec3f(0,0,0);
	}
    
    ofVec3f oculus2DPt = screenToOculus2D(screenPt, considerHeadOrientation);
    ofRectangle viewport = getOculusViewport();
    return baseCamera->screenToWorld(oculus2DPt, viewport);
}

//TODO head orientation not considered
ofVec3f ofxOculusDK2::screenToOculus2D(ofVec3f screenPt, bool considerHeadOrientation){

	ofRectangle viewport = getOculusViewport();
//  viewport.x -= viewport.width  / 2;
//	viewport.y -= viewport.height / 2;
	viewport.scaleFromCenter(oculusScreenSpaceScale);
    return ofVec3f(ofMap(screenPt.x, 0, ofGetWidth(),  viewport.getMinX(), viewport.getMaxX()),
                   ofMap(screenPt.y, 0, ofGetHeight(), viewport.getMinY(), viewport.getMaxY()),
                   screenPt.z);    
}

//TODO: head position!
ofVec3f ofxOculusDK2::mousePosition3D(float z, bool considerHeadOrientation){
//	ofVec3f cursor3D = screenToWorld(cursor2D);
	return screenToWorld(ofVec3f(ofGetMouseX(), ofGetMouseY(), z) );
}

float ofxOculusDK2::distanceFromMouse(ofVec3f worldPoint){
	//map the current 2D position into oculus space
	return distanceFromScreenPoint(worldPoint, ofVec3f(ofGetMouseX(), ofGetMouseY()) );
}

float ofxOculusDK2::distanceFromScreenPoint(ofVec3f worldPoint, ofVec2f screenPoint){
	ofVec3f cursorRiftSpace = screenToOculus2D(screenPoint);
	ofVec3f targetRiftSpace = worldToScreen(worldPoint);
	
	float dist = ofDist(cursorRiftSpace.x, cursorRiftSpace.y,
						targetRiftSpace.x, targetRiftSpace.y);
	return dist;
}


void ofxOculusDK2::multBillboardMatrix(){
	if(baseCamera == NULL){
		return;
	}
	ofVec3f mouse3d = mousePosition3D();
	ofNode n;
	n.setPosition(  mouse3d );
	n.lookAt(baseCamera->getPosition());
	ofVec3f axis; float angle;
	n.getOrientationQuat().getRotate(angle, axis);
	// Translate the object to its position.
	ofTranslate( mouse3d );
	// Perform the rotation.
	ofRotate(angle, axis.x, axis.y, axis.z);
}

ofVec2f ofxOculusDK2::gazePosition2D(){
    ofVec3f angles = getOrientationQuat().getEuler();
	return ofVec2f(ofMap(angles.y, 90, -90, 0, ofGetWidth()),
                   ofMap(angles.z, 90, -90, 0, ofGetHeight()));
}

void ofxOculusDK2::draw(){
	
	if(!bSetup) return;
	
	distortionShader.begin();
//	distortionShader.setUniformTexture("Texture0", renderTarget.getTextureReference(), 1);
//	distortionShader.setUniform2f("dimensions", renderTarget.getWidth(), renderTarget.getHeight());
//	const OVR::Util::Render::DistortionConfig& distortionConfig = stereo.GetDistortionConfig();
//    distortionShader.setUniform4f("HmdWarpParam",
//								  distortionConfig.K[0],
//								  distortionConfig.K[1],
//								  distortionConfig.K[2],
//								  distortionConfig.K[3]);
//    distortionShader.setUniform4f("ChromAbParam",
//                                  distortionConfig.ChromaticAberration[0],
//                                  distortionConfig.ChromaticAberration[1],
//                                  distortionConfig.ChromaticAberration[2],
//                                  distortionConfig.ChromaticAberration[3]);
//	
//	setupShaderUniforms(OVR::Util::Render::StereoEye_Left);
//	leftEyeMesh.draw();
//	
//	setupShaderUniforms(OVR::Util::Render::StereoEye_Right);
//	rightEyeMesh.draw();

	distortionShader.end();
	
	bUseOverlay = false;
	bUseBackground = false;
}

void ofxOculusDK2::setupShaderUniforms(ovrEyeType eye){

//	float w = .5;
//	float h = 1.0;
//	float y = 0;
//	float x;
//	float xCenter;
//	const OVR::Util::Render::DistortionConfig& distortionConfig = stereo.GetDistortionConfig();
//	if(eye == OVR::Util::Render::StereoEye_Left){
//		x = 0;
//		xCenter = distortionConfig.XCenterOffset;
//	}
//	else if(eye == OVR::Util::Render::StereoEye_Right){
//		x = .5;
//		xCenter = -distortionConfig.XCenterOffset;
//	}
//    	
//    float as = float(renderTarget.getWidth())/float(renderTarget.getHeight())*.5;
//    // We are using 1/4 of DistortionCenter offset value here, since it is
//    // relative to [-1,1] range that gets mapped to [0, 0.5].
//	ofVec2f lensCenter(x + (w + xCenter * 0.5f)*0.5f,
//					   y + h*0.5f);
//	
//    distortionShader.setUniform2f("LensCenter", lensCenter.x, lensCenter.y);
//	
//	ofVec2f screenCenter(x + w*0.5f, y + h*0.5f);
//    distortionShader.setUniform2f("ScreenCenter", screenCenter.x,screenCenter.y);
//	
//    // MA: This is more correct but we would need higher-res texture vertically; we should adopt this
//    // once we have asymmetric input texture scale.
//    float scaleFactor = 1.0f / distortionConfig.Scale;
//	//	cout << "scale factor " << scaleFactor << endl;
//	
//	ofVec2f scale( (w/2) * scaleFactor, (h/2) * scaleFactor * as);
//	ofVec2f scaleIn( (2/w), (2/h) / as);
//	
//    distortionShader.setUniform2f("Scale", scale.x,scale.y);
//    distortionShader.setUniform2f("ScaleIn",scaleIn.x,scaleIn.y);

//	cout << "UNIFORMS " << endl;
//	cout << "	scale " << scale << endl;
//	cout << "	scale in " << scaleIn << endl;
//	cout << "	screen center " << screenCenter << endl;
//	cout << "	lens center " << lensCenter << endl;
//	cout << "	scale factor " << scaleFactor << endl;
}

void ofxOculusDK2::setUsePredictedOrientation(bool usePredicted){
	bUsePredictedOrientation = usePredicted;
}
bool ofxOculusDK2::getUsePredictiveOrientation(){
	return bUsePredictedOrientation;
}

