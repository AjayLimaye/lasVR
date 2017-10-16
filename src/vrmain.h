#ifndef VRMAIN_H
#define VRMAIN_H

#include "viewer.h"
#include "ui_vrmain.h"
#include "volumefactory.h"
#include "glhiddenwidget.h"
#include "loaderthread.h"
#include "popupslider.h"
#include "keyframeeditor.h"
#include "keyframe.h"

#include <QThread>
#include <QDockWidget>

class VrMain : public QMainWindow
{
  Q_OBJECT

 public :
  VrMain(QWidget *parent=0);

 signals :
  void playKeyFrames(int, int, int);

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

   void on_actionEditMode_triggered();
   void on_actionCamMode_triggered();
   void on_actionAlign_triggered();
   void on_actionUndo_triggered();
   void on_actionSaveInfo_triggered();
   void on_actionVRMode_triggered();

   void on_actionSave_ImageSequence_triggered();
   void on_actionSave_Movie_triggered();

   void on_actionSave_Keyframes_triggered();
   void on_actionLoad_Keyframes_triggered();

 private :
  Ui::VrMain ui;

  Viewer *m_viewer;

  GLHiddenWidget *m_hiddenGL;
  LoaderThread *m_lt;
  QThread *m_thread;

  VolumeFactory m_volumeFactory;

  QString m_prevDir;

  PopUpSlider *m_pointBudget;
  PopUpSlider *m_timeStep;

  QDockWidget *m_dockKeyframe;
  KeyFrameEditor *m_keyFrameEditor;
  KeyFrame *m_keyFrame;

  void loadTiles(QStringList);
};

#endif
