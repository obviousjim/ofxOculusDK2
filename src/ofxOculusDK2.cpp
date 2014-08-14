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
    uniform vec2 EyeToSourceUVScale;
    uniform vec2 EyeToSourceUVOffset;
    uniform mat4 EyeRotationStart;
    uniform mat4 EyeRotationEnd;

    varying vec4 oColor;
    varying vec2 oTexCoord0;
    varying vec2 oTexCoord1;
    varying vec2 oTexCoord2;

    void main()
    {
		gl_Position.x = gl_Vertex.x;
		gl_Position.y = gl_Vertex.y;
		gl_Position.z = 0.0;
		gl_Position.w = 1.0;

		// Vertex inputs are in TanEyeAngle space for the R,G,B channels (i.e. after chromatic aberration and distortion).
		// These are now "real world" vectors in direction (x,y,1) relative to the eye of the HMD.
		vec3 TanEyeAngleR = vec3 ( gl_Normal.x, gl_Normal.y, 1.0 );
		vec3 TanEyeAngleG = vec3 ( gl_Color.r, gl_Color.g, 1.0 );
		vec3 TanEyeAngleB = vec3 ( gl_Color.b, gl_Color.a, 1.0 );

		// Accurate time warp lerp vs. faster

		mat3 EyeRotation;
		EyeRotation[0] = mix ( EyeRotationStart[0], EyeRotationEnd[0], gl_Vertex.z ).xyz;
		EyeRotation[1] = mix ( EyeRotationStart[1], EyeRotationEnd[1], gl_Vertex.z ).xyz;
		EyeRotation[2] = mix ( EyeRotationStart[2], EyeRotationEnd[2], gl_Vertex.z ).xyz;

		vec3 TransformedR   = EyeRotation * TanEyeAngleR;
		vec3 TransformedG   = EyeRotation * TanEyeAngleG;
		vec3 TransformedB   = EyeRotation * TanEyeAngleB;

		// Project them back onto the Z=1 plane of the rendered images.
		float RecipZR = 1.0 / TransformedR.z;
		float RecipZG = 1.0 / TransformedG.z;
		float RecipZB = 1.0 / TransformedB.z;
		vec2 FlattenedR = vec2 ( TransformedR.x * RecipZR, TransformedR.y * RecipZR );
		vec2 FlattenedG = vec2 ( TransformedG.x * RecipZG, TransformedG.y * RecipZG );
		vec2 FlattenedB = vec2 ( TransformedB.x * RecipZB, TransformedB.y * RecipZB );

		// These are now still in TanEyeAngle space.
		// Scale them into the correct [0-1],[0-1] UV lookup space (depending on eye)
		vec2 SrcCoordR = FlattenedR * EyeToSourceUVScale + EyeToSourceUVOffset;
		vec2 SrcCoordG = FlattenedG * EyeToSourceUVScale + EyeToSourceUVOffset;
		vec2 SrcCoordB = FlattenedB * EyeToSourceUVScale + EyeToSourceUVOffset;

		oTexCoord0 = SrcCoordR;
		oTexCoord0.y = 1.0-oTexCoord0.y;
		oTexCoord1 = SrcCoordG;
		oTexCoord1.y = 1.0-oTexCoord1.y;
		oTexCoord2 = SrcCoordB;
		oTexCoord2.y = 1.0-oTexCoord2.y;

		oColor = vec4(gl_Normal.z, gl_Normal.z, gl_Normal.z, gl_Normal.z);
	}
);
                               
static const char* OculusWarpFrag = GLSL(120,
    uniform sampler2D Texture;
    
    varying vec4 oColor;
    varying vec2 oTexCoord0;
    varying vec2 oTexCoord1;
    varying vec2 oTexCoord2;
    
    void main()
    {
       gl_FragColor.r = oColor.r * texture2D(Texture, oTexCoord0).r;
       gl_FragColor.g = oColor.g * texture2D(Texture, oTexCoord1).g;
       gl_FragColor.b = oColor.b * texture2D(Texture, oTexCoord2).b;
       gl_FragColor.a = 1.0;
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

ofRectangle toOf(const ovrRecti& vp){
	return ofRectangle(vp.Pos.x,vp.Pos.y,vp.Size.w,vp.Size.h);
}

ofVec3f toOf(const ovrVector3f& v){
	return ofVec3f(v.x,v.y,v.z);
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
//  pFusionResult = new SensorFusion();
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

	initializeClientRenderer();

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
	/*
	float w = windowSize.w;
	float h = windowSize.h;
	
	renderTarget.allocate(w, h, GL_RGB);
	*/

	//TODO: PULL BACKGROUND BACK IN
	/*
    backgroundTarget.allocate(w/2, h);
	backgroundTarget.begin();
    ofClear(0.0, 0.0, 0.0);
	backgroundTarget.end();
		
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
    */

    bPositionTrackingEnabled = (hmd->TrackingCaps & ovrTrackingCap_Position);
	reloadShader();
	bSetup = true;
	return true;
}

void ofxOculusDK2::initializeClientRenderer(){

	Sizei recommenedTex0Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Left, hmd->DefaultEyeFov[0], 1.0f);
	Sizei recommenedTex1Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Right, hmd->DefaultEyeFov[1], 1.0f);

	renderTargetSize.w = recommenedTex0Size.w + recommenedTex1Size.w;
	renderTargetSize.h = max ( recommenedTex0Size.h, recommenedTex1Size.h );

	const int eyeRenderMultisample = 1;
	renderTarget.allocate(renderTargetSize.w, renderTargetSize.h,GL_RGBA,eyeRenderMultisample);
	

	eyeRenderDesc[0] = ovrHmd_GetRenderDesc(hmd, ovrEye_Left, eyeFov[0]);
	eyeRenderDesc[1] = ovrHmd_GetRenderDesc(hmd, ovrEye_Right, eyeFov[1]);

	eyeRenderViewport[0].Pos  = Vector2i(0,0);
    eyeRenderViewport[0].Size = Sizei(renderTargetSize.w / 2, renderTargetSize.h);
    eyeRenderViewport[1].Pos  = Vector2i((renderTargetSize.w + 1) / 2, 0);
    eyeRenderViewport[1].Size = eyeRenderViewport[0].Size;

	//Generate distortion mesh for each eye
	for ( int eyeNum = 0; eyeNum < 2; eyeNum++ ){
		// Allocate & generate distortion mesh vertices.
		ovrDistortionMesh meshData;
//		int caps = ovrDistortionCap_Chromatic | ovrDistortionCap_TimeWarp | ovrDistortionCap_Vignette;
		int caps = ovrDistortionCap_Chromatic | ovrDistortionCap_Vignette;

		ovrHmd_CreateDistortionMesh(hmd, eyeRenderDesc[eyeNum].Eye, eyeRenderDesc[eyeNum].Fov, caps, &meshData);
		ovrHmd_GetRenderScaleAndOffset(eyeRenderDesc[eyeNum].Fov, renderTargetSize, eyeRenderViewport[eyeNum], UVScaleOffset[eyeNum]);

		// Now parse the vertex data and create a render ready vertex buffer from it
//		DistortionVertex * pVBVerts = (DistortionVertex*)OVR_ALLOC(sizeof(DistortionVertex) * meshData.VertexCount );
//		DistortionVertex * v = pVBVerts;
		ofVboMesh& v = eyeMesh[eyeNum];
		v.clear();
		v.getVertices().resize(meshData.VertexCount);
		v.getColors().resize(meshData.VertexCount);
//		v.getTexCoords().resize(meshData.VertexCount);
		v.getNormals().resize(meshData.VertexCount);

		ovrDistortionVertex * ov = meshData.pVertexData;
		for( unsigned vertNum = 0; vertNum < meshData.VertexCount; vertNum++ ){

			v.getVertices()[vertNum].x = ov->ScreenPosNDC.x;
			v.getVertices()[vertNum].y = ov->ScreenPosNDC.y;
			v.getVertices()[vertNum].z = ov->TimeWarpFactor;

			v.getNormals()[vertNum].x = ov->TanEyeAnglesR.x;
			v.getNormals()[vertNum].y = ov->TanEyeAnglesR.y;
			v.getNormals()[vertNum].z = ov->VignetteFactor;

			v.getColors()[vertNum].r = ov->TanEyeAnglesG.x;
			v.getColors()[vertNum].g = ov->TanEyeAnglesG.y;
			v.getColors()[vertNum].b = ov->TanEyeAnglesB.x;
			v.getColors()[vertNum].a = ov->TanEyeAnglesB.y;
			
			ov++;
		}

		//Register this mesh with the renderer
		v.getIndices().resize(meshData.IndexCount);

		unsigned short * oi = meshData.pIndexData;
		for(int i = 0; i < meshData.IndexCount; i++){
			v.getIndices()[i] = *oi;
			oi++;
		}

		ovrHmd_DestroyDistortionMesh( &meshData );
	}
}

bool ofxOculusDK2::isSetup(){
	return bSetup;
}

void ofxOculusDK2::reset(){
	if(bSetup){
//		pFusionResult->Reset();
	}
}

/*
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
*/

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
	
//  OVR::Util::Render::StereoEyeParams eyeRenderParams = stereo.GetEyeRenderParams( eye );
//	OVR::Util::Render::Viewport VP = eyeRenderParams.VP;

	if(bUseBackground){
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		backgroundTarget.getTextureReference().draw(toOf(eyeRenderViewport[eye]));
		glPopAttrib();
	}
    
	headPose[eye] = ovrHmd_GetEyePose(hmd, eye);

	ofSetMatrixMode(OF_MATRIX_PROJECTION);
	ofLoadIdentityMatrix();
	
	ofMatrix4x4 projectionMatrix = ofMatrix4x4::getTransposedOf( toOf(ovrMatrix4f_Projection(eyeRenderDesc[eye].Fov, 100.f, 100000.0f, false) ) );
	ofLoadMatrix( projectionMatrix );
	
	//what to do about this 
	//******************
	//Matrix4f view = Matrix4f(orientation.Inverted()) * Matrix4f::Translation(-WorldEyePosition);
	//and this view adjust
	//Matrix4f::Translation(EyeRenderDesc[eye].ViewAdjust) * view);
	//******************

	ofSetMatrixMode(OF_MATRIX_MODELVIEW);
	ofLoadIdentityMatrix();
		
	//orientationMatrix = ofMatrix4x4::getTransposedOf( getOrientationMat() );
	orientationMatrix = getOrientationMat();
	
	ofMatrix4x4 headRotation = orientationMatrix;
	if(baseCamera != NULL){
		headRotation = headRotation * baseCamera->getGlobalTransformMatrix();
		baseCamera->begin();
		baseCamera->end();
	}
	
	// lock the camera when enabled...
	if (!lockView) {
		ofLoadMatrix( ofMatrix4x4::getInverseOf( headRotation ));
//		ofLoadMatrix( headRotation );
	}
	
	ofViewport(toOf(eyeRenderViewport[eye]));
	ofMatrix4x4 viewAdjust;
	viewAdjust.makeTranslationMatrix( toOf(eyeRenderDesc[eye].ViewAdjust) );
	ofMultMatrix(viewAdjust);
	
}

ofRectangle ofxOculusDK2::getOculusViewport(){
//	OVR::Util::Render::StereoEyeParams eyeRenderParams = stereo.GetEyeRenderParams( OVR::Util::Render::StereoEye_Left );
//	return toOf(eyeRenderParams.VP);
    return toOf(eyeRenderViewport[0]);
}

void ofxOculusDK2::reloadShader(){
	//this allows you to hack on the shader if you'd like
	if(ofFile("Shaders/HmdWarp.vert").exists() && ofFile("Shaders/HmdWarp.frag").exists()){
		cout << "** SHADERS loading from file" << endl;
		distortionShader.load("Shaders/HmdWarp");
	}
	//otherwise we load the hardcoded one
	else{
		cout << OculusWarpVert << endl<<endl<<endl;
		cout << OculusWarpFrag << endl;

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
	
	frameTiming = ovrHmd_BeginFrameTiming(hmd, 0);

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

        ofMatrix4x4 projectionMatrixLeft = toOf(ovrMatrix4f_Projection(eyeRenderDesc[ovrEye_Left].Fov, 0.01f, 10000.0f, true));
        ofMatrix4x4 projectionMatrixRight = toOf(ovrMatrix4f_Projection(eyeRenderDesc[ovrEye_Right].Fov, 0.01f, 10000.0f, true));
        
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
	
	ovr_WaitTillTime(frameTiming.TimewarpPointSeconds);

	///JG START HERE 
	// Prepare for distortion rendering. 
	/*
	distortionShader.begin();
	distortionShader.setUniformTexture("Texture", renderTarget.getTextureReference(), 1);
	//???
//	distortionShaderFill.SetInputLayout(DistortionData.VertexIL);
	for (int eyeIndex = 0; eyeIndex < 2; eyeIndex++) {
		// Setup shader constants
		distortionShader.setUniform2f("EyeToSourceUVScale", UVScaleOffset[eyeIndex][0].x, 
															UVScaleOffset[eyeIndex][0].y);
		distortionShader.setUniform2f("EyeToSourceUVOffset", UVScaleOffset[eyeIndex][1].x, 
															 UVScaleOffset[eyeIndex][1].y);
		ovrMatrix4f timeWarpMatrices[2];
		ovrHmd_GetEyeTimewarpMatrices(hmd, (ovrEyeType) eyeIndex, headPose[eyeIndex], timeWarpMatrices);
		distortionShader.setUniformMatrix4f("EyeRotationStart", toOf(timeWarpMatrices[0]) );
		distortionShader.setUniformMatrix4f("EyeRotationEnd", toOf(timeWarpMatrices[1]) );
		// Perform distortion
//		pRender->Render(&distortionShaderFill,
//		DistortionData.MeshVBs[eyeIndex], DistortionData.MeshIBs[eyeIndex]);
		eyeMesh[eyeIndex].draw();
	}
	distortionShader.end();
	*/

	// JG Test Output

	/////////////////////
	ovrHmd_EndFrameTiming(hmd);
    
    // EZ
    //    StereoEyeParams CalculateStereoEyeParams ( HmdRenderInfo const &hmd,
    //                                              StereoEye eyeType,
    //                                              Sizei const &actualRendertargetSurfaceSize,
    //                                              bool bRendertargetSharedByBothEyes,
    //                                              bool bRightHanded = true,
    //                                              float zNear = 0.01f, float zFar = 10000.0f,
    //                                              Sizei const *pOverrideRenderedPixelSize = NULL,
    //                                              FovPort const *pOverrideFovport = NULL,
    //                                              float zoomFactor = 1.0f );
    HMDState* p = (HMDState *)hmd;
    StereoEyeParams eyeParams = CalculateStereoEyeParams(p->RenderState.RenderInfo, StereoEye_Left, renderTargetSize, true);

	distortionShader.begin();
	distortionShader.setUniformTexture("Texture0", renderTarget.getTextureReference(), 1);
	distortionShader.setUniform2f("dimensions", renderTarget.getWidth(), renderTarget.getHeight());
//	const OVR::Util::Render::DistortionConfig& distortionConfig = stereo.GetDistortionConfig();
//    distortionShader.setUniform4f("HmdWarpParam",
//								  distortionConfig.K[0],
//								  distortionConfig.K[1],
//								  distortionConfig.K[2],
//								  distortionConfig.K[3]);
    // EZ
    // Note that the shader currently doesn't use Distortion.K[0], it hardwires it to 1.0.
    distortionShader.setUniform4f("HmdWarpParam",
                                  1.0f,
                                  eyeParams.Distortion.Lens.K[1],
                                  eyeParams.Distortion.Lens.K[2],
                                  eyeParams.Distortion.Lens.K[3]);
//    distortionShader.setUniform4f("ChromAbParam",
//                                  distortionConfig.ChromaticAberration[0],
//                                  distortionConfig.ChromaticAberration[1],
//                                  distortionConfig.ChromaticAberration[2],
//                                  distortionConfig.ChromaticAberration[3]);
    // EZ
    // These are stored as deltas off the "main" distortion coefficients, but
    // in the shader we use them as absolute values.
    distortionShader.setUniform4f("ChromAbParam",
                                  eyeParams.Distortion.Lens.ChromaticAberration[0] + 1.0f,
                                  eyeParams.Distortion.Lens.ChromaticAberration[1],
                                  eyeParams.Distortion.Lens.ChromaticAberration[2] + 1.0f,
                                  eyeParams.Distortion.Lens.ChromaticAberration[3]);
//
//	setupShaderUniforms(OVR::Util::Render::StereoEye_Left);
//	leftEyeMesh.draw();
//	
//	setupShaderUniforms(OVR::Util::Render::StereoEye_Right);
//	rightEyeMesh.draw();

	distortionShader.end();
	
	renderTarget.getTextureReference().draw(0,0, ofGetWidth(), ofGetHeight());

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

