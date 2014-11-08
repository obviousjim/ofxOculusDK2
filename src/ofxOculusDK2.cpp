//
//  ofxOculusDK2.cpp
//  OculusRiftRendering
//
//  Created by Andreas Müller on 30/04/2013.
//  Updated by James George September 27th 2013
//  Updated by Jason Walters October 22 2013
//  Adapted to DK2 by James George and Elie Zananiri August 2014
//  Updated for DK2 by Matt Ebb October 2014

#include "ofxOculusDK2.h"

//ofxOculusDK2::bInitialized = false;
bool ofxOculusDK2::bInitialized = false;
ovrHmd ofxOculusDK2::hmd = 0;

#define SDK_RENDER 1

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
		gl_Position.z = .0;
		gl_Position.w = 1.0;

		// Vertex inputs are in TanEyeAngle space for the R,G,B channels (i.e. after chromatic aberration and distortion).
		// These are now "real world" vectors in direction (x,y,1) relative to the eye of the HMD.
		vec3 TanEyeAngleR = vec3 ( gl_Normal.x, gl_Normal.y, 1.0 );
		vec3 TanEyeAngleG = vec3 ( gl_Color.r, gl_Color.g, 1.0 );
		vec3 TanEyeAngleB = vec3 ( gl_Color.b, gl_Color.a, 1.0 );

		mat3 EyeRotation;
		EyeRotation[0] = mix ( EyeRotationStart[0], EyeRotationEnd[0], gl_Vertex.z ).xyz;
		EyeRotation[1] = mix ( EyeRotationStart[1], EyeRotationEnd[1], gl_Vertex.z ).xyz;
		EyeRotation[2] = mix ( EyeRotationStart[2], EyeRotationEnd[2], gl_Vertex.z ).xyz;

		vec3 TransformedR = EyeRotation * TanEyeAngleR;
		vec3 TransformedG = EyeRotation * TanEyeAngleG;
		vec3 TransformedB = EyeRotation * TanEyeAngleB;

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

        //Vignette
		oColor = vec4(gl_Normal.z, gl_Normal.z, gl_Normal.z, gl_Normal.z);
	}
);
                               
static const char* OculusWarpFrag = GLSL(120,
	uniform sampler2DRect Texture;
	uniform vec2 TextureScale;

	varying vec4 oColor;
	varying vec2 oTexCoord0;
	varying vec2 oTexCoord1;
	varying vec2 oTexCoord2;
    
	void main()
	{
	  gl_FragColor.r = oColor.r * texture2DRect(Texture, oTexCoord0 * TextureScale).r;
	  gl_FragColor.g = oColor.g * texture2DRect(Texture, oTexCoord1 * TextureScale).g;
	  gl_FragColor.b = oColor.b * texture2DRect(Texture, oTexCoord2 * TextureScale).b;
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

ovrVector3f toOVR(const ofVec3f& v ){
	ovrVector3f ov;
	ov.x = v.x;
	ov.y = v.y;
	ov.z = v.z;
	return ov;
}

ofxOculusDK2::ofxOculusDK2(){


    insideFrame = false;
    frameIndex = 0;

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
	applyTranslation = true;
}

ofxOculusDK2::~ofxOculusDK2(){
	if(bSetup){

        if (hmd) {
            ovrHmd_Destroy(hmd);
            hmd = 0;
        }
        
        ovr_Shutdown();
        
		bSetup = false;
	}
}

void ofxOculusDK2::initialize(){

	int x = 0;
	int y = 0;
	bool debug = false;
	bInitialized = true;

	ovr_Initialize();
 	hmd = ovrHmd_Create(0);
	if (hmd == NULL)
	{
		hmd = ovrHmd_CreateDebug(ovrHmd_DK1);
		//bUsingDebugHmd = true;
	}

}

bool ofxOculusDK2::setup(){
	ofFbo::Settings settings;
	//settings.numSamples = 4;
	settings.numSamples = 1;
	settings.internalformat = GL_RGBA;
	settings.useDepth = true;
	settings.textureTarget = GL_TEXTURE_2D;
	settings.minFilter = GL_LINEAR;
	settings.maxFilter = GL_LINEAR;
	settings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
	settings.wrapModeVertical = GL_CLAMP_TO_EDGE;
	return setup(settings);
}

bool ofxOculusDK2::setup(ofFbo::Settings& render_settings){
	
	if(bSetup){
		ofLogError("ofxOculusDK2::setup") << "Already set up";
		return false;
	}

	if(!bInitialized){
		initialize();
	}

	/*
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
            printf("simulated hmd->resolution %d %d \n", hmd->Resolution.w, hmd->Resolution.h);
            bUsingDebugHmd = true;
        }
	}
    */

	eyeFov[0] = hmd->DefaultEyeFov[0];
	eyeFov[1] = hmd->DefaultEyeFov[1];
	Sizei recommendedTex0Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Left, eyeFov[0], 1.0f);
    Sizei recommendedTex1Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Right, eyeFov[1], 1.0f);
    
    renderTargetSize.w = recommendedTex0Size.w + recommendedTex1Size.w;
    renderTargetSize.h = max(recommendedTex0Size.h, recommendedTex1Size.h);
 
	render_settings.width = renderTargetSize.w;
	render_settings.height = renderTargetSize.h;
	renderTarget.allocate(render_settings);

	/*
    glGenFramebuffers(1, &frameBuffer);
 
    glGenTextures(1, &texture);
 
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, renderTargetSize.w, renderTargetSize.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
 
    glGenRenderbuffers(1, &renderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, renderTargetSize.w, renderTargetSize.h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBuffer);
 
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
		glDeleteFramebuffers(1, &frameBuffer);
		glDeleteTextures(1, &texture);
		glDeleteRenderbuffers(1, &renderBuffer);
 
		ofLogError("Framebuffer failed!");
		ofExit(0);
    }
	*/

    ovrFovPort eyeFov[2] = { hmd->DefaultEyeFov[0], hmd->DefaultEyeFov[1] };
 
    eyeRenderViewport[0].Pos = Vector2i(0, 0);
    eyeRenderViewport[0].Size = Sizei(renderTargetSize.w / 2, renderTargetSize.h);
    eyeRenderViewport[1].Pos = Vector2i((renderTargetSize.w + 1) / 2, 0);
    eyeRenderViewport[1].Size = eyeRenderViewport[0].Size;
 
    eyeTexture[0].OGL.Header.API = ovrRenderAPI_OpenGL;
    eyeTexture[0].OGL.Header.TextureSize = renderTargetSize;
    eyeTexture[0].OGL.Header.RenderViewport = eyeRenderViewport[0];
//    eyeTexture[0].OGL.TexId = texture;
	eyeTexture[0].OGL.TexId = renderTarget.getTextureReference().getTextureData().textureID;
 
    eyeTexture[1] = eyeTexture[0];
    eyeTexture[1].OGL.Header.RenderViewport = eyeRenderViewport[1];
 
    ovrGLConfig cfg;
    cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
    cfg.OGL.Header.RTSize = Sizei(hmd->Resolution.w, hmd->Resolution.h);
    cfg.OGL.Header.Multisample = 1;

#ifdef OVR_OS_WIN32
    if (!(hmd->HmdCaps & ovrHmdCap_ExtendDesktop)) {
		ovrHmd_AttachToWindow(hmd, ofGetWin32Window(), NULL, NULL);
	}
 
	cfg.OGL.Window = ofGetWin32Window();
    cfg.OGL.DC = NULL;
#endif
 
    ovrHmd_ConfigureRendering(hmd, &cfg.Config, ovrDistortionCap_Chromatic | ovrDistortionCap_Vignette | ovrDistortionCap_TimeWarp | ovrDistortionCap_Overdrive, eyeFov, eyeRenderDesc);
    ovrHmd_SetEnabledCaps(hmd, ovrHmdCap_LowPersistence | ovrHmdCap_DynamicPrediction);
    ovrHmd_ConfigureTracking(hmd, ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position, 0);


    backgroundTarget.allocate(renderTargetSize.w/2, renderTargetSize.h);
	backgroundTarget.begin();
    ofClear(0.0, 0.0, 0.0);
	backgroundTarget.end();

	/*
    if (hmd->HmdCaps & ovrHmdCap_ExtendDesktop) {
        windowSize = hmd->Resolution;
        printf("hmd->resolution %d %d \n", hmd->Resolution.w, hmd->Resolution.h);
    }
    else {
        // In Direct App-rendered mode, we can use smaller window size,
        // as it can have its own contents and isn't tied to the buffer.
        windowSize = Sizei(ofGetWidth(), ofGetHeight()); //Sizei(960, 540); avoid rotated output bug.
    }
    
	// Start the sensor which provides the Rift’s pose and motion.
	ovrHmd_ConfigureTracking(hmd, 
		ovrTrackingCap_Orientation | 
		ovrTrackingCap_MagYawCorrection | 
		ovrTrackingCap_Position, 0);

	eyeFov[0] = hmd->DefaultEyeFov[0];
	eyeFov[1] = hmd->DefaultEyeFov[1];
    
    Sizei recommenedTex0Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Left, eyeFov[0], 1.0f);
	Sizei recommenedTex1Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Right, eyeFov[1], 1.0f);
    
	renderTargetSize.w = recommenedTex0Size.w + recommenedTex1Size.w;
	renderTargetSize.h = max ( recommenedTex0Size.h, recommenedTex1Size.h );

	render_settings.width = renderTargetSize.w;
	render_settings.height = renderTargetSize.h;
    
    glGenFramebuffers(1, &frameBuffer);
 
    glGenTextures(1, &texture);
 
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, renderTargetSize.w, renderTargetSize.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
 
    glGenRenderbuffers(1, &renderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, renderTargetSize.w, renderTargetSize.h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBuffer);
    //renderTarget.allocate(render_settings);


	eyeRenderDesc[0] = ovrHmd_GetRenderDesc(hmd, ovrEye_Left, eyeFov[0]);
	eyeRenderDesc[1] = ovrHmd_GetRenderDesc(hmd, ovrEye_Right, eyeFov[1]);
    
    hmdToEyeViewOffsets[0] = eyeRenderDesc[0].HmdToEyeViewOffset;
    hmdToEyeViewOffsets[1] = eyeRenderDesc[1].HmdToEyeViewOffset;

	eyeRenderViewport[0].Pos  = Vector2i(0,0);
    eyeRenderViewport[0].Size = Sizei(renderTargetSize.w / 2, renderTargetSize.h);
    eyeRenderViewport[1].Pos  = Vector2i((renderTargetSize.w + 1) / 2, 0);
    eyeRenderViewport[1].Size = eyeRenderViewport[0].Size;

    unsigned int distortionCaps = ovrDistortionCap_Chromatic | ovrDistortionCap_TimeWarp | ovrDistortionCap_Vignette | ovrDistortionCap_Overdrive; // | ovrDistortionCap_SRGB;
    
#ifdef SDK_RENDER
    // END mattebb SDK rendering test
    ovrRenderAPIConfig config = ovrRenderAPIConfig();
    config.Header.API = ovrRenderAPI_OpenGL;
    config.Header.RTSize = Sizei(hmd->Resolution.w, hmd->Resolution.h);
    config.Header.Multisample = 0; // configurable ?
    
    // Store texture pointers that will be passed for rendering.
    // Same texture is used, but with different viewports.
    memset(EyeTexture, 0, 2 * sizeof(ovrGLTexture));
    EyeTexture[0].Header.API            = ovrRenderAPI_OpenGL;
    EyeTexture[0].Header.TextureSize    = renderTargetSize;
    EyeTexture[0].Header.RenderViewport = eyeRenderViewport[0];

    // same texture, shifted viewport
    EyeTexture[1] = EyeTexture[0];
    EyeTexture[1].Header.RenderViewport = eyeRenderViewport[1];
    
    // set the tex IDs from the ofFbo
    //((ovrGLTexture &)EyeTexture[0]).OGL.TexId = renderTarget.getFbo();
    //((ovrGLTexture &)EyeTexture[1]).OGL.TexId = renderTarget.getFbo();
	((ovrGLTexture &)EyeTexture[0]).OGL.TexId = texture;
    ((ovrGLTexture &)EyeTexture[1]).OGL.TexId = texture;

    cout << "renderTargetsize : " << renderTargetSize.w << " " << renderTargetSize.h << endl;
    cout << "eye tex 0  viewpos: " << EyeTexture[0].Header.RenderViewport.Pos.x << " " << EyeTexture[0].Header.RenderViewport.Pos.y << endl;
    cout << "eye tex 0 size: " << EyeTexture[0].Header.RenderViewport.Size.w << " " << EyeTexture[0].Header.RenderViewport.Size.h << endl;
    cout << "eye tex 1 pos: " << EyeTexture[1].Header.RenderViewport.Pos.x << " " << EyeTexture[1].Header.RenderViewport.Pos.y << endl;
    cout << "eye tex 1 size: " << EyeTexture[1].Header.RenderViewport.Size.w << " " << EyeTexture[1].Header.RenderViewport.Size.h << endl;
    
    int hmdCaps;
    hmdCaps |= ovrHmdCap_DynamicPrediction;
    hmdCaps |= ovrHmdCap_LowPersistence;
    
    ovrHmd_SetEnabledCaps(hmd, hmdCaps);
    
    if (!ovrHmd_ConfigureRendering( hmd, &config, distortionCaps, eyeFov, eyeRenderDesc ))
    {
        // Fail exit? TBD
        return false;
    }
    
    // END mattebb SDK rendering test
#else
	//Generate distortion mesh for each eye
	for ( int eyeNum = 0; eyeNum < 2; eyeNum++ ){
		// Allocate & generate distortion mesh vertices.
		ovrDistortionMesh meshData;

		ovrHmd_CreateDistortionMesh(hmd, eyeRenderDesc[eyeNum].Eye, eyeRenderDesc[eyeNum].Fov, distortionCaps, &meshData);
		ovrHmd_GetRenderScaleAndOffset(eyeRenderDesc[eyeNum].Fov, renderTargetSize, eyeRenderViewport[eyeNum], UVScaleOffset[eyeNum]);

		// Now parse the vertex data and create a render ready vertex buffer from it
		ofVboMesh& v = eyeMesh[eyeNum];
		v.clear();
		v.getVertices().resize(meshData.VertexCount);
		v.getColors().resize(meshData.VertexCount);
		v.getNormals().resize(meshData.VertexCount);
//		v.getTexCoords().resize(meshData.VertexCount);
		ovrDistortionVertex * ov = meshData.pVertexData;
		for( unsigned vertNum = 0; vertNum < meshData.VertexCount; vertNum++ ){
			
			v.getVertices()[vertNum].x = ov->ScreenPosNDC.x;
			v.getVertices()[vertNum].y = ov->ScreenPosNDC.y;
			//cout << vertNum<< "/" << meshData.VertexCount << "SCREEN POS IS " << ov->ScreenPosNDC.x << " " <<  ov->ScreenPosNDC.y << endl;
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
    
    reloadShader();
    
#endif
	*/
    bPositionTrackingEnabled = (hmd->TrackingCaps & ovrTrackingCap_Position);

	bSetup = true;
	return true;
}

bool ofxOculusDK2::isSetup(){
	return bSetup;
}

void ofxOculusDK2::reset(){
	if(bSetup){
		ovrHmd_RecenterPose(hmd);
	}
}

ofQuaternion ofxOculusDK2::getOrientationQuat(){
//	return toOf(pFusionResult->GetPredictedOrientation());

	ovrTrackingState ts = ovrHmd_GetTrackingState(hmd, ovr_GetTimeInSeconds());
	if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked)){
		return toOf(ts.HeadPose.ThePose.Orientation);
	}
	 return ofQuaternion();
}

ofMatrix4x4 ofxOculusDK2::getProjectionMatrix(ovrEyeType eye) {
    return toOf(ovrMatrix4f_Projection(eyeRenderDesc[eye].Fov, .01f, 10000.0f, true) );
}

ofMatrix4x4 ofxOculusDK2::getViewMatrix(ovrEyeType eye) {

    ofMatrix4x4 baseCameraMatrix = baseCamera->getModelViewMatrix();
    //ofMatrix4x4 viewAdjust;
    
    // pre-transform offset for IPD
    //viewAdjust.makeTranslationMatrix( toOf(eyeRenderDesc[eye].HmdToEyeViewOffset) );
    
    // head orientation and position
    ofMatrix4x4 hmdView =   ofMatrix4x4::newRotationMatrix( toOf(headPose[eye].Orientation)) * \
    ofMatrix4x4::newTranslationMatrix( toOf(headPose[eye].Position));
    
    // final multiplication of everything
    //return viewAdjust * hmdView.getInverse() * baseCameraMatrix;
    return baseCameraMatrix * hmdView.getInverse();
}

void ofxOculusDK2::setupEyeParams(ovrEyeType eye){
	
	if(bUseBackground){
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glDisable(GL_LIGHTING);
		ofDisableDepthTest();
		backgroundTarget.getTextureReference().draw(toOf(eyeRenderViewport[eye]));
		glPopAttrib();
	}
    
    // xxx mattebb
    frameIndex=0;
    ovrHmd_GetEyePoses(hmd, frameIndex, hmdToEyeViewOffsets, headPose, NULL);

	ofViewport(toOf(eyeRenderViewport[eye]));

	ofSetMatrixMode(OF_MATRIX_PROJECTION);
	ofLoadIdentityMatrix();
	ofLoadMatrix( getProjectionMatrix(eye) );
    
	ofSetMatrixMode(OF_MATRIX_MODELVIEW);
	ofLoadIdentityMatrix();
    ofLoadMatrix( getViewMatrix(eye) );
}

ofRectangle ofxOculusDK2::getOculusViewport(){
//	OVR::Util::Render::StereoEyeParams eyeRenderParams = stereo.GetEyeRenderParams( OVR::Util::Render::StereoEye_Left );
//	return toOf(eyeRenderParams.VP);
    return toOf(eyeRenderViewport[0]);
}

void ofxOculusDK2::reloadShader(){
    // XXX mattebb
    /*
     if(ofFile("Shaders_GL3/debug.vert").exists() && ofFile("Shaders_GL3/debug.frag").exists()){
		cout << "** debug SHADERS loading from file" << endl;
		debugShader.load("Shaders_GL3/debug");
	}
     */
    
    
	//this allows you to hack on the shader if you'd like
    if (ofIsGLProgrammableRenderer()) {
        if(ofFile("Shaders_GL3/HmdWarpDK2.vert").exists() && ofFile("Shaders_GL3/HmdWarpDK2.frag").exists()){
            cout << "** SHADERS loading from file" << endl;
            distortionShader.load("Shaders_GL3/HmdWarpDK2");
        }
        //otherwise we load the hardcoded one
        else{   // XXX mattebb : create an embedded shader for GL3 ?
            cout << OculusWarpVert << endl<<endl<<endl;
            cout << OculusWarpFrag << endl;
            distortionShader.setupShaderFromSource(GL_VERTEX_SHADER, OculusWarpVert);
            distortionShader.setupShaderFromSource(GL_FRAGMENT_SHADER, OculusWarpFrag);
            distortionShader.linkProgram();
        }
    } else {
        if(ofFile("Shaders/HmdWarpDK2.vert").exists() && ofFile("Shaders/HmdWarpDK2.frag").exists()){
            cout << "** SHADERS loading from file" << endl;
            distortionShader.load("Shaders/HmdWarpDK2");
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
}

void ofxOculusDK2::beginBackground(){
	bUseBackground = true;
	insideFrame = true;
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
	
#ifdef SDK_RENDER
    frameTiming = ovrHmd_BeginFrame(hmd, 0);
#else
    frameTiming = ovrHmd_BeginFrameTiming(hmd, ++frameIndex);
#endif
    
	insideFrame = true;

	renderTarget.begin();

	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 	
	ofPushView();
//	ofViewport(0,0,renderTargetSize.w, renderTargetSize.h);
//	ofSetupScreenPerspective(renderTargetSize.w, renderTargetSize.h, baseCamera->getFov(), baseCamera->getNearClip(), baseCamera->getFarClip());
//	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	ofClear(255,0,0);

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
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);

	renderTarget.end();	
}

void ofxOculusDK2::renderOverlay(){

	// cout << "renering overlay!" << endl;
	
	ofPushStyle();
	ofPushMatrix();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glDisable(GL_LIGHTING);
	ofDisableDepthTest();

	
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
    
    ofMatrix4x4 projectedLeft = getViewMatrix(ovrEye_Left) * getProjectionMatrix(ovrEye_Right);
    
/*
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
//        baseCamera->begin();
//        baseCamera->end();
        modelViewMatrix = modelViewMatrix.getInverse();
    
        ofVec3f cameraXYZ = worldPosition * (modelViewMatrix * projectionMatrixLeft);
        cameraXYZ.interpolate(worldPosition * (modelViewMatrix * projectionMatrixRight), 0.5f);

        ofVec3f screenXYZ((cameraXYZ.x + 1.0f) / 2.0f * viewport.width + viewport.x,
                          (1.0f - cameraXYZ.y) / 2.0f * viewport.height + viewport.y,
                          cameraXYZ.z);        
        return screenXYZ;

    }
     */
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
	multBillboardMatrix(mousePosition3D());
}

void ofxOculusDK2::multBillboardMatrix(ofVec3f objectPosition, ofVec3f updirection){

	if(baseCamera == NULL){
		return;
	}
	
	ofNode n;
	n.setPosition( objectPosition );
	n.lookAt(baseCamera->getPosition(), updirection);
	ofVec3f axis; float angle;
	n.getOrientationQuat().getRotate(angle, axis);
	// Translate the object to its position.
	ofTranslate( objectPosition );
	// Perform the rotation.
	ofRotate(angle, axis.x, axis.y, axis.z);
}
ofVec2f ofxOculusDK2::gazePosition2D(){
    ofVec3f angles = getOrientationQuat().getEuler();
	return ofVec2f(ofMap(angles.y, 90, -90, 0, ofGetWidth()),
                   ofMap(angles.z, 90, -90, 0, ofGetHeight()));
}

#ifdef SDK_RENDER
void ofxOculusDK2::draw(){
	
	if(!bSetup) return;
	
	if(!insideFrame) return;

//	glBindVertexArray(0);
 
//    ofPixels dp;
//    renderTarget.readToPixels(dp);
//    debugImage.setFromPixels(dp);
//    debugImage.saveImage("debug.png");
//    GLint err = glGetError();

    ovrHmd_EndFrame(hmd, headPose, &eyeTexture[0].Texture);
    
    bUseOverlay = false;
	bUseBackground = false;
	insideFrame = false;
}
#else
void ofxOculusDK2::draw(){
	
	if(!bSetup) return;
	
	if(!insideFrame) return;

	ovr_WaitTillTime(frameTiming.TimewarpPointSeconds);

//    ofPixels dp;
//    renderTarget.readToPixels(dp);
//    debugImage.setFromPixels(dp);
//    debugImage.saveImage("debug.png");


    /*
    ofDisableDepthTest();
    ofEnableAlphaBlending();
	debugShader.begin();
    debugShader.setUniformTexture("Texture", renderTarget.getTextureReference(), 1);
    debugShader.setUniform2f("TextureScale",
                  renderTarget.getTextureReference().getWidth(),
                  renderTarget.getTextureReference().getHeight());
    
    debugMesh.clear();
    float width = renderTargetSize.w;
    float height = renderTargetSize.h;
	//ofRectangle debugrect = ofRectangle(-width/2, -height/2,width,height);
    ofRectangle debugrect = ofRectangle(100,100, width/2,height/2);
	debugMesh.addVertex( ofVec3f(debugrect.getMinX(), debugrect.getMinY(), 100) );
	debugMesh.addVertex( ofVec3f(debugrect.getMaxX(), debugrect.getMinY(), 100) );
	debugMesh.addVertex( ofVec3f(debugrect.getMinX(), debugrect.getMaxY(), 100) );
	debugMesh.addVertex( ofVec3f(debugrect.getMaxX(), debugrect.getMaxY(), 100) );
    
	debugMesh.addTexCoord( ofVec2f(0, height ) );
	debugMesh.addTexCoord( ofVec2f(width, height) );
	debugMesh.addTexCoord( ofVec2f(0, 0) );
	debugMesh.addTexCoord( ofVec2f(width, 0) );
	
	debugMesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
    debugMesh.draw();
    
    debugShader.end();
    */
   
	///JG START HERE 
	// Prepare for distortion rendering. 
	ofDisableDepthTest();
    ofEnableAlphaBlending();
	distortionShader.begin();
	distortionShader.setUniformTexture("Texture", renderTarget.getTextureReference(), 1);
	distortionShader.setUniform2f("TextureScale", 
		renderTarget.getTextureReference().getWidth(), 
		renderTarget.getTextureReference().getHeight());

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

		eyeMesh[eyeIndex].draw();
	}
	distortionShader.end();
	
	/////////////////////
	ovrHmd_EndFrameTiming(hmd);
    

	ofEnableDepthTest();

	bUseOverlay = false;
	bUseBackground = false;
	insideFrame = false;
}
#endif

void ofxOculusDK2::setUsePredictedOrientation(bool usePredicted){
	bUsePredictedOrientation = usePredicted;
}
bool ofxOculusDK2::getUsePredictiveOrientation(){
	return bUsePredictedOrientation;
}

bool ofxOculusDK2::isHD(){
	if(bSetup){
		return hmd->Type == ovrHmd_DK2 || hmd->Type == ovrHmd_DKHD;
	}
	return false;
}
