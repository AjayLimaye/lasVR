#ifndef SHADERFACTORY_H
#define SHADERFACTORY_H

#include <GL/glew.h>

#include <QString>

class ShaderFactory
{
 public :

  static bool loadShadersFromFile(GLhandleARB&, QString, QString);

  static bool loadPointShader(GLhandleARB&, QString, int);

  static QString genPointShaderString();
  static QString genPointDepth(bool);

  static bool loadShader(GLhandleARB&, QString, QString);
  static QString drawShadowsVertexShader();
  static QString drawShadowsFragmentShader();

  static QString genBilateralFilterShaderString();

  static int loadShaderFromFile(GLhandleARB, QString);

  static void createTextureShader();
  static GLuint rcShader();
  static GLint* rcShaderParm();

  static void createCubeMapShader();
  static GLuint cubemapShader();
  static GLint* cubemapShaderParm();

 private :
  static GLuint m_rcShader;
  static GLint m_rcShaderParm[10];

  static GLuint m_cubemapShader;
  static GLint m_cubemapShaderParm[10];
};

#endif
