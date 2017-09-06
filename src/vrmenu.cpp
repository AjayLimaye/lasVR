#include "vrmenu.h"
#include <QMessageBox>

VrMenu::VrMenu() : QObject()
{
  m_menus["00"] = new Map();
  m_menus["01"] = new Menu00();
  m_menus["02"] = new Menu01();

  m_currMenu = "none";

  connect(m_menus["02"], SIGNAL(resetModel()),
	  this, SIGNAL(resetModel()));
  connect(m_menus["02"], SIGNAL(updatePointSize(int)),
	  this, SIGNAL(updatePointSize(int)));
  connect(m_menus["02"], SIGNAL(updateScale(int)),
	  this, SIGNAL(updateScale(int)));
  connect(m_menus["02"], SIGNAL(updateSoftShadows(bool)),
	  this, SIGNAL(updateSoftShadows(bool)));
  connect(m_menus["02"], SIGNAL(updateEdges(bool)),
	  this, SIGNAL(updateEdges(bool)));
}

VrMenu::~VrMenu()
{
}

void
VrMenu::setTimeStep(QString stpStr)
{
  ((Menu00*)(m_menus["01"]))->setTimeStep(stpStr);
}

QVector2D
VrMenu::pinPoint2D()
{
  return ((Map*)(m_menus["00"]))->pinPoint2D();
}

void
VrMenu::setCurrPos(QVector3D v, QVector3D vd)
{
  m_currPos = v;
  m_currPosD = vd;

  ((Map*)(m_menus["00"]))->setCurrPos(v, vd);
}

void
VrMenu::setTeleports(QList<QVector3D> tp)
{
  m_teleports = tp;
  ((Map*)(m_menus["00"]))->setTeleports(tp);
}

void
VrMenu::setImage(QImage img)
{
  ((Map*)(m_menus["00"]))->setImage(img);
}

void
VrMenu::draw(QMatrix4x4 mvp, QMatrix4x4 matL, bool triggerPressed)
{
  if (m_currMenu == "00")
    ((Map*)(m_menus[m_currMenu]))->draw(mvp, matL);
  if (m_currMenu == "01")
    ((Menu00*)(m_menus[m_currMenu]))->draw(mvp, matL);
  if (m_currMenu == "02")
    ((Menu01*)(m_menus[m_currMenu]))->draw(mvp, matL, triggerPressed);
}

int
VrMenu::checkTeleport(QMatrix4x4 matL, QMatrix4x4 matR)
{
  if (m_currMenu == "00")
    return ((Map*)(m_menus["00"]))->checkTeleport(matL, matR);
  else
    return -1;
}

int
VrMenu::checkOptions(QMatrix4x4 matL, QMatrix4x4 matR, bool triggered)
{
//  if (((Menu00*)(m_menus["01"]))->isVisible())
  if (m_currMenu == "01")
    return ((Menu00*)(m_menus[m_currMenu]))->checkOptions(matL, matR);

  if (m_currMenu == "02")
    ((Menu01*)(m_menus[m_currMenu]))->checkOptions(matL, matR, triggered);
  
  return -1;
}

void
VrMenu::setPlay(bool p)
{
  ((Menu00*)(m_menus["01"]))->setPlay(p);
}

void
VrMenu::setCurrentMenu(QString m)
{
  m_currMenu = m;

  ((Map*)(m_menus["00"]))->setVisible(false);
  ((Menu00*)(m_menus["01"]))->setVisible(false);
  ((Menu01*)(m_menus["02"]))->setVisible(false);

  if (m_currMenu == "00")
    ((Map*)(m_menus["00"]))->setVisible(true);

  if (m_currMenu == "01")
    ((Menu00*)(m_menus["01"]))->setVisible(true);

  if (m_currMenu == "02")
    ((Menu01*)(m_menus["02"]))->setVisible(true);
}

bool
VrMenu::pointingToMenu()
{
  bool pm = false;
  
  pm = pm || ((Map*)(m_menus["00"]))->pointingToMenu();
  pm = pm || ((Menu00*)(m_menus["01"]))->pointingToMenu();
  pm = pm || ((Menu01*)(m_menus["02"]))->pointingToMenu();

  return pm;
}
