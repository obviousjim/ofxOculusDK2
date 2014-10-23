#version 150

// #extension GL_ARB_texture_rectangle : enable

uniform vec2 EyeToSourceUVScale;
uniform vec2 EyeToSourceUVOffset;
uniform mat4 EyeRotationStart;
uniform mat4 EyeRotationEnd;

in vec4 position;
in vec4 color;
in vec3 normal;
// in vec2 TexCoord0;
// in vec2 TexCoord1;
// in vec2 TexCoord2;

out vec4 oColor;
out vec2 oTexCoord0;
out vec2 oTexCoord1;
out vec2 oTexCoord2;

// varying vec4 oColor;
// varying vec2 oTexCoord0;
// varying vec2 oTexCoord1;
// varying vec2 oTexCoord2;

void main()
{
	// gl_Position.x = gl_Vertex.x;
	// gl_Position.y = gl_Vertex.y;
	gl_Position.x = position.x;
    gl_Position.y = position.y;
	gl_Position.z = .0;
	gl_Position.w = 1.0;

	// Vertex inputs are in TanEyeAngle space for the R,G,B channels (i.e. after chromatic aberration and distortion).
	// These are now "real world" vectors in direction (x,y,1) relative to the eye of the HMD.
	vec3 TanEyeAngleR = vec3 ( normal.x, normal.y, 1.0 );
	vec3 TanEyeAngleG = vec3 ( color.r, color.g, 1.0 );
	vec3 TanEyeAngleB = vec3 ( color.b, color.a, 1.0 );
	// vec3 TanEyeAngleR = vec3 ( TexCoord0.x, TexCoord0.y, 1.0 );
	// vec3 TanEyeAngleG = vec3 ( TexCoord1.x, TexCoord1.y, 1.0 );
	// vec3 TanEyeAngleB = vec3 ( TexCoord2.x, TexCoord2.y, 1.0 );

	mat3 EyeRotation;
	EyeRotation[0] = mix ( EyeRotationStart[0], EyeRotationEnd[0], position.z ).xyz;
	EyeRotation[1] = mix ( EyeRotationStart[1], EyeRotationEnd[1], position.z ).xyz;
	EyeRotation[2] = mix ( EyeRotationStart[2], EyeRotationEnd[2], position.z ).xyz;
	
    // EyeRotation[0] = mix ( EyeRotationStart[0], EyeRotationEnd[0], Color.a ).xyz;
    // EyeRotation[1] = mix ( EyeRotationStart[1], EyeRotationEnd[1], Color.a ).xyz;
    // EyeRotation[2] = mix ( EyeRotationStart[2], EyeRotationEnd[2], Color.a ).xyz;

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

	// oTexCoord0 = SrcCoordR;
 //    oTexCoord1 = SrcCoordG;
 //    oTexCoord2 = SrcCoordB;
    
 	oColor = vec4(normal.z, normal.z, normal.z, normal.z);
    //oColor = vec4(Color.r, Color.r, Color.r, Color.r);              // Used for vignette fade.

}