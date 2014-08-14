#version 120
#extension GL_ARB_texture_rectangle : enable

uniform vec2 LensCenter;
uniform vec2 ScreenCenter;
uniform vec2 Scale;
uniform vec2 ScaleIn;
uniform vec4 HmdWarpParam;
uniform vec4 ChromAbParam;
uniform sampler2DRect Texture0;
uniform vec2 dimensions;
varying vec2 oTexCoord;

void main()
{
  vec2  theta = (oTexCoord - LensCenter) * ScaleIn; // Scales to [-1, 1]
  float rSq = theta.x * theta.x + theta.y * theta.y;
    vec2  theta1 = theta * (HmdWarpParam.x +
                            HmdWarpParam.y * rSq +
              HmdWarpParam.z * rSq * rSq +
                            HmdWarpParam.w * rSq * rSq * rSq);
    
    // Detect whether blue texture coordinates are out of range
    // since these will scale out the furthest.
    vec2 thetaBlue = theta1 * (ChromAbParam.z + ChromAbParam.w * rSq);
    vec2 tcBlue = LensCenter + Scale * thetaBlue;
    if (!all(equal(clamp(tcBlue, ScreenCenter - vec2(0.25, 0.5), ScreenCenter + vec2(0.25, 0.5)), tcBlue))) {
        gl_FragColor = vec4(0);
    }
    else {
        // Now do blue texture lookup.
        float blue = texture2DRect(Texture0, tcBlue * dimensions).b;
        
        // Do green lookup (no scaling).
        vec2 tcGreen = LensCenter + Scale * theta1;
        vec4 center = texture2DRect(Texture0, tcGreen * dimensions);
        
        // Do red scale and lookup.
        vec2 thetaRed = theta1 * (ChromAbParam.x + ChromAbParam.y * rSq);
        vec2 tcRed = LensCenter + Scale * thetaRed;
        float red = texture2DRect(Texture0, tcRed * dimensions).r;
        
        gl_FragColor = vec4(red, center.g, blue, center.a) * gl_Color;
    }
}
