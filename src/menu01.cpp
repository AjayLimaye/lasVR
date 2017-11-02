#include "global.h"
#include "menu01.h"
#include "shaderfactory.h"
#include "staticfunctions.h"

#include <QFont>
#include <QColor>
#include <QtMath>
#include <QMessageBox>

Menu01::Menu01() : QObject()
{
  m_stepTexture = 0;
  m_dataNameTexture = 0;
  m_glTexture = 0;
  m_glVertBuffer = 0;
  m_glIndexBuffer = 0;
  m_glVertArray = 0;
  m_vertData = 0;

  m_mapScale = 0.1;

  m_menuList.clear();
  m_menuDim.clear();

  m_relGeom.clear();
  m_optionsGeom.clear();

  m_selected = -1;

  m_visible = true;
  m_pointingToMenu = false;

  m_softShadows = true;
  m_edges = true;
  m_spheres = false;
  m_play = false;
  m_playMenu = false;
  m_playButton = false;

  m_showMap = false;
}

Menu01::~Menu01()
{
  if(m_vertData)
    {
      delete [] m_vertData;
      m_vertData = 0;
      glDeleteBuffers(1, &m_glIndexBuffer);
      glDeleteVertexArrays( 1, &m_glVertArray );
      glDeleteBuffers(1, &m_glVertBuffer);
      m_glIndexBuffer = 0;
      m_glVertArray = 0;
      m_glVertBuffer = 0;
    }
}

void
Menu01::setTimeStep(QString stpStr)
{
  QFont font = QFont("Helvetica", 48);
  QColor color(250, 230, 210); 
  QImage img = StaticFunctions::renderText(stpStr,
					   font,
					   Qt::transparent, color, false);

  int twd = img.width();
  int tht = img.height();

  int szh = tht;
  int szw = 1.333*szh; // 120/90=aspect ration of final image
  if (szw < twd)
    {
      szw = twd;
      szh = szw/1.333;
    }
  QImage bimg = QImage(szw, szh, QImage::Format_ARGB32);
  bimg.fill(Qt::transparent);
  QPainter p(&bimg);
  p.drawImage((szw-twd)/2, (szh-tht)/2, img.mirrored(false, true));


  if (!m_stepTexture)
    glGenTextures(1, &m_stepTexture);

  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, m_stepTexture);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D,
	       0,
	       4,
	       szw,
	       szh,
	       0,
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       bimg.bits());
  
  glDisable(GL_TEXTURE_2D);
}

void
Menu01::setDataShown(QString dataStr)
{
  QFont font = QFont("Helvetica", 48);
  QColor color(250, 230, 210); 
  QImage img = StaticFunctions::renderText(dataStr,
					   font,
					   Qt::transparent, color, false);

  int twd = img.width();
  int tht = img.height();

  int szh = tht;
  int szw = 5*szh; // 450/90=aspect ratio of final image
  if (szw < twd)
    {
      szw = twd;
      szh = szw/5.0;
    }
  QImage bimg = QImage(szw, szh, QImage::Format_ARGB32);
  bimg.fill(Qt::transparent);
  QPainter p(&bimg);
  p.drawImage((szw-twd)/2, (szh-tht)/2, img.mirrored(false, true));


  if (!m_dataNameTexture)
    glGenTextures(1, &m_dataNameTexture);

  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, m_dataNameTexture);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D,
	       0,
	       4,
	       szw,
	       szh,
	       0,
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       bimg.bits());
  
  glDisable(GL_TEXTURE_2D);
}

void
Menu01::genVertData()
{
  if (m_playMenu && !m_stepTexture)
    setTimeStep("0");

  if (!m_dataNameTexture)
    setDataShown("...");
  

  m_vertData = new float[32];
  memset(m_vertData, 0, sizeof(float)*32);


  // create and bind a VAO to hold state for this model
  glGenVertexArrays( 1, &m_glVertArray );
  glBindVertexArray( m_glVertArray );
      
      // Populate a vertex buffer
  glGenBuffers( 1, &m_glVertBuffer );
  glBindBuffer( GL_ARRAY_BUFFER, m_glVertBuffer );
  glBufferData( GL_ARRAY_BUFFER,
		sizeof(float)*8*4,
		NULL,
		GL_STATIC_DRAW );
      
  // Identify the components in the vertex buffer
  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 0, //attribute 0
			 3, // size
			 GL_FLOAT, // type
			 GL_FALSE, // normalized
			 sizeof(float)*8, // stride
			 (void *)0 ); // starting offset

  glEnableVertexAttribArray( 1 );
  glVertexAttribPointer( 1,
			 3,
			 GL_FLOAT,
			 GL_FALSE,
			 sizeof(float)*8,
			 (char *)NULL + sizeof(float)*3 );
  
  glEnableVertexAttribArray( 2 );
  glVertexAttribPointer( 2,
			 2,
			 GL_FLOAT,
			 GL_FALSE, 
			 sizeof(float)*8,
			 (char *)NULL + sizeof(float)*6 );
  
  
  
  uchar indexData[6];
  indexData[0] = 0;
  indexData[1] = 1;
  indexData[2] = 2;
  indexData[3] = 0;
  indexData[4] = 2;
  indexData[5] = 3;
  
  // Create and populate the index buffer
  glGenBuffers( 1, &m_glIndexBuffer );
  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer );
  glBufferData( GL_ELEMENT_ARRAY_BUFFER,
		sizeof(uchar) * 6,
		&indexData[0],
		GL_STATIC_DRAW );
  
  glBindVertexArray( 0 );
    

  if (!m_glTexture)
    {
      QImage menuImg(":/images/menu00.png");
      m_texWd = menuImg.width();
      m_texHt = menuImg.height();

      glGenTextures(1, &m_glTexture);
  
      glActiveTexture(GL_TEXTURE4);
      glBindTexture(GL_TEXTURE_2D, m_glTexture);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexImage2D(GL_TEXTURE_2D,
		   0,
		   4,
		   m_texWd,
		   m_texHt,
		   0,
		   GL_RGBA,
		   GL_UNSIGNED_BYTE,
		   menuImg.bits());
  
      glDisable(GL_TEXTURE_2D);
    }


  m_relGeom.clear();
  //m_relGeom << QRect(20, 10, 480, 90); // RESET
  m_relGeom << QRect(20, 10, 400, 90); // RESET

  m_relGeom << QRect(420, 10, 90, 90); // MAP

  m_relGeom << QRect(300, 100, 100, 100); // point size -
  m_relGeom << QRect(400, 100, 100, 100); // point size +
  m_relGeom << QRect(300, 200, 100, 100); // scaling -
  m_relGeom << QRect(400, 200, 100, 100); // scaling +

  m_relGeom << QRect(400, 300, 100, 55); // soft shadows
  m_relGeom << QRect(400, 365, 100, 55); // edges
  m_relGeom << QRect(400, 430, 100, 55); // spheres

  m_relGeom << QRect(25,  510, 75, 80); // play-reset
  m_relGeom << QRect(100, 510, 90, 80); // step back
  m_relGeom << QRect(190, 510, 90, 80); // step forward
  m_relGeom << QRect(280, 510, 90, 80); // play/pause

  m_relGeom << QRect(380, 505, 120, 90); // step number


  m_optionsGeom.clear();
  for (int i=0; i<m_relGeom.count(); i++)
    {
       float cx = m_relGeom[i].x();
       float cy = m_relGeom[i].y();
      float cwd = m_relGeom[i].width();
      float cht = m_relGeom[i].height();

      m_optionsGeom << QRectF(cx/(float)m_texWd,
			      cy/(float)m_texHt,
			      cwd/(float)m_texWd,
			      cht/(float)m_texHt);
    }


  m_menuDim << QRect(0, 0, m_texWd, 500); // default menu
  m_menuDim << QRect(0, 500, m_texWd, 100); // play menu


  //data name
  m_dataGeom << QRectF(25/(float)m_texWd, 510/(float)m_texHt,
		       450/(float)m_texWd, 90/(float)m_texHt);
  m_dataGeom << QRectF(25/(float)m_texWd, 610/(float)m_texHt,
		       450/(float)m_texWd, 90/(float)m_texHt);
}

void
Menu01::setShowMap(bool b)
{
  m_showMap = b;

  QImage menuImg(":/images/menu00.png");
  m_texWd = menuImg.width();
  m_texHt = menuImg.height();

  if (!m_glTexture)
    glGenTextures(1, &m_glTexture);

  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, m_glTexture);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  
  if (m_showMap)
    {
      QImage pimg = QImage(m_texWd, m_texHt, QImage::Format_ARGB32);
      QPainter p(&pimg);
      p.drawImage(0, 0, menuImg.rgbSwapped());

      QFont font = QFont("Helvetica", 48);
      QColor color(200, 220, 250); 
      QImage tmpTex = StaticFunctions::renderText("map",
						  font,
						  Qt::black, color); 
      tmpTex = tmpTex.scaledToWidth(60, Qt::SmoothTransformation);

      p.drawImage(435, m_texHt-75, tmpTex.mirrored());

      glTexImage2D(GL_TEXTURE_2D,
		   0,
		   4,
		   m_texWd,
		   m_texHt,
		   0,
		   GL_RGBA,
		   GL_UNSIGNED_BYTE,
		   pimg.bits());
    }
  else
    glTexImage2D(GL_TEXTURE_2D,
		 0,
		 4,
		 m_texWd,
		 m_texHt,
		 0,
		 GL_RGBA,
		 GL_UNSIGNED_BYTE,
		 menuImg.bits());
  
  glDisable(GL_TEXTURE_2D);
}

void
Menu01::draw(QMatrix4x4 mvp, QMatrix4x4 matL, bool triggerPressed)
{
  if (!m_visible)
    return;

  QVector3D center = QVector3D(matL * QVector4D(0,0,0,1));
  QVector3D ofront, oright, oup;
  QVector3D front, right, up;

  ofront = QVector3D(matL * QVector4D(0,0,1,1)) - center; 
  oup = QVector3D(matL * QVector4D(0,1,0,1)) - center;
  oright = QVector3D(matL * QVector4D(1,0,0,1)) - center;

  front = m_mapScale * ofront;
  right = m_mapScale * oright;
  up = 0.05 * oup;

  float opmod = qAbs(qBound(-0.1f, 0.0f, ofront.y()))/0.1f;  

  if (!m_vertData)
    genVertData();


  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, m_glTexture);
  glEnable(GL_TEXTURE_2D);

  glBindVertexArray(m_glVertArray);
  glBindBuffer( GL_ARRAY_BUFFER, m_glVertBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);  

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);


  glUseProgram(ShaderFactory::rcShader());
  GLint *rcShaderParm = ShaderFactory::rcShaderParm();
  glUniformMatrix4fv(rcShaderParm[0], 1, GL_FALSE, mvp.data() );  
  glUniform1i(rcShaderParm[1], 4); // texture
  glUniform3f(rcShaderParm[2], 0, 0, 0); // color
  glUniform3f(rcShaderParm[3], 0, 0, 0); // view direction
  glUniform1f(rcShaderParm[4], opmod); // opacity modulator
  glUniform1i(rcShaderParm[5], 5); // texture + color
  glUniform1f(rcShaderParm[6], 5); // pointsize

  glUniform1f(rcShaderParm[7], 0.5); // mixcolor


  QVector3D vfront = 2*front;

  float rt = (float)m_menuDim[1].height()/(float)m_texHt;
  QVector3D vfrontActual = vfront + rt*vfront;

  if (m_playMenu) // show play menu
    {
      // increase height of menu image to accommodate play menu
      vfront = vfrontActual;
    }

  QVector3D vright = 2*right;
  QVector3D vleft = center - right - 0.1*front;

  QVector3D v[4];  
  v[0] = vleft;
  v[1] = v[0] - vfront;
  v[2] = v[1] + vright;
  v[3] = v[2] + vfront;
      
  for(int i=0; i<4; i++)
    {
      m_vertData[8*i + 0] = v[i].x();
      m_vertData[8*i + 1] = v[i].y();
      m_vertData[8*i + 2] = v[i].z();
      m_vertData[8*i+3] = 0;
      m_vertData[8*i+4] = 0;
      m_vertData[8*i+5] = 0;
    }

  float texC[] = {0,1, 0,0, 1,0, 1,1};
  if (!m_playMenu) // don't show play menu
    {
      float rt = (float)m_menuDim[1].height()/(float)m_texHt;
      float tws = 0;
      float twe = 1;
      float ths = rt;
      float the = 1;
      texC[0] = tws;      texC[1] = the;
      texC[2] = tws;      texC[3] = ths;
      texC[4] = twe;      texC[5] = ths;
      texC[6] = twe;      texC[7] = the;
    }
  else
    {
      texC[0] = 0;
      texC[1] = 1;
      texC[2] = 0;
      texC[3] = 0;
      texC[4] = 1;
      texC[5] = 0;
      texC[6] = 1;
      texC[7] = 1;
    }


  m_vertData[6] =  texC[0];  m_vertData[7] =  texC[1];
  m_vertData[14] = texC[2];  m_vertData[15] = texC[3];
  m_vertData[22] = texC[4];  m_vertData[23] = texC[5];
  m_vertData[30] = texC[6];  m_vertData[31] = texC[7];
  
  glBufferSubData(GL_ARRAY_BUFFER,
		  0,
		  sizeof(float)*8*4,
		  &m_vertData[0]);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);  
    

  //for(int og=0; og<m_optionsGeom.count(); og++)
  int ogend = m_optionsGeom.count();
  if (!m_playMenu)
    ogend = 9;
  else
    ogend = m_optionsGeom.count()-1; // don't show step number here

  for(int og=0; og<ogend; og++)
    {
      bool ok = false;
      ok = (m_selected == og);
      ok |= (og == 6 && m_softShadows);
      ok |= (og == 7 && m_edges);
      ok |= (og == 8 && m_spheres);
      ok |= (og >= 12);

      if (ok)
	{
	  float cx = m_optionsGeom[og].x();
	  float cy = m_optionsGeom[og].y();
	  float cwd = m_optionsGeom[og].width();
	  float cht = m_optionsGeom[og].height();

	  Vec mc = Vec(0,0,0);
	  if (og == m_selected)
	    {
	      if (triggerPressed)
		mc = Vec(0.2, 0.8, 0.2);
	      else
		mc = Vec(0.5, 0.7, 1);
	    }
	  else
	    {
	      if (og < 12 || (og == 12 && m_play))
		mc = Vec(0.5, 1.0, 0.5);
	    }

	  if (og == 12 && !m_playButton) // hide play button
	    mc = Vec(0.01, 0.01, 0.01);

	  showText(m_glTexture, m_optionsGeom[og],
		   vleft, vright, vfrontActual, up,
		   cx, cx+cwd, cy, cy+cht,
		   mc);
	}
    }
  
  // ---- 
  // show step number
  if (m_playMenu)
    showText(m_stepTexture, m_optionsGeom[13],
	     vleft, vright, vfrontActual, up,
	     0, 1, 0, 1,
	     Vec(0,0,0));
  // ---- 

  // ---- 
  // show data name
  int dg = 0;
  if (m_playMenu)
    dg = 1;
  showText(m_dataNameTexture, m_dataGeom[dg],
	   vleft, vright, vfrontActual, up,
	   0, 1, 0, 1,
	   Vec(0,0,0));
  // ---- 


  glBindVertexArray(0);
  glDisable(GL_TEXTURE_2D);
  
  glUseProgram( 0 );
  
  glDisable(GL_BLEND);
}

void
Menu01::showText(GLuint tex, QRectF geom,
		 QVector3D vleft, QVector3D vright,
		 QVector3D vfrontActual, QVector3D up,
		 float tx0, float tx1, float ty0, float ty1,
		 Vec mc)
{
  glBindTexture(GL_TEXTURE_2D, tex);

  GLint *rcShaderParm = ShaderFactory::rcShaderParm();
  glUniform3f(rcShaderParm[2], mc.x, mc.y, mc.z); // mix color

  float cx =  geom.x();
  float cy =  geom.y();
  float cwd = geom.width();
  float cht = geom.height();

  QVector3D v[4];  
  v[0] = vleft + cx*vright - cy*vfrontActual + 0.01*up; // slightly raised
  v[1] = v[0] - cht*vfrontActual;
  v[2] = v[1] + cwd*vright;
  v[3] = v[2] + cht*vfrontActual;
  
  for(int i=0; i<4; i++)
    {
      m_vertData[8*i + 0] = v[i].x();
      m_vertData[8*i + 1] = v[i].y();
      m_vertData[8*i + 2] = v[i].z();
      m_vertData[8*i+3] = 0;
      m_vertData[8*i+4] = 0;
      m_vertData[8*i+5] = 0;
    }
  
  float texD[] = {tx0,1-ty0, tx0,1-ty1, tx1,1-ty1, tx1,1-ty0};
	  
  m_vertData[6] =  texD[0];  m_vertData[7] =  texD[1];
  m_vertData[14] = texD[2];  m_vertData[15] = texD[3];
  m_vertData[22] = texD[4];  m_vertData[23] = texD[5];
  m_vertData[30] = texD[6];  m_vertData[31] = texD[7];
  
  glBufferSubData(GL_ARRAY_BUFFER,
		  0,
		  sizeof(float)*8*4,
		  &m_vertData[0]);
  
  
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);  
}

int
Menu01::checkOptions(QMatrix4x4 matL, QMatrix4x4 matR, bool triggered)
{
  m_pinPt = QVector3D(-1000,-1000,-1000);
  m_pointingToMenu = false;
  m_selected = -1;

  if (!m_visible || m_optionsGeom.count()==0)
    return -1;

  QVector3D centerL = QVector3D(matL * QVector4D(0,0,0,1));
  QVector3D ofrontL, orightL, oupL;
  QVector3D frontL, rightL, upL;

  ofrontL = QVector3D(matL * QVector4D(0,0,1,1)) - centerL; 
  oupL = QVector3D(matL * QVector4D(0,1,0,1)) - centerL;
  orightL = QVector3D(matL * QVector4D(1,0,0,1)) - centerL;

  frontL = m_mapScale * ofrontL;
  rightL = m_mapScale * orightL;
  upL = 0.05 * oupL;


  float opmod = qAbs(qBound(-0.1f, 0.0f, ofrontL.y()))/0.1f;
  if (opmod < 0.1)
    return m_selected;

  QVector3D vft = 2*frontL;
  QVector3D vrt = 2*rightL;

  float rt = (float)m_menuDim[1].height()/(float)m_texHt;
  QVector3D vftActual = vft + rt*vft;
  if (m_playMenu) // show play menu
    {
      // increase height of menu image to accommodate play menu
      vft = vftActual;
    }

  QVector3D vleft = centerL - rightL - 0.1*frontL;


  QVector3D centerR = QVector3D(matR * QVector4D(0,0,0,1));
  QVector3D frontR;
  frontR = QVector3D(matR * QVector4D(0,0,-0.1,1)) - centerR;    

  QVector3D pinPoint = centerR + frontR;

  float rw, rh;
  m_pointingToMenu = StaticFunctions::intersectRayPlane(vleft, -vftActual, vrt,
							oupL.normalized(),
							centerR, frontR.normalized(),
							rh, rw);

  if (!m_pointingToMenu)
    return -1;


  m_pinPt = vleft + rh*vrt - rw*vft;

  int tp = -1;

  int ogend = 9;

  // play menu : last is step number
  if (m_playMenu)
    ogend = 13;

  for(int t=0; t<ogend; t++)
    {
      float cx = m_optionsGeom[t].x();
      float cy = m_optionsGeom[t].y();
      float cwd = m_optionsGeom[t].width();
      float cht = m_optionsGeom[t].height();

      if (cx < rh && cx+cwd > rh &&
	  cy < rw && cy+cht > rw)
	{
	  tp = t;
	}
    }

  m_selected = tp;

  //--------------------
  if (!m_playButton && m_selected == 12)
    m_selected = -1;
  //--------------------
  
  if (triggered)
    {
      if (m_selected == 0) emit resetModel();
      else if (m_selected == 1) emit updateMap();
      else if (m_selected == 2) emit updatePointSize(-1);
      else if (m_selected == 3) emit updatePointSize(1);
      else if (m_selected == 4) emit updateScale(-1);
      else if (m_selected == 5) emit updateScale(1);
      else if (m_selected == 6)
	{
	  m_softShadows = !m_softShadows;
	  emit updateSoftShadows(m_softShadows);
	}
      else if (m_selected == 7)
	{
	  m_edges = !m_edges;
	  emit updateEdges(m_edges);
	}
      else if (m_selected == 8)
	{
	  m_spheres = !m_spheres;
	  emit updateSpheres(m_spheres);
	}
      else if (m_selected == 9) emit gotoFirstStep();
      else if (m_selected == 10) emit gotoPreviousStep();
      else if (m_selected == 11) emit gotoNextStep();
      else if (m_selected == 12 && m_playButton)
	{
	  m_play = !m_play;
	  emit playPressed(m_play);
	}
    }

  return m_selected;
}

