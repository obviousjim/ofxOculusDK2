#version 330

uniform mat4 modelViewProjectionMatrix; // automatically imported by OF
uniform mat4 modelViewMatrix; // automatically imported by OF
uniform mat4 normalMatrix; // the normal matrix (the inversed-then-transposed modelView matrix)
uniform int lightsNumber;

in vec4 eyeSpaceVertexPos, ambientGlobal;
in vec3 vertex_normal, interp_eyePos;

out vec4 fragColor;

void main()
{
  vec3 n;

  fragColor = ambientGlobal;
  /* a fragment shader can't write an in variable, hence we need
     a new variable to store the normalized interpolated normal */
  n = normalize(vertex_normal);

  fragColor += dot(n, normalize(vec3(0,100,0)- eyeSpaceVertexPos)) * vec3(.8,.8,0);
  fragColor.w = 1.0;
}
