#include "testApp.h"

ovrHmd hmd;
GLuint frameBuffer;
GLuint MVPMatrixLocation;
GLuint positionLocation;
ovrEyeRenderDesc eyeRenderDesc[2];
ovrRecti eyeRenderViewport[2];
GLuint vertexShader;
GLuint vertexArray;
ovrGLTexture eyeTexture[2];

//--------------------------------------------------------------
void testApp::setup()
{
 
        //int x = SDL_WINDOWPOS_CENTERED;
        //int y = SDL_WINDOWPOS_CENTERED;
       // Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
 
   
 
    // SDL_Window *window = SDL_CreateWindow("Oculus Rift SDL2 OpenGL Demo", x, y, w, h, flags);
    //SDL_GLContext context = SDL_GL_CreateContext(window); 
    //glewExperimental = GL_TRUE;
    //glewInit();
 
    Sizei recommendedTex0Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Left, hmd->DefaultEyeFov[0], 1.0f);
    Sizei recommendedTex1Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Right, hmd->DefaultEyeFov[1], 1.0f);
    Sizei renderTargetSize;
    renderTargetSize.w = recommendedTex0Size.w + recommendedTex1Size.w;
    renderTargetSize.h = max(recommendedTex0Size.h, recommendedTex1Size.h);
 
    glGenFramebuffers(1, &frameBuffer);
 
    GLuint texture;
    glGenTextures(1, &texture);
 
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, renderTargetSize.w, renderTargetSize.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
 
    GLuint renderBuffer;
    glGenRenderbuffers(1, &renderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, renderTargetSize.w, renderTargetSize.h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBuffer);
 
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
		/*
            glDeleteFramebuffers(1, &frameBuffer);
            glDeleteTextures(1, &texture);
            glDeleteRenderbuffers(1, &renderBuffer);
 
            SDL_GL_DeleteContext(context);
 
            SDL_DestroyWindow(window);
 
            ovrHmd_Destroy(hmd);
 
            ovr_Shutdown();
 
            SDL_Quit();
		*/
		ofLogError("Framebuffer failed!");
//                return 0;
    }
 
    ovrFovPort eyeFov[2] = { hmd->DefaultEyeFov[0], hmd->DefaultEyeFov[1] };
 

    eyeRenderViewport[0].Pos = Vector2i(0, 0);
    eyeRenderViewport[0].Size = Sizei(renderTargetSize.w / 2, renderTargetSize.h);
    eyeRenderViewport[1].Pos = Vector2i((renderTargetSize.w + 1) / 2, 0);
    eyeRenderViewport[1].Size = eyeRenderViewport[0].Size;
 

    eyeTexture[0].OGL.Header.API = ovrRenderAPI_OpenGL;
    eyeTexture[0].OGL.Header.TextureSize = renderTargetSize;
    eyeTexture[0].OGL.Header.RenderViewport = eyeRenderViewport[0];
    eyeTexture[0].OGL.TexId = texture;
 
    eyeTexture[1] = eyeTexture[0];
    eyeTexture[1].OGL.Header.RenderViewport = eyeRenderViewport[1];
 
	/*
    SDL_SysWMinfo info;
 
    SDL_VERSION(&info.version);
 
    SDL_GetWindowWMInfo(window, &info);
*/
    ovrGLConfig cfg;
    cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
    cfg.OGL.Header.RTSize = Sizei(hmd->Resolution.w, hmd->Resolution.h);
    cfg.OGL.Header.Multisample = 1;
    if (!(hmd->HmdCaps & ovrHmdCap_ExtendDesktop))
            ovrHmd_AttachToWindow(hmd, ofGetWin32Window(), NULL, NULL);
 
	cfg.OGL.Window = ofGetWin32Window();
    cfg.OGL.DC = NULL;
 
 
    ovrHmd_ConfigureRendering(hmd, &cfg.Config, ovrDistortionCap_Chromatic | ovrDistortionCap_Vignette | ovrDistortionCap_TimeWarp | ovrDistortionCap_Overdrive, eyeFov, eyeRenderDesc);
 
    ovrHmd_SetEnabledCaps(hmd, ovrHmdCap_LowPersistence | ovrHmdCap_DynamicPrediction);
 
    ovrHmd_ConfigureTracking(hmd, ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position, 0);
 
    const GLchar *vertexShaderSource[] = {
            "#version 150\n"
            "uniform mat4 MVPMatrix;\n"
            "in vec3 position;\n"
            "void main()\n"
            "{\n"
            "    gl_Position = MVPMatrix * vec4(position, 1.0);\n"
            "}"
    };
 
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
    glCompileShader(vertexShader);
 
    const GLchar *fragmentShaderSource[] = {
            "#version 150\n"
            "out vec4 outputColor;\n"
            "void main()\n"
            "{\n"
            "    outputColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
            "}"
    };
 
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
 
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glUseProgram(program);
 
    MVPMatrixLocation = glGetUniformLocation(program, "MVPMatrix");
    positionLocation = glGetAttribLocation(program, "position");
 

    glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);
 
    GLfloat vertices[] = {
            0.0f, 1.0f, -2.0f,
            -1.0f, -1.0f, -2.0f,
            1.0f, -1.0f, -2.0f
    };
 
    GLuint positionBuffer;
    glGenBuffers(1, &positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(positionLocation);
 
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClearDepth(1.0f);
 
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
 
    bool running = true;
 
}


//--------------------------------------------------------------
void testApp::update()
{

}

//--------------------------------------------------------------
void testApp::draw()
{

	
	ovrFrameTiming frameTiming = ovrHmd_BeginFrame(hmd, 0);
 
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 
	ovrPosef eyeRenderPose[2];
 
	for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
	{
			ovrEyeType eye = hmd->EyeRenderOrder[eyeIndex];
			eyeRenderPose[eye] = ovrHmd_GetHmdPosePerEye(hmd, eye);
 
			Matrix4f MVPMatrix = Matrix4f(ovrMatrix4f_Projection(eyeRenderDesc[eye].Fov, 0.01f, 10000.0f, true)) * Matrix4f::Translation(eyeRenderDesc[eye].HmdToEyeViewOffset) * Matrix4f(Quatf(eyeRenderPose[eye].Orientation).Inverted());
 
			glUniformMatrix4fv(MVPMatrixLocation, 1, GL_FALSE, &MVPMatrix.Transposed().M[0][0]);
 
			glViewport(eyeRenderViewport[eye].Pos.x, eyeRenderViewport[eye].Pos.y, eyeRenderViewport[eye].Size.w, eyeRenderViewport[eye].Size.h);
 
			glDrawArrays(GL_TRIANGLES, 0, 3);
	}
 
	glBindVertexArray(0);
 
	ovrHmd_EndFrame(hmd, eyeRenderPose, &eyeTexture[0].Texture);
 
	glBindVertexArray(vertexArray);
 
}

//--------------------------------------------------------------
void testApp::drawScene()
{
	
	ofPushMatrix();
	ofRotate(90, 0, 0, -1);
	ofDrawGridPlane(500.0f, 10.0f, false );
	ofPopMatrix();
	
	ofPushStyle();
	ofNoFill();
	for(int i = 0; i < demos.size(); i++){
		ofPushMatrix();
//		ofRotate(ofGetElapsedTimef()*(50-demos[i].radius), 0, 1, 0);
		ofTranslate(demos[i].floatPos);
//		ofRotate(ofGetElapsedTimef()*4*(50-demos[i].radius), 0, 1, 0);

        if (demos[i].bMouseOver)
            ofSetColor(ofColor::white.getLerped(ofColor::red, sin(ofGetElapsedTimef()*10.0)*.5+.5));
        else if (demos[i].bGazeOver)
            ofSetColor(ofColor::white.getLerped(ofColor::green, sin(ofGetElapsedTimef()*10.0)*.5+.5));
        else
            ofSetColor(demos[i].color);

		ofSphere(demos[i].radius);
		ofPopMatrix();
	}
    
	
	
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
