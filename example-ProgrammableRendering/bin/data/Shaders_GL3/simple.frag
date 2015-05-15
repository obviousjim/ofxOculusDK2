#version 330

uniform mat4 modelViewProjectionMatrix; // automatically imported by OF
uniform mat4 modelViewMatrix; // automatically imported by OF
uniform mat4 normalMatrix; // the normal matrix (the inversed-then-transposed modelView matrix)
uniform mat4 textureMatrix;
uniform vec3 color;

//uniform sampler2D tex0;
uniform sampler2DRect src_tex_unit0;

in vec2 texCoordVarying;

out vec4 fragColor;

void main()
{
  fragColor = texture(src_tex_unit0, texCoordVarying) * vec4(color,1.0);

}
