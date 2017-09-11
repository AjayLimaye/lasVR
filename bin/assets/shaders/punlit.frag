#version 410

in vec3 fragmentColor;

out vec4 fragColor;

uniform int gltype;

void main()
{
//  if (gltype == 0) // GL_POINTS
//    {
//      float dc = length(2.0 * gl_PointCoord - 1.0);
//      if (dc > 1.0) discard;
//    }

  fragColor = vec4(fragmentColor, 1.0);
}
