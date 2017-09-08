#include "global.h"
#include "map.h"
#include "shaderfactory.h"
#include "staticfunctions.h"

#include <QFont>
#include <QColor>
#include <QtMath>

Map::Map() : QObject()
{
  m_glTexture = 0;
  m_glVertBuffer = 0;
  m_glIndexBuffer = 0;
  m_glVertArray = 0;
  m_vertData = 0;
  m_VBpoints = 0;

  m_bc = 0;

  m_teleportTouched = -1;
  m_teleports.clear();

  m_pinPt = QVector3D(-1000,-1000,-1000);

  m_mapScale = 0.3;

  m_visible = true;

  m_pointingToMenu = false;  
}

Map::~Map()
{
  if(m_vertData)
    {
      delete [] m_vertData;
      glDeleteBuffers(1, &m_glIndexBuffer);
      glDeleteVertexArrays( 1, &m_glVertArray );
      glDeleteBuffers(1, &m_glVertBuffer);
      glDeleteVertexArrays( 1, &m_glVA );
      glDeleteBuffers(1, &m_glVB);
      m_glIndexBuffer = 0;
      m_glVertArray = 0;
      m_glVertBuffer = 0;
      m_glVA = 0;
      m_glVB = 0;
    }
}

void
Map::setTeleports(QList<QVector3D> tp)
{
  m_teleports = tp;
}

void
Map::setImage(QImage img)
{
  m_texWd = img.width();
  m_texHt = img.height();
  
  if (!m_glTexture)
    glGenTextures(1, &m_glTexture);

  m_image = QImage(m_texWd, m_texHt, QImage::Format_ARGB32);
  QPainter p(&m_image);
  p.setBrush(QColor(0,0,0,180));
  p.setPen(QPen(Qt::yellow, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  //p.drawRoundedRect(1, 1, m_texWd-2, m_texHt-2, 50, 50);
  p.drawEllipse(1, 1, m_texWd-2, m_texHt-2);
  p.drawImage(0, 0, img.rgbSwapped());


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
	       m_image.bits());

  glDisable(GL_TEXTURE_2D);
}

void
Map::genOtherVertData()
{
  glGenVertexArrays(1, &m_glVA);
  glBindVertexArray(m_glVA);
  
  glGenBuffers(1, &m_glVB);
  glBindBuffer( GL_ARRAY_BUFFER, m_glVB );
  glBufferData( GL_ARRAY_BUFFER,
		sizeof(float)*1000,
		NULL,
		GL_STATIC_DRAW );

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
  
  glBindVertexArray( 0 );

  m_VBpoints = 0;
}

void
Map::genVertData()
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
  
  
  genOtherVertData();
  
  
  if (!m_glTexture)
    {
      QFont font = QFont("Helvetica", 48);
      QColor color(200, 220, 250); 
      QImage tmpTex = StaticFunctions::renderText("MENU",
						  font,
						  Qt::black, color);      
      m_texWd = tmpTex.width();
      m_texHt = tmpTex.height();
      
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
		   tmpTex.bits());
      
      glDisable(GL_TEXTURE_2D);
    }
}

void
Map::draw(QMatrix4x4 mvp, QMatrix4x4 matL)
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

  QVector3D vmid = center - front;
  QVector3D vleft = vmid - right - front;;
  QVector3D vft = 2*front;
  QVector3D vrt = 2*right;

  QVector3D ovmid = vmid;

  float opmod = qAbs(qBound(-0.1f, 0.0f, ofront.y()))/0.1f;


  //-----------------------------
  // rotate map based on view direction
  // only if any teleport is not touched
  //-----------------------------
  

  if (!m_vertData)
    genVertData();

  
  QVector3D frontRot = m_mapRot.rotatedVector(front);
  QVector3D rightRot = m_mapRot.rotatedVector(right);

  QVector3D v[4];

  QVector3D vleftRot = vmid - rightRot - frontRot;
  QVector3D vftRot = 2*frontRot;
  QVector3D vrtRot = 2*rightRot;

  v[0] = vleftRot;
  v[1] = v[0] + vftRot;
  v[2] = v[1] + vrtRot;
  v[3] = v[2] - vftRot;

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

  float texC[] = {tx1,ty0, tx0,ty0, tx0,ty1, tx1,ty1};
  m_vertData[6] =  texC[0];  m_vertData[7] =  texC[1];
  m_vertData[14] = texC[2];  m_vertData[15] = texC[3];
  m_vertData[22] = texC[4];  m_vertData[23] = texC[5];
  m_vertData[30] = texC[6];  m_vertData[31] = texC[7];


  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, m_glTexture);
  glEnable(GL_TEXTURE_2D);

  glBindVertexArray(m_glVertArray);
  glBindBuffer( GL_ARRAY_BUFFER, m_glVertBuffer);
  glBufferSubData(GL_ARRAY_BUFFER,
		  0,
		  sizeof(float)*8*4,
		  &m_vertData[0]);



  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  glUseProgram(ShaderFactory::rcShader());
  GLint *rcShaderParm = ShaderFactory::rcShaderParm();
  glUniformMatrix4fv(rcShaderParm[0], 1, GL_FALSE, mvp.data() );  
  glUniform1i(rcShaderParm[1], 4); // texture
  glUniform3f(rcShaderParm[2], 0, 0, 0); // mix color
  glUniform3f(rcShaderParm[3], 0, 0, 0); // view direction
  glUniform1f(rcShaderParm[4], opmod); // opacity modulator
  glUniform1i(rcShaderParm[5], 2); // applytexture
  glUniform1f(rcShaderParm[6], 5); // pointsize

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);  
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);  
  glBindVertexArray(0);

  glEnable(GL_PROGRAM_POINT_SIZE );
  glEnable(GL_POINT_SPRITE);

  // show current position and teleports
  {
    float rw = 1 - m_currPos.x()/m_texWd;
    float rh = m_currPos.y()/m_texHt;
    QVector3D vcpCP = vleftRot + rh*vrtRot + rw*vftRot;

    float rwd = 1 - m_currPosD.x()/m_texWd;
    float rhd = m_currPosD.y()/m_texHt;
    QVector3D vcpd = vleftRot + rhd*vrtRot + rwd*vftRot;

    float pvx = -(m_currPosD.y()-m_currPos.y())*0.3;
    float pvy = (m_currPosD.x()-m_currPos.x())*0.3;

    float rwp0 = 1 - (m_currPos.x()-pvx)/m_texWd;
    float rhp0 = (m_currPos.y()-pvy)/m_texHt;
    QVector3D vcpp0 = vleftRot + rhp0*vrtRot + rwp0*vftRot;

    float rwp1 = 1 - (m_currPos.x()+pvx)/m_texWd;
    float rhp1 = (m_currPos.y()+pvy)/m_texHt;
    QVector3D vcpp1 = vleftRot + rhp1*vrtRot + rwp1*vftRot;
    

    float vd[1000];
    memset(vd, 0, 1000*sizeof(float));

    int ofn = 0;
    vd[8*ofn+0] = vcpCP.x() + 0.05*up.x();
    vd[8*ofn+1] = vcpCP.y() + 0.05*up.y();
    vd[8*ofn+2] = vcpCP.z() + 0.05*up.z();    
    ofn++;

    vd[8*ofn+0] = vcpp0.x() + 0.05*up.x();
    vd[8*ofn+1] = vcpp0.y() + 0.05*up.y();
    vd[8*ofn+2] = vcpp0.z() + 0.05*up.z();
    ofn++;

    vd[8*ofn+0] = vcpd.x() + 0.05*up.x();
    vd[8*ofn+1] = vcpd.y() + 0.05*up.y();
    vd[8*ofn+2] = vcpd.z() + 0.05*up.z();
    ofn++;

    vd[8*ofn+0] = vcpp1.x() + 0.05*up.x();
    vd[8*ofn+1] = vcpp1.y() + 0.05*up.y();
    vd[8*ofn+2] = vcpp1.z() + 0.05*up.z();
    ofn++;

    int npts = ofn;
    for(int t=0; t<m_teleports.count(); t++)
      {
	float rw = 1 - m_teleports[t].x()/m_texWd;
	float rh = m_teleports[t].y()/m_texHt;
	QVector3D vcp = vleftRot + rh*vrtRot + rw*vftRot;

	vd[8*npts+0] = vcp.x() + 0.1*up.x();
	vd[8*npts+1] = vcp.y() + 0.1*up.y();
	vd[8*npts+2] = vcp.z() + 0.1*up.z();
	npts++;
      }

    int npts0 = npts;

    if (m_pinPt.x() > -2)
      {
	vd[8*npts+0] = m_pinPt.x() + 0.1*up.x();
	vd[8*npts+1] = m_pinPt.y() + 0.1*up.y();
	vd[8*npts+2] = m_pinPt.z() + 0.1*up.z();
	npts++;
      }
    int npts1 = npts;
    
    m_bc = (m_bc+1)%400;
    float g = qAbs(m_bc-200)*0.005;
	
    QVector3D vcpT;
    if (m_teleportTouched >= 0)
      {
	vd[8*npts+0] = vcpCP.x() + 0.05*up.x();
	vd[8*npts+1] = vcpCP.y() + 0.05*up.y();
	vd[8*npts+2] = vcpCP.z() + 0.05*up.z();
	vd[8*npts+6] = 0.0;
	vd[8*npts+7] = 0.0;
	npts++;

	float rw = 1 - m_teleports[m_teleportTouched].x()/m_texWd;
	float rh = m_teleports[m_teleportTouched].y()/m_texHt;
	vcpT = vleftRot + rh*vrtRot + rw*vftRot;

	vd[8*npts+0] = vcpT.x() + 0.05*up.x();
	vd[8*npts+1] = vcpT.y() + 0.05*up.y();
	vd[8*npts+2] = vcpT.z() + 0.05*up.z();
	vd[8*npts+6] = 1.0;
	vd[8*npts+7] = 1.0;
	npts++;
      }

    glBindVertexArray(m_glVA);
    glBindBuffer( GL_ARRAY_BUFFER, m_glVB);
    glBufferSubData(GL_ARRAY_BUFFER,
		    0,
		    sizeof(float)*npts*8,
		    &vd[0]);

    glLineWidth(1.0);
    glUniform3f(rcShaderParm[2], 1, 1, 1); // mix color

    glUniform1i(rcShaderParm[5], 1); // lines
    glDrawArrays(GL_LINE_LOOP, 0, ofn);  

    glUniform1i(rcShaderParm[5], 0); // points
    glDrawArrays(GL_POINTS, 0, 1);  

    if (npts-ofn > 0)
      {
	if (m_teleportTouched >= 0)
	  {	    
	    glLineWidth(5);
	    glUniform1f(rcShaderParm[4], opmod*0.5); // opacity modulator

	    // if teleport is behind paint red
	    if (QVector3D::dotProduct((vcpT-vcpCP).normalized(), front) > -0.05)
	      glUniform3f(rcShaderParm[2], 1, 0, 0); // mix color
	    else
	      glUniform3f(rcShaderParm[2], 1, 1, 1); // mix color

	    glUniform1i(rcShaderParm[5], 1); // lines
	    glDrawArrays(GL_LINES, npts1, 2);  
	  }

	if (m_pinPt.x() > -2)
	  {
	    glUniform1f(rcShaderParm[4], opmod*0.7); // opacity modulator
	    if (QVector3D::dotProduct((m_pinPt-vcpCP).normalized(), front) > -0.05)
	      glUniform3f(rcShaderParm[2], 1, 0, 0); // mix color
	    else
	      glUniform3f(rcShaderParm[2], 1, 1, 1); // mix color

	    glUniform1i(rcShaderParm[5], 0); // points
	    glDrawArrays(GL_POINTS, npts0, 1);  
	  }

	//-------------------------
	glEnable(GL_POINT_SPRITE);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, Global::homeSpriteTexture());
	glEnable(GL_TEXTURE_2D);
	//-------------------------

	glUniform1f(rcShaderParm[6], 10); // pointsize

	glUniform1i(rcShaderParm[5], 3); // apply teleport flags
	glDrawArrays(GL_POINTS, ofn, npts0-ofn);  

	glDisable(GL_POINT_SPRITE);
      }
  }

  glDisable(GL_TEXTURE_2D);

  glUseProgram( 0 );

  glDisable(GL_BLEND);
}

int
Map::checkTeleport(QMatrix4x4 matL, QMatrix4x4 matR)
{
  m_pointingToMenu = false;

  if (!m_visible)
    return -1;

  m_pinPt = projectPin(matL, matR);

  int ptt = m_teleportTouched;

  m_teleportTouched = -1;
  if (m_teleports.count() == 0)
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

  QVector3D vmidL = centerL - frontL;
  QVector3D vleftL = vmidL - rightL - frontL;;
  QVector3D vftL = 2*frontL;
  QVector3D vrtL = 2*rightL;


  float opmod = qAbs(qBound(-0.1f, 0.0f, ofrontL.y()))/0.1f;
  if (opmod < 0.1)
    return -1;


  //-----------------------------
  // rotate map based on view direction
  // only if any teleport is not touched
//  if (ptt < 0 && m_pinPt.x() < -2)
    {
      float rx = m_currPos.x()/m_texWd;
      float ry = m_currPos.y()/m_texHt;
      QVector3D vcp = vleftL + ry*vrtL + (1.0-rx)*vftL;
      
      float rxd = m_currPosD.x()/m_texWd;
      float ryd = m_currPosD.y()/m_texHt;
      QVector3D vcpd = vleftL + ryd*vrtL + (1.0-rxd)*vftL;
      
      QVector3D vcd = vcpd - vcp;
      vcd.normalize();
      float angle = StaticFunctions::getAngleBetweenVectors(vcd, -ofrontL);
      QVector3D axis = QVector3D::normal(vcd, -ofrontL);
      if (QVector3D::dotProduct(axis, oupL) > 0)
	axis = oupL;
      else
	axis = -oupL;
      m_mapRot = QQuaternion::fromAxisAndAngle(axis, qRadiansToDegrees(angle));
    }
  //-----------------------------

  QVector3D frontRot = m_mapRot.rotatedVector(frontL);
  QVector3D rightRot = m_mapRot.rotatedVector(rightL);
  
  QVector3D vleftRot = vmidL - rightRot - frontRot;
  QVector3D vftRot = 2*frontRot;
  QVector3D vrtRot = 2*rightRot;


  QVector3D centerR = QVector3D(matR * QVector4D(0,0,0,1));
  QVector3D frontR;
  frontR = QVector3D(matR * QVector4D(0,0,-0.1,1)) - centerR;    

  QVector3D pinPoint = centerR + frontR;

  int tp = -1;
  float ndist = 1;
  for(int t=0; t<m_teleports.count(); t++)
    {
      float rx = 1 - m_teleports[t].x()/m_texWd;
      float ry = m_teleports[t].y()/m_texHt;
      QVector3D vcp = vleftRot + ry*vrtRot + rx*vftRot;
      
      float dtl = vcp.distanceToLine(pinPoint, frontR);
      if (dtl < 0.02 && ndist > dtl)
	{	      
	  tp = t;
	  ndist = dtl;
	}
    }

  m_teleportTouched = tp;


//  m_pointingToMenu = StaticFunctions::intersectRayPlane(vleftRot, vftRot, vrtRot,
//							  oupL.normalized(),
//							  centerR, frontR.normalized());


  return tp;
}

QVector3D
Map::projectPin(QMatrix4x4 matL, QMatrix4x4 matR)
{
  m_pinPt2D = QVector2D(-1000,-1000);

  QVector3D centerL = QVector3D(matL * QVector4D(0,0,0,1));
  QVector3D ofrontL, orightL, oupL;
  QVector3D frontL, rightL, upL;

  ofrontL = QVector3D(matL * QVector4D(0,0,1,1)) - centerL; 
  oupL = QVector3D(matL * QVector4D(0,1,0,1)) - centerL;
  orightL = QVector3D(matL * QVector4D(1,0,0,1)) - centerL;

  frontL = m_mapScale * ofrontL;
  rightL = m_mapScale * orightL;
  upL = 0.05 * oupL;

  QVector3D vmidL = centerL - frontL;
  QVector3D vleftL = vmidL - rightL - frontL;;
  QVector3D vftL = 2*frontL;
  QVector3D vrtL = 2*rightL;


  float opmod = qAbs(qBound(-0.1f, 0.0f, ofrontL.y()))/0.1f;
  if (opmod < 0.1)
    return QVector3D(-1000,-1000,-1000);
		     

  //-----------------------------
  // rotate map based on view direction
  // only if any teleport is not touched
//  if (m_teleportTouched < 0 && m_pinPt.x() < -2)
    {
      float rx = m_currPos.x()/m_texWd;
      float ry = m_currPos.y()/m_texHt;
      QVector3D vcp = vleftL + ry*vrtL + (1.0-rx)*vftL;
      
      float rxd = m_currPosD.x()/m_texWd;
      float ryd = m_currPosD.y()/m_texHt;
      QVector3D vcpd = vleftL + ryd*vrtL + (1.0-rxd)*vftL;
      
      QVector3D vcd = vcpd - vcp;
      vcd.normalize();
      float angle = StaticFunctions::getAngleBetweenVectors(vcd, -ofrontL);
      QVector3D axis = QVector3D::normal(vcd, -ofrontL);
      if (QVector3D::dotProduct(axis, oupL) > 0)
	axis = oupL;
      else
	axis = -oupL;
      m_mapRot = QQuaternion::fromAxisAndAngle(axis, qRadiansToDegrees(angle));
    }
  //-----------------------------

  QVector3D frontRot = m_mapRot.rotatedVector(frontL);
  QVector3D rightRot = m_mapRot.rotatedVector(rightL);
  
  QVector3D vleftRot = vmidL - rightRot - frontRot;
  QVector3D vftRot = 2*frontRot;
  QVector3D vrtRot = 2*rightRot;


  QVector3D centerR = QVector3D(matR * QVector4D(0,0,0,1));
  QVector3D frontR = QVector3D(matR * QVector4D(0,0,-0.1,1)) - centerR;    

  //-------------------
  float rw, rh;
  m_pointingToMenu = StaticFunctions::intersectRayPlane(vleftRot, vftRot, vrtRot,
							oupL.normalized(),
							centerR, frontR.normalized(),
							rh, rw);
  if (!m_pointingToMenu)
    return QVector3D(-1000,-1000,-1000);

//  QVector3D pinPoint = centerR + frontR;
//  QVector3D pp = pinPoint-vleftRot;
//  float rz = QVector3D::dotProduct(pp, oupL.normalized());
//  if (qAbs(rz) > 0.04)  // not closer enough to the map
//    return QVector3D(-1000,-1000,-1000);

  m_pinPt2D = QVector2D(rw, rh);
  return (vleftRot + rh*vrtRot + rw*vftRot);

  //-------------------

//  QVector3D pinPoint = centerR + frontR;
//
//
//  // project pinPoint onto map plane
//  QVector3D pp = pinPoint-vleftRot;
//  QVector3D rw = QVector3D::dotProduct(pp, vftRot.normalized());
//  QVector3D rh = QVector3D::dotProduct(pp, vrtRot.normalized());
//  float rz = QVector3D::dotProduct(pp, oupL.normalized());
//
//  if (qAbs(rz) > 0.04 ||  // not closer enough to the map
//      rw < 0 || rw > vftRot.length() || // not within the map bounds
//      rh < 0 || rh > vrtRot.length())
//    return QVector3D(-1000,-1000,-1000);
//
//  rh/=vrtRot.length();
//  rw/=vftRot.length();
//
//  m_pinPt2D = QVector2D(rw, rh);
//
//  return (vleftRot + rh*vrtRot + rw*vftRot);
}

