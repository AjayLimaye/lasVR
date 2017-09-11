#version 420 core

layout(location = 0) in vec3 vertex;
uniform mat4 MVP;

void main()
{
  gl_Position =  MVP * vec4(vertex,1);
}
