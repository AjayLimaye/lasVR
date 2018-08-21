TEMPLATE = app
TARGET = srishti
DEPENDPATH += .

QT += opengl widgets core gui xml

CONFIG += release
DESTDIR = ..\bin

RESOURCES = las.qrc

INCLUDEPATH += c:\Qt\libQGLViewer-2.6.1 \
 	c:\cygwin64\home\acl900\drishtilib\glew-1.11.0\include \
	LASzip \
	c:/cygwin64/home/acl900/VR/openvr-1.0.10/headers
 
QMAKE_LIBDIR += c:\Qt\libQGLViewer-2.6.1\lib \
		c:\cygwin64\home\acl900\drishtilib\glew-1.11.0\lib\Release\x64 \
	        LASzip \
	        c:/cygwin64/home/acl900/VR/openvr-1.0.10/lib/win64

win32 {
  DEFINES += USE_GLMEDIA
  INCLUDEPATH += glmedia-64
  QMAKE_LIBDIR += glmedia-64
  LIBS += glmedia.lib
}

LIBS += QGLViewer2.lib glew32.lib LASzip.lib openvr_api.lib 

FORMS += vrmain.ui \
	 saveimgseq.ui \
	 savemovie.ui \
	 propertyeditor.ui


INCLUDEPATH +=	laszip_dll.h


HEADERS += vrmain.h \
	volumefactory.h \
	volume.h \
	global.h \
	viewer.h \
	glewinitialisation.h \
	shaderfactory.h \
	boundingbox.h \
	mymanipulatedframe.h \
	octreenode.h \
	glhiddenwidget.h \
	loaderthread.h \
	volumeloaderthread.h \
	pointcloud.h \
	staticfunctions.h \
	label.h \
	vr.h \
	cglrendermodel.h \
	vrmenu.h \
	map.h \
        menu01.h \
        menu02.h \
	cubemap.h \
	popupslider.h \
	keyframe.h \
	keyframeinformation.h \
	keyframeeditor.h \
	saveimageseqdialog.h \
	savemoviedialog.h \
        propertyeditor.h \
        ply.h \
        triset.h \
        captionwidget.h \
        button.h \
        iconlibrary.h


SOURCES += main.cpp \
	vrmain.cpp \
	volumefactory.cpp \
	volume.cpp \
	global.cpp \
	viewer.cpp \
	glewinitialisation.cpp \
	shaderfactory.cpp \
	boundingbox.cpp \
	mymanipulatedframe.cpp \
	octreenode.cpp \
	glhiddenwidget.cpp \
	loaderthread.cpp \
	volumeloaderthread.cpp \
	pointcloud.cpp \
	staticfunctions.cpp \
	label.cpp \
	vr.cpp \
	cglrendermodel.cpp \
	vrmenu.cpp \
	map.cpp \
        menu01.cpp \
        menu02.cpp \
	cubemap.cpp \
	popupslider.cpp \
	keyframe.cpp \
	keyframeinformation.cpp \
	keyframeeditor.cpp \
	saveimageseqdialog.cpp \
	savemoviedialog.cpp \
        propertyeditor.cpp \
        ply.c \
        triset.cpp \
        captionwidget.cpp \
        button.cpp \
        iconlibrary.cpp
