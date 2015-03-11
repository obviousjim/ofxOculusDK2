#version 330

uniform sampler2DRect Texture;
uniform vec2 textureScale;

in vec4 eyeSpaceVertexPos, ambientGlobal;
in vec3 vertex_normal, interp_eyePos;

in vec2 texCoordVarying;




out vec4 fragColor;

void main()
{
  //vec3 n;

  //fragColor = ambientGlobal;
  /* a fragment shader can't write an in variable, hence we need
     a new variable to store the normalized interpolated normal */
  //n = normalize(vertex_normal);
  //fragColor += calc_lighting_color(n);
  //fragColor += dot(n, normalize(vec3(0,100,0)- eyeSpaceVertexPos)) * vec3(.8,.8,0);

  // fragColor.r = texture2DRect(Texture, texcoord).r;

  
  //fragColor = texture(Texture, texcoord * textureScale);
  fragColor = texture(Texture, texCoordVarying);
  //fragColor.b += 0.085;
  //fragColor += vec4(texCoordVarying.x/textureScale.x, texCoordVarying.y/textureScale.y,0,0) * 0.15;
  fragColor.a = 1.0;

}
