#version 410

uniform mat4 MVP;
uniform vec3 hmdPos;
uniform float scale;

layout (location = 0) in vec3 aPos;

out vec3 TexCoord;

void main()
{
    TexCoord = aPos;
    vec4 pos = MVP * vec4(scale*aPos.xzy + hmdPos, 1.0);
    gl_Position = pos.xyww;
} 
