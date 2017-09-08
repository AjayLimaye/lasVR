#include "global.h"
#include "menu00.h"
#include "shaderfactory.h"
#include "staticfunctions.h"

#include <QFont>
#include <QColor>
#include <QtMath>
#include <QMessageBox>

Menu00::Menu00() : QObject()
{
  m_glTexture = 0;
  m_stepTexture = 0;
  m_glVertBuffer = 0;
  m_glIndexBuffer = 0;
  m_glVertArray = 0;
  m_vertData = 0;

  m_mapScale = 0.1;

  m_menuList.clear();
  m_colorList.clear();

  m_menuList << "|>";
  m_menuList << "||";
  m_menuList << "(-)";
  m_menuList << "(+)";
  m_menuList << "|<";


  m_colorList << QColor(200, 220, 250); 
  m_colorList << QColor(250, 150, 100); 
  m_colorList << QColor(200, 220, 250); 
  m_colorList << QColor(200, 220, 250); 
  m_colorList << QColor(200, 220, 250); 

  m_relGeom.clear();
  m_optionsGeom.clear();

  m_play = false;
  m_selected = -1;

  m_visible = true;
  m_pointingToMenu = false;  
}

Menu00::~Menu00()
{
  if(m_vertData)
    {
      delete [] m_vertData;
      glDeleteBuffers(1, &m_glIndexBuffer);
      glDeleteVertexArrays( 1, &m_glVertArray );
      glDeleteBuffers(1, &m_glVertBuffer);
      m_glIndexBuffer = 0;
      m_glVertArray = 0;
      m_glVertBuffer = 0;
    }
}

void
Menu00::setTimeStep(QString stpStr)
{
  QFont font = QFont("Helvetica", 48);
  QColor color(250, 230, 210); 
  QImage img = StaticFunctions::renderText(stpStr,
					   font,
					   Qt::black, color);
  int twd = img.width();
  int tht = img.height();

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
	       twd,
	       tht,
	       0,
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       img.bits());
  
  glDisable(GL_TEXTURE_2D);
}

void
Menu00::genVertData()
{
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
    

  QFont font = QFont("Helvetica", 48);
  QColor color(200, 220, 250); 

  int wd = 0;
  QList<QImage> img;
  for (int i=0; i<m_menuList.count(); i++)
    {
      QImage textimg = StaticFunctions::renderText(m_menuList[i],
						   font,
						   Qt::black, m_colorList[i]);
      wd = qMax(wd, textimg.width());
      img << textimg;
    }
  
  m_relGeom.clear();
  int ht = 0;
  for (int i=0; i<img.count(); i++)
    {
      m_relGeom << QRect(0, ht, img[i].width(), img[i].height()); 
      ht = ht + img[i].height();
    }
  
  m_texWd = wd;
  m_texHt = ht;
  
  QImage images = QImage(m_texWd, m_texHt, QImage::Format_ARGB32);
  images.fill(Qt::transparent);
  QPainter p(&images);
  for (int i=0; i<img.count(); i++)
    {
      QRect geom = m_relGeom[i];
      p.drawImage(geom.x(), geom.y(), img[i].rgbSwapped());
    }
  
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
	       images.bits());
  
  glDisable(GL_TEXTURE_2D);


  if (!m_stepTexture)
    setTimeStep("Start");


  m_optionsGeom.clear();
  m_optionsGeom << QRectF(0.50, 0.1, 0.30, 0.30);
  m_optionsGeom << QRectF(0.20, 0.1, 0.30, 0.30);
  m_optionsGeom << QRectF(0.80, 0.1, 0.30, 0.30);
  m_optionsGeom << QRectF(-0.1, 0.1, 0.30, 0.30);

  // for step number
  m_optionsGeom << QRectF(0, 0.45, 1, 0.30);
}

void
Menu00::draw(QMatrix4x4 mvp, QMatrix4x4 matL)
{
  if (!m_glTexture)
    genVertData();

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
  glUniform3f(rcShaderParm[3], 0, 0, 0); // view direction
  glUniform1f(rcShaderParm[4], opmod); // opacity modulator
  glUniform1i(rcShaderParm[5], 5); // texture + color
  glUniform1f(rcShaderParm[6], 5); // pointsize


  QVector3D vfront = 2*front;
  QVector3D vright = 2*right;
  QVector3D vleft = center - right;

  for(int og=0; og<5; og++)
    {
      float cx = m_optionsGeom[og].x();
      float cy = m_optionsGeom[og].y();
      float cwd = m_optionsGeom[og].width();
      float cht = m_optionsGeom[og].height();

      QVector3D v[4];  
      v[0] = vleft + cx*vright - cy*vfront;
      v[1] = v[0] - cht*vfront;
      v[2] = v[1] + cwd*vright;
      v[3] = v[2] + cht*vfront;
      
      for(int i=0; i<4; i++)
	{
	  m_vertData[8*i + 0] = v[i].x();
	  m_vertData[8*i + 1] = v[i].y();
	  m_vertData[8*i + 2] = v[i].z();
	  m_vertData[8*i+3] = 0;
	  m_vertData[8*i+4] = 0;
	  m_vertData[8*i+5] = 0;
	}
      
      float tx0 = 0;
      float ty0 = 0;
      float tx1 = 1;
      float ty1 = 1;

      if (og == 0)
	{
	  if (!m_play)
	    {
	      tx0 = m_relGeom[0].x()/(float)m_texWd;
	      tx1 = tx0 + m_relGeom[0].width()/(float)m_texWd;
	      ty0 = m_relGeom[0].y()/(float)m_texHt;
	      ty1 = ty0 + m_relGeom[0].height()/(float)m_texHt;
	    }
	  else
	    {
	      tx0 = m_relGeom[1].x()/(float)m_texWd;
	      tx1 = tx0 + m_relGeom[1].width()/(float)m_texWd;
	      ty0 = m_relGeom[1].y()/(float)m_texHt;
	      ty1 = ty0 + m_relGeom[1].height()/(float)m_texHt;
	    }
	}
      else if (og < 4) // not showing the current time step
	{
	  tx0 = m_relGeom[og+1].x()/(float)m_texWd;
	  tx1 = tx0 + m_relGeom[og+1].width()/(float)m_texWd;

	  ty0 = m_relGeom[og+1].y()/(float)m_texHt;
	  ty1 = ty0 + m_relGeom[og+1].height()/(float)m_texHt;
	}      
      else if (og == 4) // show step number
	{
	  //glActiveTexture(GL_TEXTURE4);
	  glBindTexture(GL_TEXTURE_2D, m_stepTexture);
	  //glEnable(GL_TEXTURE_2D);
	}

      float texC[] = {tx0,ty0, tx0,ty1, tx1,ty1, tx1,ty0};
      
      m_vertData[6] =  texC[0];  m_vertData[7] =  texC[1];
      m_vertData[14] = texC[2];  m_vertData[15] = texC[3];
      m_vertData[22] = texC[4];  m_vertData[23] = texC[5];
      m_vertData[30] = texC[6];  m_vertData[31] = texC[7];

      glBufferSubData(GL_ARRAY_BUFFER,
		      0,
		      sizeof(float)*8*4,
		      &m_vertData[0]);


      if (m_selected == og)
	glUniform3f(rcShaderParm[2], 0.5, 0.7, 1); // mix color
      else
	glUniform3f(rcShaderParm[2], 0, 0, 0); // mix color
      


      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);  
    }
  
  glBindVertexArray(0);
  glDisable(GL_TEXTURE_2D);
  
  glUseProgram( 0 );
  
  glDisable(GL_BLEND);
}

int
Menu00::checkOptions(QMatrix4x4 matL, QMatrix4x4 matR)
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

  QVector3D vleft = centerL - rightL;
  QVector3D vft = -2*frontL;
  QVector3D vrt = 2*rightL;


  QVector3D centerR = QVector3D(matR * QVector4D(0,0,0,1));
  QVector3D frontR;
  frontR = QVector3D(matR * QVector4D(0,0,-0.1,1)) - centerR;    


  float rw, rh;
  m_pointingToMenu = StaticFunctions::intersectRayPlane(vleft, vft, vrt,
							oupL.normalized(),
							centerR, frontR.normalized(),
							rh, rw);

  if (!m_pointingToMenu)
    return -1;

//  QVector3D pinPoint = centerR + frontR;
//
//  // project pinPoint onto map plane
//  QVector3D pp = pinPoint-vleft;
//  float rw = QVector3D::dotProduct(pp, vft.normalized());
//  float rh = QVector3D::dotProduct(pp, vrt.normalized());
//  float rz = QVector3D::dotProduct(pp, oupL.normalized());
//
//  if (qAbs(rz) > 0.05)  // not closer enough to the map
//    return m_selected;
//  
//  rh/=vrt.length();
//  rw/=vft.length();

  m_pinPt = vleft + rh*vrt + rw*vft;

  int tp = -1;
  for(int t=0; t<4; t++)
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

  return m_selected;
}

