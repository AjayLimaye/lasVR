#version 420 core

in vec3 fragmentColor;
in vec3 pointPos;
in float zdepth;
in float zdepthR;

// Ouput data
layout(location=0) out vec4 color;

layout(location=1) out vec3 depth;

layout (depth_greater) out float gl_FragDepth;


uniform vec3 eyepos;
uniform vec3 viewdir;
uniform bool shadows;

void main()
{
  if (length(fragmentColor) <= 0)
    discard;

//  float dc = length(2.0 * gl_PointCoord - 1.0);
//  if (dc > 1.0) discard;

  vec2 pc = 2.0*gl_PointCoord - 1.0;
  float dc = (pc.x*pc.x + pc.y*pc.y);  
  if (dc > 1.0) discard;

  float alpha = pow(1.0-dc, 0.25);
  color = vec4(fragmentColor, alpha);


  gl_FragDepth = zdepth - zdepthR*dc;


//  color = vec4(fragmentColor, 1);
//  gl_FragDepth = zdepth;

  if (shadows)
    {
      float d = dot((pointPos-eyepos), viewdir);
      depth = vec3(d,zdepthR,gl_FragDepth);
    }
}
