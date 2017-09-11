#version 410

uniform samplerCube skybox;

in vec3 TexCoord;

out vec4 gl_FragColor;

void main()
{    
  gl_FragColor = texture(skybox, TexCoord);
}
