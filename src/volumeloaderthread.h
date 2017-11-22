#ifndef VOLUMELOADERTHREAD_H
#define VOLUMELOADERTHREAD_H

#include "triset.h"
#include "pointcloud.h"

class VolumeLoaderThread : public QObject
{
  Q_OBJECT

 public :
  VolumeLoaderThread();

  void setTrisets(QList<Triset*>);
  void setPointClouds(QList<PointCloud*>);

 public slots :
   void startLoading();
   void stopLoading();
       
 private :
   QList<Triset*> m_trisets;
   QList<PointCloud*> m_pointClouds;
   bool m_stopLoading;
   bool m_loading;
};


#endif
