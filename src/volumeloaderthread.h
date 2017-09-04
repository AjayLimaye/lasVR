#ifndef VOLUMELOADERTHREAD_H
#define VOLUMELOADERTHREAD_H

#include "pointcloud.h"

class VolumeLoaderThread : public QObject
{
  Q_OBJECT

 public :
  VolumeLoaderThread();

  void setPointClouds(QList<PointCloud*>);

 public slots :
   void startLoading();
   void stopLoading();
       
 private :
   QList<PointCloud*> m_pointClouds;
   bool m_stopLoading;
   bool m_loading;
};


#endif
