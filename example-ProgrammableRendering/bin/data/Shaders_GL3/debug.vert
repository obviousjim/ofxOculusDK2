#version 330

uniform mat4 modelViewProjectionMatrix; // automatically imported by OF
uniform mat4 modelViewMatrix; // automatically imported by OF
uniform mat4 normalMatrix; // the normal matrix (the inversed-then-transposed modelView matrix)
uniform mat4 textureMatrix;

in vec4 position; // in local space
in vec3 normal; // in local space
in vec3 texcoord;

out vec4 eyeSpaceVertexPos, ambientGlobal;
out vec3 vertex_normal, interp_eyePos;
out vec2 texCoordVarying;

void main() {
  // ambientGlobal = material.emission; // no global lighting for the moment
  // eyeSpaceVertexPos = modelViewMatrix * position;
  // vertex_normal = normalize((normalMatrix * vec4(normal, 0.0)).xyz);
  // interp_eyePos = vec3(-eyeSpaceVertexPos);

  texCoordVarying = (textureMatrix*vec4(texcoord.x,texcoord.y,0,1)).xy;

  gl_Position = modelViewProjectionMatrix * position;
}
