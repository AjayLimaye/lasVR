#version 420 core
 
// Ouput data
layout(location=0) out vec4 color;
layout(depth_greater) out float gl_FragDepth;
		      
uniform sampler2DRect colorTex;
uniform sampler2DRect depthTex;

uniform bool showedges;
uniform bool softShadows;

uniform float nearDist;
uniform float farDist;

uniform bool showsphere;

void main()
{

  vec2 spos = gl_FragCoord.xy;

  color = texture2DRect(colorTex, spos.xy);

  vec4 dtex = texture2DRect(depthTex, spos.xy);
  float depth = dtex.x;
  gl_FragDepth = dtex.z;

  if (depth < 0.001) 
    {
      gl_FragDepth = 1.0;
      return;
    }

  float cx[8] = float[](-1.0, 0.0, 1.0, 0.0, -1.0,-1.0, 1.0, 1.0);
  float cy[8] = float[]( 0.0,-1.0, 0.0, 1.0, -1.0, 1.0,-1.0, 1.0);


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

//  //------------------------
//  // apply color smoothing
//  {
//    float tele = 4.0;
//    vec4 colorsum = tele*color;
//    for(int i=0; i<8; i++)
//      {
//	float r = 1.0 + float(i)/8.0;
//	vec2 pos = spos + vec2(r*cx[int(mod(i,8))],r*cy[int(mod(i,8))]);
//	vec3 dff = texture2DRect(depthTex, pos).xyz;
//	float stp = step(dff.z, dtex.z-dff.y);
//	colorsum += stp*texture2DRect(colorTex, pos);
//	tele += stp;
//      } 
//    color = colorsum/tele;
//
//    //color.rgb = mix(color.rgb*color.a, color.rgb, smoothstep(0.0, 0.5, log2(depth)));
//
//    if (showsphere)
//      color.rgb *= color.a;
//							     
//    color.a = 1.0;
//  }
//  //------------------------

  float nearD1 = nearDist;
  float farD1 = nearDist+(farDist-nearDist)*0.3;
  if (softShadows && dtex.x < farD1)
   {
    float stpfrc = smoothstep(nearD1, farD1, dtex.x);
    int nsteps = int(200.0*max(0.1, 1.0-stpfrc));

    float sum = 0.0;
    float tele = 0.0;
    float r = 1.0;
    float theta = 0.0;
    int cnt = 4;
    float ege = 0.0;
    int j = 0;
    for(int i=0; i<nsteps; i++)
    {
      r = 1.0 + float(i)/4.0;
      vec2 pos = spos + vec2(r*cx[int(mod(i,8))],r*cy[int(mod(i,8))]);
      float od = depth - texture2DRect(depthTex, pos).x;
      float ege = r*0.005;
      sum += step(ege, od);
      tele ++;
    } 
    sum /= tele;
    sum = 1.0-sum;
    sum = pow(sum, 0.5);
    color.rgb = mix(color.rgb*sum, color.rgb, smoothstep(nearD1, farD1, dtex.x));
   }

  float nearD2 = nearDist+(farDist-nearDist)*0.1;
  float farD2 = nearDist+(farDist-nearDist)*0.5;
  if (showedges && dtex.x < farD2)
   {
    float response = 0.0;
    float tele = 0.0;
    //float dty1 = pow(dtex.z, 0.5);
    float dty1 = dtex.z*dtex.z;
    float shadow = 0.5*dty1;
    depth = log2(depth);
    for(int i=0; i<8; i++)
    {
      vec2 pos = spos + vec2(cx[i],cy[i]);

      float od = depth - log2(texture2DRect(depthTex, pos).x);
      response += max(0.0, od-0.05);

      //vec4 p = texture2DRect(depthTex, pos);
      //float od = dtex.z - (p.z-0.1*p.y);
      //response += max(0.0, od);

      tele ++;
    } 
    response /= tele;
    shadow = exp(-response*10*shadow);
    color.rgb = mix(color.rgb*shadow, color.rgb, smoothstep(nearD2, farD2, dtex.x));

    //float shadow = exp(-response*300);
    //color.rgb = mix(color.rgb*shadow, color.rgb, smoothstep(nearD2, farD2, dtex.x));
   }
}
