#version 410 core
uniform sampler2D diffuse;
uniform vec3 color;
uniform vec3 viewDir;
uniform float opmod;
uniform int applytexture;
uniform float mixcolor;

in vec2 v2TexCoord;
in vec3 v3Normal;
out vec4 outputColor;
void main()
{
  if (applytexture == 0) // points
    {
      float dc = length(2.0 * gl_PointCoord - 1.0);
      if (dc > 1.0) discard;

      outputColor = vec4(color,1.0);
    }
  else if (applytexture == 1) // lines
    outputColor = vec4(color,1.0);
  else if (applytexture == 2)
    outputColor = texture( diffuse, v2TexCoord);
  else if (applytexture == 3) // for point sprites
    outputColor = texture(diffuse, gl_PointCoord);
  else if (applytexture == 4) // for textured lines
    outputColor = vec4(color*v2TexCoord.x,v2TexCoord.x);
  else if (applytexture == 5) // for texture + color
    outputColor = texture( diffuse, v2TexCoord);


  if (outputColor.a < 0.001)
    discard;

  if (length(color) > 0)
    outputColor.rgb = mix(outputColor.rgb,
			  color*outputColor.a,
			  mixcolor);      


  if (length(viewDir) > 0.0)
    {
      float edge = abs(dot(v3Normal, viewDir));
      outputColor.rgb += color*edge;
    }
  else
    {
      outputColor *= opmod;
    }
}
