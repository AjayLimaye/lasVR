#version 420 core
 
// Ouput data
layout(location=0) out vec4 color;
layout(location=1) out vec3 depth;
layout(depth_greater) out float gl_FragDepth;
		      
uniform sampler2DRect colorTex;
uniform sampler2DRect depthTex;

void main()
{

  vec2 spos = gl_FragCoord.xy;

  color = texture2DRect(colorTex, spos.xy);
  vec4 dtex = texture2DRect(depthTex, spos.xy);
  depth = dtex.xyz;
  gl_FragDepth = dtex.z;

  if (dtex.x < 0.001) 
    return;

  //------------------------
  // apply color smoothing
  vec4 col[50];
  float tdst = 0;
  int idx=0;
  for(int i=-2; i<=2; i++)
    for(int j=-2; j<=2; j++)
      {
	float dc = max(0.2, length(vec2(i,j)));
	float stp = 1.0-step(2.0, dc);
	vec2 pos = spos + vec2(i,j);
	float frc = stp*1.0/dc;
	col[idx] = frc*texture2DRect(colorTex, pos);
	tdst += frc;
	idx++;
      }

  vec4 fcol = vec4(0,0,0,0);
  for(int i=0; i<idx; i++)
    fcol += col[i];
  color = fcol/tdst;
}
