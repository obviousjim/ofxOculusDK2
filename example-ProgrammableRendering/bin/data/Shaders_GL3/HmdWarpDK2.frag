#version 150

uniform sampler2D Texture;
uniform vec2 TextureScale;
    
in vec4 oColor;
in vec2 oTexCoord0;
in vec2 oTexCoord1;
in vec2 oTexCoord2;

out vec4 fragColor;

void main()
{
   fragColor.r = oColor.r * texture(Texture, oTexCoord0).r;
   fragColor.g = oColor.g * texture(Texture, oTexCoord1).g;
   fragColor.b = oColor.b * texture(Texture, oTexCoord2).b;

   fragColor.a = 1.0;
}
