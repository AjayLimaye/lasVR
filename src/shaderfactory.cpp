#include "shaderfactory.h"

#include <QTextEdit>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFile>

int
ShaderFactory::loadShaderFromFile(GLhandleARB obj, QString filename)
{
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return 0;

  QByteArray lines = file.readAll();

  int len = lines.count(); 
  char *source = new char[len+1];
  sprintf(source, lines.constData());
 
  const char *sstr = source;
  glShaderSourceARB(obj, 1, &sstr, NULL);

  delete [] source;

  return 1;
}


bool
ShaderFactory::loadShader(GLhandleARB &progObj,
			  QString vertShaderString,
			  QString fragShaderString)
{
  GLhandleARB fragObj = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);  
  glAttachObjectARB(progObj, fragObj);

  GLhandleARB vertObj = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);  
  glAttachObjectARB(progObj, vertObj);

  {  // vertObj   
    int len = vertShaderString.length();
    char *tbuffer = new char[len+1];
    sprintf(tbuffer, vertShaderString.toLatin1().data());
    const char *sstr = tbuffer;
    glShaderSourceARB(vertObj, 1, &sstr, NULL);
    delete [] tbuffer;

    GLint compiled = -1;
    glCompileShaderARB(vertObj);
    glGetObjectParameterivARB(vertObj,
			      GL_OBJECT_COMPILE_STATUS_ARB,
			      &compiled);
    if (!compiled)
      {
	GLcharARB str[1000];
	GLsizei len;
	glGetInfoLogARB(vertObj,
			(GLsizei) 1000,
			&len,
			str);
	
	QString estr;
	QStringList slist = vertShaderString.split("\n");
	for(int i=0; i<slist.count(); i++)
	  estr += QString("%1 : %2\n").arg(i+1).arg(slist[i]);
	
	QTextEdit *tedit = new QTextEdit();
	tedit->insertPlainText("-------------Error----------------\n\n");
	tedit->insertPlainText(str);
	tedit->insertPlainText("\n-----------Shader---------------\n\n");
	tedit->insertPlainText(estr);
	
	QVBoxLayout *layout = new QVBoxLayout();
	layout->addWidget(tedit);
	
	QDialog *showError = new QDialog();
	showError->setWindowTitle("Error in Vertex Shader");
	showError->setSizeGripEnabled(true);
	showError->setModal(true);
	showError->setLayout(layout);
	showError->exec();
	return false;
    }
  }
    
    
  { // fragObj
    int len = fragShaderString.length();
    char *tbuffer = new char[len+1];
    sprintf(tbuffer, fragShaderString.toLatin1().data());
    const char *sstr = tbuffer;
    glShaderSourceARB(fragObj, 1, &sstr, NULL);
    delete [] tbuffer;
  
    GLint compiled = -1;
    glCompileShaderARB(fragObj);
    glGetObjectParameterivARB(fragObj,
			      GL_OBJECT_COMPILE_STATUS_ARB,
			      &compiled);
	
    if (!compiled)
      {
	GLcharARB str[1000];
	GLsizei len;
	glGetInfoLogARB(fragObj,
			(GLsizei) 1000,
			&len,
			str);
	
	//-----------------------------------
	// display error
	
	//qApp->beep();
	
	QString estr;
	QStringList slist = fragShaderString.split("\n");
	for(int i=0; i<slist.count(); i++)
	  estr += QString("%1 : %2\n").arg(i+1).arg(slist[i]);
	
	QTextEdit *tedit = new QTextEdit();
	tedit->insertPlainText("-------------Error----------------\n\n");
	tedit->insertPlainText(str);
	tedit->insertPlainText("\n-----------Shader---------------\n\n");
	tedit->insertPlainText(estr);
	
	QVBoxLayout *layout = new QVBoxLayout();
	layout->addWidget(tedit);
	
	QDialog *showError = new QDialog();
	showError->setWindowTitle("Error in Fragment Shader");
	showError->setSizeGripEnabled(true);
	showError->setModal(true);
	showError->setLayout(layout);
	showError->exec();
	//-----------------------------------
	
	return false;
      }
  }

  
  //----------- link program shader ----------------------
  GLint linked = -1;
  glLinkProgramARB(progObj);
  glGetObjectParameterivARB(progObj, GL_OBJECT_LINK_STATUS_ARB, &linked);
  if (!linked)
    {
      GLcharARB str[1000];
      GLsizei len;
      QMessageBox::information(0,
			       "ProgObj",
			       "error linking texProgObj");
      glGetInfoLogARB(progObj,
		      (GLsizei) 1000,
		      &len,
		      str);
      QMessageBox::information(0,
			       "Error",
			       QString("%1\n%2").arg(len).arg(str));
      return false;
    }

  glDeleteObjectARB(fragObj);
  glDeleteObjectARB(vertObj);

  return true;
}

bool
ShaderFactory::loadShadersFromFile(GLhandleARB &progObj,
				   QString vertShader,
				   QString fragShader)
{
  QFile vfile(vertShader);
  if (!vfile.open(QIODevice::ReadOnly | QIODevice::Text))
    return false;
  QString vertShaderString = QString::fromLatin1(vfile.readAll());


  QFile ffile(fragShader);
  if (!ffile.open(QIODevice::ReadOnly | QIODevice::Text))
    return false;
  QString fragShaderString = QString::fromLatin1(ffile.readAll());
  
  return loadShader(progObj,
		    vertShaderString,
		    fragShaderString);  
}

bool
ShaderFactory::loadPointShader(GLhandleARB &progObj,
			       QString shaderString,
			       int dpv)
{
  GLhandleARB fragObj = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);  
  glAttachObjectARB(progObj, fragObj);

  GLhandleARB vertObj = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);  
  glAttachObjectARB(progObj, vertObj);

  {  // vertObj
    QString qstr;
    qstr += "#version 330 core\n";
    qstr += "// Input vertex data, different for all executions of this shader.\n";

    if (dpv == 3)
      qstr += "layout(location = 0) in vec3 vertexPosition_modelspace;\n";

    if (dpv == 4)
      qstr += "layout(location = 0) in vec4 vertexPosition_modelspace;\n";

    if (dpv == 6)
      {
	qstr += "layout(location = 0) in vec3 vertexPosition_modelspace;\n";
	qstr += "layout(location = 1) in vec4 vertexColor;\n\n";
      }
    
    qstr += "// Output data ; will be interpolated for each fragment.\n";
    qstr += "out vec3 fragmentColor;\n";    
    qstr += "out vec3 pointPos;\n";    
    qstr += "out float zdepth;\n";    

    qstr += "uniform mat4 MV;\n\n";
    qstr += "uniform mat4 MVP;\n\n";
    qstr += "uniform int pointSize;\n\n";

    qstr += "uniform vec3 coordMin;\n";    
    qstr += "uniform vec3 coordMax;\n";    

    qstr += "uniform sampler1D colorTex;\n";

    qstr += "uniform vec3 rightVec;\n";
    qstr += "uniform int screenWidth;\n";

    qstr += "uniform float projFactor;\n";

    qstr += "uniform sampler2DRect ommTex;\n";
    qstr += "uniform sampler2DRect visTex;\n";

    qstr += "uniform bool pointType;\n";

    //-----------------------
    qstr += "float numberOfOnes(float number, float index)\n";
    qstr += "{\n";
    qstr += "   float tmp = mod(number, pow(2.0, index + 1.0));\n";
    qstr += "   float numOnes = 0.0;\n";
    qstr += "   for (float i = 0.0; i < 8.0; i++)\n";
    qstr += "   {\n";
    qstr += "      if (mod(tmp, 2.0) != 0.0)\n";
    qstr += "         numOnes++;\n";
    qstr += "      tmp = floor(tmp / 2.0);\n";
    qstr += "    }\n";
    qstr += "    return numOnes;\n";
    qstr += "}\n";
    //-----------------------

    //-----------------------
    qstr += "bool isBitSet(float number, float index)\n";
    qstr += "{\n";
    qstr += "  return mod(floor(number / pow(2.0, index)), 2.0) != 0.0;\n";
    qstr += "}\n";
    //-----------------------

    //-----------------------
    qstr += "float getLOD(float tile, vec3 omin, vec3 omax, vec3 opos)\n";
    qstr += "{\n";

    qstr += "    if (any(lessThan(opos, omin)) || any(greaterThan(opos, omax)))\n";
    qstr += "      return 10.0;";

    qstr += "  float depth = 0.0;\n";
    qstr += "  vec3 osize = omax-omin;\n";
    qstr += "  vec3 pos = opos-omin;\n";
    qstr += "  vec3 offset = vec3(0.0);\n";
    qstr += "  float ioffset = 0.0;\n";

    qstr += "  vec4 valueTop = texture2DRect(visTex, vec2(ioffset,tile)).rgba;";

    qstr += "  for(int i=0; i<20; i++)\n"; // assuming we do not have more than 20 levels
    qstr += "  {\n";
    qstr += "    vec3 nsal = osize/pow(2.0, float(i));\n"; // node size at this octree level
    qstr += "    vec3 idx3d = (pos-offset)/nsal;\n";// which of the 8 octree node children

    //qstr += "    if (any(lessThan(idx3d, vec3(0.0)) ||
    //                     greaterThan(idx3d, vec3(1.0))))\n";
    //qstr += "      return 10.0;";    
    //qstr += "    idx3d = clamp(idx3d, vec3(0.0), vec3(1.0));\n";

    qstr += "    idx3d = floor(idx3d+vec3(0.5));\n";      // does the point lie in
    qstr += "    float index = 4.0*idx3d.x + 2.0*idx3d.y + idx3d.z;\n"; // node position
    qstr += "    vec4 value = texture2DRect(visTex, vec2(ioffset,tile)).rgba;";
    qstr += "    float mask = value.r*255.0;\n";  // get visible child nodes
    qstr += "    if (isBitSet(mask,index))\n"; // visible node at this position
    qstr += "    {\n";
    qstr += "      ioffset = ioffset + value.g * 255.0 + numberOfOnes(mask, index-1.0);";
    qstr += "      depth ++;\n";
    qstr += "    }";
    qstr += "    else"; // no mode visible child nodes at this position
    qstr += "    {\n";
    qstr += "      if (value.a > 0.0)\n";
    qstr += "        depth = max(depth, value.b*255.0);";
    qstr += "      return depth;";
    qstr += "    }";

    qstr += "    offset = offset + (nsal*0.5) * idx3d;\n";

    qstr += "  }\n";

    qstr += " return depth;";
    qstr += "}\n";
    //-----------------------


    qstr += "void main(){	\n\n";

    qstr += "	// Output position of the vertex, in clip space : MVP * position\n";

    qstr += "   pointPos = vertexPosition_modelspace.xyz;\n";

    qstr += "	gl_Position =  MVP * vec4(pointPos,1);\n";

    qstr += "   zdepth = ((gl_DepthRange.diff * gl_Position.z/gl_Position.w) +";
    qstr += "   gl_DepthRange.near + gl_DepthRange.far) / 2.0;\n";


    if (dpv == 3)
      {
	qstr += "   gl_PointSize = pointSize;\n";
	qstr += "   float z = (vertexPosition_modelspace.z-coordMin.z)/(coordMax.z-coordMin.z);\n";
	qstr += "   z = 0.5+sign(z-0.5)*sqrt(abs(z-0.5));\n";
	qstr += "   z = clamp(z, 0.0, 1.0);\n";
	qstr += "   fragmentColor = texture(colorTex, z).rgb;\n";
      }
    else
      {
	// pointsize
	//----------------------------------
	//qstr += "   gl_PointSize = pointSize*(vertexColor.a+1.0);\n";
	qstr += "   if (!pointType)\n"; // fixed pointsize
	qstr += "     gl_PointSize = pointSize;\n";
	
//	qstr += "   float tile = vertexColor.a;\n";
//	qstr += "   vec4 omins = texture2DRect(ommTex, vec2(0.0,tile));\n";
//	qstr += "   float spacing = omins.a;\n";
//	qstr += "   vec3 omin = omins.rgb;\n";
//	qstr += "   vec3 omax = texture2DRect(ommTex, vec2(1.0,tile)).rgb;\n";
//	qstr += "   float lod = getLOD(tile, omin, omax, pointPos);\n";
//	qstr += "   float clen = spacing/pow(1.9, lod);\n"; // world distance
//	qstr += "   vec4 p0 =  MVP * vec4(pointPos-clen*rightVec,1);\n";
//	qstr += "   p0 /= vec4(p0.w);\n";
//	qstr += "   vec4 p1 =  MVP * vec4(pointPos+clen*rightVec,1);\n";
//	qstr += "   p1 /= vec4(p1.w);\n";
//	qstr += "   float ptsz = abs(p1.x-p0.x);\n";
//	qstr += "   gl_PointSize = pointSize*clamp(ptsz, 1.0, 20);\n";


	qstr += "   if (pointType)\n"; // adaptive pointsize
	qstr += "   {\n";
	qstr += "     float tile = vertexColor.a;\n";
	qstr += "     vec4 omins = texture2DRect(ommTex, vec2(0.0,tile));\n";
	qstr += "     vec4 omaxs = texture2DRect(ommTex, vec2(1.0,tile));\n";
	qstr += "     float spacing = omins.a;\n";
	qstr += "     vec3 omin = omins.rgb;\n";
	qstr += "     vec3 omax = omaxs.rgb;\n";
	//qstr += "     float lod = omaxs.a;\n";
	//qstr += "     vec3 omax = texture2DRect(ommTex, vec2(1.0,tile)).rgb;\n";
	qstr += "     float lod = getLOD(tile, omin, omax, pointPos);\n";
	qstr += "     float viewZ = -(MV * vec4(pointPos,1)).z;\n";
	qstr += "     float r = spacing*1.5;\n";
	qstr += "     float worldSize = pointSize*r/pow(1.9, lod);\n";
	qstr += "     gl_PointSize = worldSize*projFactor/viewZ;\n"; 
	qstr += "     gl_PointSize = clamp(gl_PointSize, 0.5, 20.0);\n";
	qstr += "   }\n";
	//----------------------------------

	//----------------------------------
	// color
//	qstr += "vec3 col = vec3(1,0,0);\n";
//	qstr += "if (lod == 0) col = vec3(1, 1, 1);\n";
//	qstr += "if (lod == 1) col = vec3(1, 1, 0);\n";
//	qstr += "if (lod == 2) col = vec3(1, 0.5, 0);\n";
//	qstr += "if (lod == 3) col = vec3(0.5, 1, 0);\n";
//	qstr += "if (lod == 4) col = vec3(0, 1, 0.5);\n";
//	qstr += "if (lod == 5) col = vec3(0, 1, 0);\n";
//	qstr += "if (lod == 6) col = vec3(1, 1, 1);\n";
//	qstr += "if (lod == 7) col = vec3(1, 0.5, 0.5);\n";
// 	qstr += "fragmentColor = col;\n";
	
	qstr += "   fragmentColor = vertexColor.rgb/vec3(255.0,255.0,255.0);\n";
	//----------------------------------

      }


//    if (dpv == 4)
//      {
//	qstr += "   float idx = (vertexPosition_modelspace.w)/7.0;\n";
//	qstr += "   fragmentColor = texture(colorTex, idx).rgb;\n";
//	
//	qstr += "   float z = (vertexPosition_modelspace.z-coordMin.z)/(coordMax.z-coordMin.z);\n";
//	qstr += "   z = 0.5+sign(z-0.5)*sqrt(abs(z-0.5));\n";
//	qstr += "   z = clamp(z, 0.0, 1.0);\n";
//	qstr += "   fragmentColor = texture(colorTex, z).rgb;\n";
//	
//	qstr += "   fragmentColor = mix(fragmentColor, texture(colorTex, z).rgb, 0.5);\n";
//      }



    qstr += "}\n";
    

    int len = qstr.length();
    char *tbuffer = new char[len+1];
    sprintf(tbuffer, qstr.toLatin1().data());
    const char *sstr = tbuffer;
    glShaderSourceARB(vertObj, 1, &sstr, NULL);
    delete [] tbuffer;

    GLint compiled = -1;
    glCompileShaderARB(vertObj);
    glGetObjectParameterivARB(vertObj,
			      GL_OBJECT_COMPILE_STATUS_ARB,
			      &compiled);
    if (!compiled)
      {
	GLcharARB str[1000];
	GLsizei len;
	glGetInfoLogARB(vertObj,
			(GLsizei) 1000,
			&len,
			str);
	
	QMessageBox::information(0,
				 "Error : Vertex Shader",
				 str);
	return false;
    }
  }
    
    
  { // fragObj
    int len = shaderString.length();
    char *tbuffer = new char[len+1];
    sprintf(tbuffer, shaderString.toLatin1().data());
    const char *sstr = tbuffer;
    glShaderSourceARB(fragObj, 1, &sstr, NULL);
    delete [] tbuffer;
  
    GLint compiled = -1;
    glCompileShaderARB(fragObj);
    glGetObjectParameterivARB(fragObj,
			    GL_OBJECT_COMPILE_STATUS_ARB,
			      &compiled);
    if (!compiled)
      {
	GLcharARB str[1000];
	GLsizei len;
	glGetInfoLogARB(fragObj,
			(GLsizei) 1000,
			&len,
			str);
	
	//-----------------------------------
	// display error
	
	//qApp->beep();
	
	QString estr;
	QStringList slist = shaderString.split("\n");
	for(int i=0; i<slist.count(); i++)
	  estr += QString("%1 : %2\n").arg(i+1).arg(slist[i]);
	
	QTextEdit *tedit = new QTextEdit();
	tedit->insertPlainText("-------------Error----------------\n\n");
	tedit->insertPlainText(str);
	tedit->insertPlainText("\n-----------Shader---------------\n\n");
	tedit->insertPlainText(estr);
	
	QVBoxLayout *layout = new QVBoxLayout();
	layout->addWidget(tedit);

	QDialog *showError = new QDialog();
	showError->setWindowTitle("Error in Fragment Shader");
	showError->setSizeGripEnabled(true);
	showError->setModal(true);
	showError->setLayout(layout);
	showError->exec();
	//-----------------------------------

	return false;
      }
  }

  
  //----------- link program shader ----------------------
  GLint linked = -1;
  glLinkProgramARB(progObj);
  glGetObjectParameterivARB(progObj, GL_OBJECT_LINK_STATUS_ARB, &linked);
  if (!linked)
    {
      GLcharARB str[1000];
      GLsizei len;
      QMessageBox::information(0,
			       "ProgObj",
			       "error linking texProgObj");
      glGetInfoLogARB(progObj,
		      (GLsizei) 1000,
		      &len,
		      str);
      QMessageBox::information(0,
			       "Error",
			       QString("%1\n%2").arg(len).arg(str));
      return false;
    }

  glDeleteObjectARB(fragObj);
  glDeleteObjectARB(vertObj);

  return true;
}

QString
ShaderFactory::genPointShaderString()
{
  QString shader;

  shader += "#version 330 core\n";
  shader += "\n";
  shader += "// Interpolated values from the vertex shaders\n";
  shader += "in vec3 fragmentColor;\n\n";

  shader += "// Ouput data\n";
  shader += "layout(location=0) out vec3 color;\n";

  shader += "void main(){\n";

  shader += "	// Output color = color specified in the vertex shader, \n";
  shader += "	// interpolated between all 3 surrounding vertices\n";
  shader += "	color = fragmentColor;\n\n";

  shader += "}\n";

  return shader;
}

QString
ShaderFactory::genPointDepth(bool shadows)
{
  QString shader;

  shader += "#version 330 core\n";
  shader += "\n";
  shader += "in vec3 fragmentColor;\n\n";
  shader += "in vec3 pointPos;\n\n";
  shader += "in float zdepth;\n\n";

  shader += "// Ouput data\n";
  shader += "layout(location=0) out vec4 color;\n";

  if (shadows)
    shader += "layout(location=1) out vec3 depth;\n";

  shader += "uniform vec3 eyepos;\n";
  shader += "uniform vec3 viewdir;\n";

  shader += "void main(){\n";

  shader += "  float dc = length(2.0 * gl_PointCoord - 1.0);\n";
  shader += "  if (dc > 1.0) discard;\n";

  shader += "  color = vec4(fragmentColor, 1.0);\n";

  if (shadows)
    {
      shader += "  float d = dot((pointPos-eyepos), viewdir);\n";
      shader += "  depth = vec3(d,zdepth,gl_FragCoord.z);\n";
    }
  shader += "}\n";

  return shader;
}

QString
ShaderFactory::drawShadowsVertexShader()
{
  QString shader;

  shader += "#version 330 core\n";
  shader += "layout(location = 0) in vec3 vertex;\n";
  shader += "uniform mat4 MV;\n";
  shader += "uniform mat4 P;\n";
  shader += "void main(){\n";  
  shader += "	gl_Position =  P* MV * vec4(vertex,1);\n";
  shader += "}\n";

  return shader;
}

QString
ShaderFactory::drawShadowsFragmentShader()
{
  QString shader;

  shader += "#version 330 core\n";
  shader += "\n";
  shader += "// Ouput data\n";
  shader += "layout(location=0) out vec4 color;\n";
  shader += "out float gl_FragDepth;\n";

  shader += "uniform sampler2DRect colorTex;\n";
  shader += "uniform sampler2DRect depthTex;\n";

  shader += "uniform bool showedges;\n";
  shader += "uniform bool softShadows;\n";

  shader += "void main(){\n";

  shader += "  vec2 spos = gl_FragCoord.xy;\n";

  shader += "  color = texture2DRect(colorTex, spos.xy);\n";

  shader += "  vec4 dtex = texture2DRect(depthTex, spos.xy);\n";
  shader += "  float depth = dtex.x;\n";
  shader += "  gl_FragDepth = dtex.y;\n";

  //shader += "  float depth = texture2DRect(depthTex, spos.xy).x;\n";

  shader += "  if (depth < 0.001) discard;\n";
  //shader += "  if (depth < 0.001) {color = vec4(0.3); return; };\n";


  shader += " if (softShadows)\n";
  shader += " {;\n";
  shader += "  float dty = dtex.y*dtex.y;\n";
  shader += "  float sum = 0.0;\n";
  shader += "  float tele = 0.0;\n";
  shader += "  float r = 1.0;\n";
  shader += "  float theta = 0.0;\n";
  shader += "  int cnt = 4;\n";
  shader += "  float ege = 0.0;\n";
  shader += "  int j = 0;\n";  
  shader += "  for(int i=0; i<(dtex.y*dtex.y)*10; i++)\n";
  shader += "  {\n";
  shader += "    int x = int(r*sin(theta));\n";
  shader += "    int y = int(r*cos(theta));\n";
  shader += "    vec2 pos = spos + vec2(x,y);\n";
  shader += "    float od = depth - texture2DRect(depthTex, pos).x;\n";
  shader += "    float ege = r*0.01;\n";
  shader += "    sum += step(ege, od);\n";
  shader += "    tele ++;\n";
  shader += "    r += float(i)/float(cnt);\n";
  shader += "    theta += 6.28/(r+3.0);\n";  
  shader += "    if (i>=cnt) cnt = cnt+int(r)+3;\n";
  shader += "  }\n"; 
  shader += "  sum /= tele;\n";
  shader += "  sum = sqrt(sum);\n";
  shader += "  color.rgb = mix(color.rgb, vec3(1.0-dty), sum);\n";
  shader += " };\n";


  shader += " if (showedges)\n";
  shader += " {;\n";
  float cx[8] = {-1.0, 0.0, 1.0, 0.0,-1.0,-1.0, 1.0, 1.0};
  float cy[8] = { 0.0,-1.0, 0.0, 1.0,-1.0, 1.0,-1.0, 1.0};
  shader += "  float cx[8];\n";
  shader += "  float cy[8];\n";
  for(int i=0; i<8; i++)
    shader += QString("  cx[%1] = float(%2);\n").arg(i).arg(cx[i]);
  for(int i=0; i<8; i++)
    shader += QString("  cy[%1] = float(%2);\n").arg(i).arg(cy[i]);
  shader += "  float response = 0.0;\n";
  shader += "  float tele = 0.0;\n";
  shader += "  float isoshadow = 2*(1.0-dtex.y)*(1.0-dtex.y);\n";  
  shader += "  depth = log2(depth);\n";
  shader += "  for(int i=0; i<8; i++)\n";
  shader += "  {\n";
  shader += "    vec2 pos = spos + vec2(cx[i],cy[i]);\n";
  shader += "    float od = depth - log2(texture2DRect(depthTex, pos).x);\n";
  shader += "    response += max(0.0, od);\n";
  shader += "    tele ++;\n";
  shader += "  }\n"; 
  shader += "  response /= tele;\n";
  shader += "  float shadow = exp(-response*300*isoshadow);\n";
  shader += "  color.rgb *= shadow;\n";
  shader += " };\n";


  shader += "}\n";

  return shader;
}

QString
ShaderFactory::genBilateralFilterShaderString()
{
  QString shader;

  shader += "#version 330 core\n";
  shader += "\n";
  shader += "// Ouput data\n";
  shader += "layout(location=0) out vec3 color;\n";

  shader += "uniform sampler2DRect depthTex;\n";
  shader += "\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  color = vec3(0.0);\n";
  shader += "  vec2 uv = gl_FragCoord.xy;\n";

  shader += "  float depth = texture2DRect(depthTex, uv).x;\n";
  shader += "  if (depth < 0.01) discard;\n";

  shader += "  float ldpt = log2(depth);\n";
  
  float cx[8] = {-1.0, 0.0, 1.0, 0.0, -1.0,-1.0, 1.0, 1.0};
  float cy[8] = { 0.0,-1.0, 0.0, 1.0, -1.0, 1.0,-1.0, 1.0};
  shader += "  float cx[8];\n";
  shader += "  float cy[8];\n";
  for(int i=0; i<8; i++)
    shader += QString("  cx[%1] = float(%2);\n").arg(i).arg(cx[i]);
  for(int i=0; i<8; i++)
    shader += QString("  cy[%1] = float(%2);\n").arg(i).arg(cy[i]);

  shader += "  float tele = 1.0;\n";
  shader += "  for(int i=0; i<8; i++)\n";
  shader += "  {\n";
  shader += "    vec2 pos = uv + vec2(cx[i],cy[i]);\n";
  shader += "    float dp = texture2DRect(depthTex, pos).x;\n";
  shader += "    float od = step(abs(log2(dp)-ldpt), 0.1);\n";
  shader += "    depth += od*dp;\n";
  shader += "    tele += od;\n";
  shader += "  }\n"; 

  shader += "  depth /= tele;\n";
  shader += "  color = vec3(depth, depth, depth);\n";

  shader += "}\n";

  return shader;
}


GLuint ShaderFactory::m_rcShader = 0;
GLuint ShaderFactory::rcShader() { return m_rcShader; }

GLint ShaderFactory::m_rcShaderParm[10];
GLint* ShaderFactory::rcShaderParm() { return &m_rcShaderParm[0]; }

void
ShaderFactory::createTextureShader()
{
  m_rcShader = glCreateProgram();
  if (!ShaderFactory::loadShadersFromFile(m_rcShader,
					  "assets/shaders/controllershader.vert",
					  "assets/shaders/controllershader.frag"))
    {
      QMessageBox::information(0, "", "Cannot load controller shaders");
    }

  m_rcShaderParm[0] = glGetUniformLocation(m_rcShader, "MVP");
  m_rcShaderParm[1] = glGetUniformLocation(m_rcShader, "diffuse");
  m_rcShaderParm[2] = glGetUniformLocation(m_rcShader, "color");
  m_rcShaderParm[3] = glGetUniformLocation(m_rcShader, "viewDir");
  m_rcShaderParm[4] = glGetUniformLocation(m_rcShader, "opmod");
  m_rcShaderParm[5] = glGetUniformLocation(m_rcShader, "applytexture");
  m_rcShaderParm[6] = glGetUniformLocation(m_rcShader, "ptsz");
  m_rcShaderParm[7] = glGetUniformLocation(m_rcShader, "mixcolor");
}


GLuint ShaderFactory::m_cubemapShader = 0;
GLuint ShaderFactory::cubemapShader() { return m_cubemapShader; }

GLint ShaderFactory::m_cubemapShaderParm[10];
GLint* ShaderFactory::cubemapShaderParm() { return &m_cubemapShaderParm[0]; }

void
ShaderFactory::createCubeMapShader()
{
  m_cubemapShader = glCreateProgram();
  if (!ShaderFactory::loadShadersFromFile(m_cubemapShader,
					  "assets/shaders/cubemap.vert",
					  "assets/shaders/cubemap.frag"))
    {
      QMessageBox::information(0, "", "Cannot load cubemap shaders");
    }

  m_cubemapShaderParm[0] = glGetUniformLocation(m_cubemapShader, "MVP");
  m_cubemapShaderParm[1] = glGetUniformLocation(m_cubemapShader, "hmdPos");
  m_cubemapShaderParm[2] = glGetUniformLocation(m_cubemapShader, "scale");
  m_cubemapShaderParm[3] = glGetUniformLocation(m_cubemapShader, "skybox");
}
