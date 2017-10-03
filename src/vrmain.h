#ifndef VRMAIN_H
#define VRMAIN_H

#include "viewer.h"
#include "ui_vrmain.h"
#include "volumefactory.h"
#include "glhiddenwidget.h"
#include "loaderthread.h"
#include "popupslider.h"

#include <QThread>

class VrMain : public QMainWindow
{
  Q_OBJECT

 public :
  VrMain(QWidget *parent=0);

 protected :
  void dragEnterEvent(QDragEnterEvent*);
  void dropEvent(QDropEvent*);
  void closeEvent(QCloseEvent*);
  void keyPressEvent(QKeyEvent*);

 private slots :
   void on_actionLoad_LAS_triggered();
   void on_actionQuit_triggered();
   void on_actionFly_triggered();
   void showFramerate(float);
   void showMessage(QString);

 private :
  Ui::VrMain ui;

  Viewer *m_viewer;

  GLHiddenWidget *m_hiddenGL;
  LoaderThread *m_lt;
  QThread *m_thread;

  VolumeFactory m_volumeFactory;

  QString m_prevDir;

  PopUpSlider *m_pointBudget;

  void loadTiles(QStringList);
};

#endif
