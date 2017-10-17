#version 420 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec4 vertexColor;

// Output data will be interpolated for each fragment.
out vec3 fragmentColor;
out vec3 pointPos;
out float zdepth;    
out float zdepthR;    

uniform mat4 MV;
uniform mat4 MVP;

uniform float pointSize;

uniform float scaleFactor;

uniform sampler1D colorTex;

uniform vec3 eyepos;
uniform vec3 viewDir;
uniform int screenWidth;

uniform float projFactor;

uniform sampler2DRect ommTex;
uniform sampler2DRect visTex;

uniform bool pointType;

uniform float zFar;

uniform float minPointSize;
uniform float maxPointSize;

uniform float deadRadius;
uniform vec3 deadPoint;

uniform bool applyXform;
uniform int xformTileId;
uniform vec3 xformShift;
uniform vec3 xformCen;
uniform float xformScale;
uniform vec4 xformRot;

//-----------------------
float numberOfOnes(float number, float index)
{
   float tmp = mod(number, pow(2.0, index + 1.0));
   float numOnes = 0.0;
   for (float i = 0.0; i < 8.0; i++)
   {;
      if (mod(tmp, 2.0) != 0.0)
         numOnes++;
      tmp = floor(tmp / 2.0);
    };
    return numOnes;
};
//-----------------------

//-----------------------
bool isBitSet(float number, float index)
{
  return mod(floor(number / pow(2.0, index)), 2.0) != 0.0;
}
//-----------------------

//-----------------------
float getLOD(float tile, vec3 omin, vec3 omax, vec3 opos)
{

  if (any(lessThan(opos, omin)) || any(greaterThan(opos, omax)))
    return 10.0;

  float depth = 0.0;
  vec3 osize = omax-omin;
  vec3 pos = opos-omin;
  vec3 offset = vec3(0.0);
  float ioffset = 0.0;

  vec4 valueTop = texture2DRect(visTex, vec2(ioffset,tile)).rgba;

  for(int i=0; i<20; i++) // assuming we do not have more than 20 levels
  {
    vec3 nsal = osize/pow(2.0, float(i)); // node size at this octree level
    vec3 idx3d = (pos-offset)/nsal;       // which of the 8 octree node children

//  if (any(lessThan(idx3d, vec3(0.0)) ||
//                   greaterThan(idx3d, vec3(1.0))))
//    return 10.0;    
//  idx3d = clamp(idx3d, vec3(0.0), vec3(1.0));

    idx3d = floor(idx3d+vec3(0.5));      // does the point lie in
    float index = 4.0*idx3d.x + 2.0*idx3d.y + idx3d.z; // node position
    vec4 value = texture2DRect(visTex, vec2(ioffset,tile)).rgba;
    float mask = value.r*255.0;  // get visible child nodes
    if (isBitSet(mask,index)) // visible node at this position
    {
      ioffset = ioffset + value.g * 255.0 + numberOfOnes(mask, index-1.0);
      depth ++;
    }
    else // no mode visible child nodes at this position
    {
      if (value.a > 0.0)
        depth = max(depth, value.b*255.0);
      return depth;
    }

    offset = offset + (nsal*0.5) * idx3d;
  }

 return depth;
}
//-----------------------

vec3
qtransform(vec4 q, vec3 v)
{ 
  //return v + 2.0*cross(cross(v, q.xyz ) + q.w*v, q.xyz);
  return v + 2.0*cross(q.xyz, cross(q.xyz,v) + q.w*v);
}

void main()
{
   // Output position of the vertex, in clip space : MVP * position

   pointPos = vertexPosition.xyz;


   //----------------------------------
   // transform point during manual registration
   if (applyXform && vertexColor.a >= xformTileId)
     {
       pointPos -= xformCen;

       //rotate
       pointPos = qtransform(xformRot, pointPos);
       //scale
       pointPos *= xformScale;

       pointPos += xformCen;

       //translate
       pointPos += xformShift;
     }
   //----------------------------------



   //----------------------------------
   //--------------------------------
   //--------------------------------
   float dpvp = length(deadPoint.xy-vertexPosition.xy);
   bool killPoint = ((deadRadius > 0.0) &&
		     (dpvp < deadRadius)); 
   if (killPoint)
     { // point cloud distrotion to show underlying data
       float dfrc = smoothstep(deadRadius*0.9, deadRadius, dpvp);
       pointPos.z *= (0.95 + 0.05*dfrc); // bring z down by half the value
     }
   //--------------------------------
   //--------------------------------
   //----------------------------------

   gl_Position =  MVP * vec4(pointPos,1);

   zdepth = ((gl_DepthRange.diff * gl_Position.z/gl_Position.w) +
              gl_DepthRange.near + gl_DepthRange.far) / 2.0;
   zdepthR = 0;


   // -----------------------------------
   // logarithmic depth buffer
   // https://outerra.blogspot.com.au/2009/08/logarithmic-z-buffer.html
   //float C = 0.001;
   //zdepth = (2.0*log(C*zdepth+1.0))/(log(C*zFar+1.0)-1.0)*gl_Position.w;
   // -----------------------------------


  // pointsize
  //----------------------------------
   gl_PointSize = 1.0;

   if (pointType) // adaptive pointsize
     {
       float tile = vertexColor.a;
       vec4 omins = texture2DRect(ommTex, vec2(0.0,tile));
       vec4 omaxs = texture2DRect(ommTex, vec2(1.0,tile));
       float spacing = omins.a;
       vec3 omin = omins.rgb;
       vec3 omax = omaxs.rgb;
       
       float lod = getLOD(tile, omin, omax, pointPos);
       
       float r = spacing * scaleFactor;
       
       float worldSize = pointSize*r/pow(1.9, lod);
       
       float viewZ = -(MV * vec4(pointPos,1)).z;
       gl_PointSize = worldSize*projFactor/viewZ; 
       
       
       // -----------------------------------
       // depth with radius
       if (maxPointSize > 5) // means this is not for depth calculation
	 {
	   vec3 e2p = normalize(pointPos-eyepos);
	   //vec4 posR =  MVP * vec4(pointPos+0.5*e2p*worldSize,1);
	   vec4 posR =  MVP * vec4(pointPos+e2p*r,1);
	   zdepthR = ((gl_DepthRange.diff * posR.z/posR.w) +
		      gl_DepthRange.near + gl_DepthRange.far) / 2.0;
	   zdepthR = zdepth-zdepthR;
	 }
       
       // -----------------------------------
     }
   else // fixed pointsize
     {
      gl_PointSize = pointSize * scaleFactor;
     }


   //----------------------------------
   gl_PointSize = clamp(gl_PointSize, minPointSize, maxPointSize);
   //----------------------------------


   //----------------------------------
   fragmentColor = vertexColor.rgb/vec3(255.0,255.0,255.0);
   //----------------------------------
}
