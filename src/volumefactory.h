#ifndef VOLUMEFACTORY_H
#define VOLUMEFACTORY_H

#include <QStack>
#include "volume.h"

class VolumeFactory
{
 public :
  VolumeFactory();
  ~VolumeFactory();

  void reset();
  
  Volume* newVolume();

  Volume* topVolume();

  void pushVolume(Volume*);
  Volume* popVolume();

  int stackSize() { return m_volumeStack.count(); }

  Volume* checkVolume(QString);

 private :
  QList<Volume*> m_volumes;

  QStack<Volume*> m_volumeStack;

};

#endif
