#version 120
#extension GL_ARB_texture_rectangle : enable

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