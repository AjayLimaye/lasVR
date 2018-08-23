#include "global.h"
#include "menu02.h"
#include "shaderfactory.h"
#include "staticfunctions.h"
#include "iconlibrary.h"

#include <QFont>
#include <QColor>
#include <QtMath>
#include <QMessageBox>
#include <QApplication>
#include <QDir>

Menu02::Menu02() : QObject()
{
  m_stepTexture = 0;
  m_glTexture = 0;
  m_glVertBuffer = 0;
  m_glIndexBuffer = 0;
  m_glVertArray = 0;
  m_vertData = 0;
  m_textData = 0;

  m_menuScale = 0.3;
  //m_menuDist = 0.5;
  m_menuDist = 0.1;
  
  m_menuList.clear();

  m_relGeom.clear();
  m_optionsGeom.clear();

  m_selected = -1;

  m_visible = true;
  m_pointingToMenu = false;
    
  m_icons.clear();
  m_buttons.clear();
}

Menu02::~Menu02()
{
  if(m_vertData)
    {
      delete [] m_vertData;
      delete [] m_textData;
      m_vertData = 0;
      m_textData = 0;
      glDeleteBuffers(1, &m_glIndexBuffer);
      glDeleteVertexArrays( 1, &m_glVertArray );
      glDeleteBuffers(1, &m_glVertBuffer);
      m_glIndexBuffer = 0;
      m_glVertArray = 0;
      m_glVertBuffer = 0;
    }

  m_buttons.clear();
}

float
Menu02::value(QString name)
{
  for (int i=0; i<m_buttons.count(); i++)
    {
      QString attr = m_buttons[i].text().toLower().trimmed();
      if (attr == name)
      {
	return m_buttons[i].value();
      }
    }
  return -1000000;
}

void
Menu02::setValue(QString name, float val)
{
  for (int i=0; i<m_buttons.count(); i++)
    {
      QString attr = m_buttons[i].text().toLower().trimmed();
      if (attr == name)
      {
	m_buttons[i].setValue(val);
	break;
      }
    }
}

void
Menu02::setIcons(QStringList icl)
{
  m_icons = icl;
  genVertData();
}

void
Menu02::genVertData()
{
  // create and bind a VAO to hold state for this model
  if (!m_glVertArray)
    glGenVertexArrays( 1, &m_glVertArray );
  glBindVertexArray( m_glVertArray );
      
  // Populate a vertex buffer
  if (!m_glVertBuffer)
    {
      glGenBuffers( 1, &m_glVertBuffer );
      glBindBuffer( GL_ARRAY_BUFFER, m_glVertBuffer );
      glBufferData( GL_ARRAY_BUFFER,
		    sizeof(float)*8*4*2,
		    NULL,
		    GL_STATIC_DRAW );


      m_vertData = new float[32];
      memset(m_vertData, 0, 32*sizeof(float));  
      m_vertData[6] = 0.0;
      m_vertData[7] = 0.0;
      m_vertData[14] = 0.0;
      m_vertData[15] = 1.0;
      m_vertData[22] = 1.0;
      m_vertData[23] = 1.0;
      m_vertData[30] = 1.0;
      m_vertData[31] = 0.0;
      
      m_vertData[0] =  0;  m_vertData[1] = 0;  m_vertData[2] = 0;
      m_vertData[8] =  0;  m_vertData[9] = 0; m_vertData[10] = -1;
      m_vertData[16] = 1; m_vertData[17] = 0; m_vertData[18] = -1;
      m_vertData[24] = 1; m_vertData[25] = 0; m_vertData[26] = 0;
      
      glBufferSubData(GL_ARRAY_BUFFER,
		      0,
		      sizeof(float)*8*4,
		      &m_vertData[0]);
    }
  else
    glBindBuffer( GL_ARRAY_BUFFER, m_glVertBuffer );
      

  if (!m_textData)
    {
      m_textData = new float[32];
      memset(m_textData, 0, sizeof(float)*32);
    }

  // Identify the components in the vertex buffer
  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 0, //attribute 0
			 3, // size2
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
  
  
  
  // Create and populate the index buffer
  if (!m_glIndexBuffer)
    {
      uchar indexData[6];
      indexData[0] = 0;
      indexData[1] = 1;
      indexData[2] = 2;
      indexData[3] = 0;
      indexData[4] = 2;
      indexData[5] = 3;
  
      glGenBuffers( 1, &m_glIndexBuffer );
      glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer );
      glBufferData( GL_ELEMENT_ARRAY_BUFFER,
		    sizeof(uchar) * 6,
		    &indexData[0],
		    GL_STATIC_DRAW );
    }
  else
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer );
  
  glBindVertexArray( 0 );
    

  if (m_relGeom.count() == 0)
    {
      m_texWd = IconLibrary::width();
      m_texHt = IconLibrary::height();

      m_relGeom = IconLibrary::iconShape();
      m_optionsGeom = IconLibrary::iconGeometry();
    }	

//  if (m_relGeom.count() == 0)
//    {
//      QFont font = QFont("Helvetica", 48);
//      QColor color(250, 230, 210); 
//      
//      m_texWd = 520;
//      m_texHt = (m_icons.count()/5)*100 + 20;
//      if (m_icons.count()%5 > 0) m_texHt += 100;
//      int flipTexHt = m_texHt-10;
//
//      QImage pimg = QImage(m_texWd, m_texHt, QImage::Format_ARGB32);
//      pimg.fill(Qt::black);
//
//      QPainter p(&pimg);      
//      p.setBrush(Qt::black);
//      QPen ppen(Qt::white);
//      ppen.setWidth(5);
//      p.setPen(ppen);
//      
//
//      p.drawRoundedRect(5,5,m_texWd-10,m_texHt-10, 5, 5);
//
//      if (m_icons.count() == 0)
//	{
//	  QImage img = StaticFunctions::renderText("NO ANNOTATION ICONS FOUND",
//						   font,
//						   Qt::black, Qt::white, true);
//
//	  img = img.scaledToHeight(90, Qt::SmoothTransformation);
//	  int twd = img.width();
//	  int tht = img.height();	  
//	  int ws = (m_texWd-twd)/2;
//	  int hs = (m_texHt-tht)/2;
//	  p.drawImage(ws, hs, img.mirrored(false,true));
//	}
//      
//      //-------------------
//      //-------------------
//      QString icondir = qApp->applicationDirPath() +	\
//                        QDir::separator() + "assets" + \
//                        QDir::separator() + "annotation_icons";
//      QDir idir(icondir);
//      for(int i=0; i<m_icons.count(); i++)
//	{
//	  // 3 icons per row
//	  int r = i/5;
//	  int c = i%5;
//	  m_relGeom << QRect(15 + c*100, 15 + r*100, 90, 90);
//
//	  QImage iconImage = QImage(idir.absoluteFilePath(m_icons[i])).	\
//	                     scaledToHeight(70, Qt::SmoothTransformation).\
//	                     mirrored(false,true);
//	    
//	  p.drawImage(25 + c*100,
//		      25 + r*100,
//		      iconImage);
//	}
//      //-------------------
//      //-------------------
//      
//      if (!m_glTexture)
//	glGenTextures(1, &m_glTexture);
//  
//      glActiveTexture(GL_TEXTURE4);
//      glBindTexture(GL_TEXTURE_2D, m_glTexture);
//      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
//      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
//      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//      glTexImage2D(GL_TEXTURE_2D,
//		   0,
//		   4,
//		   m_texWd,
//		   m_texHt,
//		   0,
//		   GL_RGBA,
//		   GL_UNSIGNED_BYTE,
//		   pimg.bits());
//  
//      glDisable(GL_TEXTURE_2D);
//    }
//
//  m_optionsGeom.clear();
//  for (int i=0; i<m_relGeom.count(); i++)
//    {
//       float cx = m_relGeom[i].x();
//       float cy = m_relGeom[i].y();
//      float cwd = m_relGeom[i].width();
//      float cht = m_relGeom[i].height();
//
//      m_optionsGeom << QRectF(cx/(float)m_texWd,
//			      cy/(float)m_texHt,
//			      cwd/(float)m_texWd,
//			      cht/(float)m_texHt);
//    }

  //-----------------------
  //-----------------------
  m_buttons.clear();
  
  for(int i=0; i<m_optionsGeom.count(); i++) // add buttons
    {
      Button button;
      button.setType(0); // simple pushbutton
      button.setRect(m_relGeom[i]);
      button.setGeom(m_optionsGeom[i]);
      button.setText("icon");
      m_buttons << button;      
    }
  //-----------------------
  //-----------------------
  
  
  float twd = m_texWd;
  float tht = m_texHt;
  float mx = qMax(twd, tht);
  twd/=mx; tht/=mx;
  {
    //float frc = 0.075/tht;
    float frc = 0.1/tht;
    twd *= frc;
    tht *= frc;
  }
  m_menuMat.setToIdentity();
  m_menuMat.translate(-twd/2, 0, 0);
  m_menuMat.scale(twd, 1, tht);
}

void
Menu02::draw(QMatrix4x4 mvpC, QMatrix4x4 matL, bool triggerPressed)
{
  if (!m_visible)
    return;

  if (!m_vertData)
    genVertData();

  QMatrix4x4 mvp = mvpC * matL * m_menuMat;

  glBindTextureUnit(5, IconLibrary::iconTexture());
  //glBindTextureUnit(4, m_glTexture);
  //glEnable(GL_TEXTURE_2D);

  glBindVertexArray(m_glVertArray);
  glBindBuffer( GL_ARRAY_BUFFER, m_glVertBuffer);

  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 0, //attribute 0
			 3, // size2
			 GL_FLOAT, // type
			 GL_FALSE, // not normalized
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

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);  
  
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);


  glUseProgram(ShaderFactory::rcShader());
  GLint *rcShaderParm = ShaderFactory::rcShaderParm();
  glUniformMatrix4fv(rcShaderParm[0], 1, GL_FALSE, mvp.data() );  
  glUniform1i(rcShaderParm[1], 5); // texture
  glUniform3f(rcShaderParm[2], 0, 0, 0); // color
  glUniform3f(rcShaderParm[3], 0, 0, 0); // view direction
  glUniform1f(rcShaderParm[4], 0.9); // opacity modulator
  glUniform1i(rcShaderParm[5], 5); // texture + color
  glUniform1f(rcShaderParm[6], 5); // pointsize

  glUniform1f(rcShaderParm[7], 0.5); // mixcolor

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);  
    
  // now change for drawing text/icons etc
  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 0, //attribute 0
			 3, // size2
			 GL_FLOAT, // type
			 GL_FALSE, // normalized
			 sizeof(float)*8, // stride
			 (char *)NULL + sizeof(float)*8*4 ); // starting offset

  glEnableVertexAttribArray( 1 );
  glVertexAttribPointer( 1,
			 3,
			 GL_FLOAT,
			 GL_FALSE,
			 sizeof(float)*8,
			 (char *)NULL + sizeof(float)*8*4 + sizeof(float)*3 );
  
  glEnableVertexAttribArray( 2 );
  glVertexAttribPointer( 2,
			 2,
			 GL_FLOAT,
			 GL_FALSE, 
			 sizeof(float)*8,
			 (char *)NULL + sizeof(float)*8*4 + sizeof(float)*6 );

  QMatrix4x4 mMat;
  mMat = matL * m_menuMat;
  QVector3D vleft = mMat.map(QVector3D(0,0,0));
  QVector3D vtop = (mMat.map(QVector3D(0,0,-1)) - vleft).normalized();
  QVector3D vright = (mMat.map(QVector3D(1,0,0)) - vleft).normalized();
  QVector3D vup = (mMat.map(QVector3D(0,1,0)) - vleft).normalized();

  //------------------
  for(int b=0; b<m_buttons.count(); b++)
    {
      QRectF geom = m_buttons[b].geom();
      
      float cx = geom.x();
      float cy = geom.y();
      float cwd = geom.width();
      float cht = geom.height();

      Vec mc = Vec(0,0,0);
	  
      if (m_buttons[b].type() != 2) // not a slider
	{
	  if (m_buttons[b].value() > m_buttons[b].minValue())
	    mc = Vec(0.5, 1.0, 0.5);
      
	  if (b == m_selected)
	    {
	      if (triggerPressed)
		mc = Vec(0.2, 0.8, 0.2);
	      else
		mc = Vec(0.5, 0.7, 1);
	    }
	  
	  
	  if (mc.x > 0.0)	    
	    showText(geom,
		     vleft, vright, vtop, vup,
		     cx, cx+cwd, cy, cy+cht,
		     mc);
	}
    }
  //------------------
  
  // ----
  // show pinPt
  if (m_pinPt.x() > -1000)
    {
      glBlendFunc(GL_ONE, GL_ONE);
      glBindTextureUnit(5, Global::circleSpriteTexture());
      glEnable(GL_PROGRAM_POINT_SIZE );
      glEnable(GL_POINT_SPRITE);
      m_textData[0] = m_pinPt.x();
      m_textData[1] = m_pinPt.y();
      m_textData[2] = m_pinPt.z();
      glBufferSubData(GL_ARRAY_BUFFER,
		      //0,
		      sizeof(float)*8*4, // offset
		      sizeof(float)*3,
		      &m_textData[0]);
      glUniform3f(rcShaderParm[2], 1, 1, 0); // mix color
      glUniform1i(rcShaderParm[5], 3); // point sprite
      glDrawArrays(GL_POINTS, 0, 1);
      glDisable(GL_POINT_SPRITE);
    }
  // ----

  glBindVertexArray(0);
  //glDisable(GL_TEXTURE_2D);
  
  glUseProgram( 0 );
  
  glDisable(GL_BLEND);

  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
}

void
Menu02::showText(QRectF geom,
		 QVector3D vleft, QVector3D vright,
		 QVector3D vtop, QVector3D vup,
		 float tx0, float tx1, float ty0, float ty1,
		 Vec mc)
{
  GLint *rcShaderParm = ShaderFactory::rcShaderParm();
  glUniform3f(rcShaderParm[2], mc.x, mc.y, mc.z); // mix color

  float cx =  geom.x();
  float cy =  geom.y();
  float cwd = geom.width();
  float cht = geom.height();

  QVector3D v[4];  
  v[0] = QVector3D(cx, 0.002, -cy); // slightly raised
  v[1] = v[0] + QVector3D(0,0,-cht);
  v[2] = v[0] + QVector3D(cwd,0,-cht);
  v[3] = v[0] + QVector3D(cwd,0,0);
  
  for(int i=0; i<4; i++)
    {
      m_textData[8*i + 0] = v[i].x();
      m_textData[8*i + 1] = v[i].y();
      m_textData[8*i + 2] = v[i].z();
      m_textData[8*i+3] = 0;
      m_textData[8*i+4] = 0;
      m_textData[8*i+5] = 0;
    }
  
  float texD[] = {tx0,ty0, tx0,ty1, tx1,ty1, tx1,ty0};
	  
  m_textData[6] =  texD[0];  m_textData[7] =  texD[1];
  m_textData[14] = texD[2];  m_textData[15] = texD[3];
  m_textData[22] = texD[4];  m_textData[23] = texD[5];
  m_textData[30] = texD[6];  m_textData[31] = texD[7];
  
  glBufferSubData(GL_ARRAY_BUFFER,
		  //0,
		  sizeof(float)*8*4, // offset
		  sizeof(float)*8*4,
		  &m_textData[0]);
  
  
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);  
}

int
Menu02::checkOptions(QMatrix4x4 matL, QMatrix4x4 matR, int triggered)
{
  m_pinPt = QVector3D(-1000,-1000,-1000);
  m_pointingToMenu = false;
  m_selected = -1;

  QMatrix4x4 mMat;
  mMat = matL * m_menuMat;
  QVector3D vleft = mMat.map(QVector3D(0,0,0));
  QVector3D vtop = mMat.map(QVector3D(0,0,-1)) - vleft;
  QVector3D vright = mMat.map(QVector3D(1,0,0)) - vleft;
  QVector3D vup = (mMat.map(QVector3D(0,1,0)) - vleft).normalized();
    
  //---------
  // reset all the grabs and return
  if (triggered == 3)
    {
      for(int b=0; b<m_buttons.count(); b++)
	m_buttons[b].setGrabbed(false);
      return -1;
    }
  //---------
  
  if (!m_visible || m_optionsGeom.count()==0)
    return -1;


  QVector3D centerR = QVector3D(matR * QVector4D(0,0,0,1));
  QVector3D frontR;
  frontR = QVector3D(matR * QVector4D(0,0,-0.1,1)) - centerR;    

  
  float rw, rh;
  m_pointingToMenu = StaticFunctions::intersectRayPlane(vleft,
							vtop,
							vright,
							vup,
							centerR, frontR.normalized(),
							rw, rh);

  if (!m_pointingToMenu)
    return -1;

  m_pinPt = QVector3D(rw, 0, -rh);

  int tp = -1;

  for(int b=0; b<m_buttons.count(); b++)
    {
      if (m_buttons[b].inBox(rw, rh))
	{
	  tp = b;
	  break;
	}
    }

  m_selected = tp;
  
  if (m_selected < 0)
    return m_selected;
    
  if (triggered == 2) // released
    {
      if (!m_buttons[m_selected].on())
	{ // switch off any other switched on buttons
	  for(int b=0; b<m_buttons.count(); b++)
	    if (m_buttons[b].on())
	      m_buttons[b].toggle();
	}

      m_buttons[m_selected].toggle();

      return m_selected;
    }

  return m_selected;
}

QString
Menu02::currentIcon()
{
  for(int b=0; b<m_buttons.count(); b++)
    if (m_buttons[b].on())
      return m_icons[b];

  // if no icon is selected
  return QString();
}
