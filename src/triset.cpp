#include "glewinitialisation.h"
#include "staticfunctions.h"
#include "triset.h"
#include "ply.h"

#include <QMessageBox>
#include <QFile>

Triset::Triset()
{
  m_vboLoaded = false;
  m_fileLoaded = false;
  
  m_vertData = 0;
  m_indexData = 0;

  m_glVertBuffer = 0;
  m_glIndexBuffer = 0;
  m_glVertArray = 0;

  QStringList ps;
  ps << "x";
  ps << "y";
  ps << "z";
  ps << "nx";
  ps << "ny";
  ps << "nz";
  ps << "red";
  ps << "green";
  ps << "blue";
  ps << "vertex_indices";
  ps << "vertex";
  ps << "face";

  for(int i=0; i<ps.count(); i++)
    {
      char *s;
      s = new char[ps[i].size()+1];
      strcpy(s, ps[i].toLatin1().data());
      plyStrings << s;
    }

  clear();

  // generate VBO structures here
  // so that they are created while in the main thread
  genVBOs();
}

Triset::~Triset()
{
  clear();
}

void
Triset::clear()
{
  m_vboLoaded = false;
  m_fileLoaded = false;
  
  m_time = -1;

  m_ntri = 0;
  m_nvert = 0;
  
  m_vertices.clear();
  m_normals.clear();
  m_vcolor.clear();
  m_triangles.clear();

  if (m_vertData) delete [] m_vertData;
  if (m_indexData) delete [] m_indexData;
  m_vertData = 0;
  m_indexData = 0;
  
  m_bmin = m_bmax = Vec(0,0,0);
  m_gmin = m_gmax = Vec(0,0,0);
  
  if(m_glVertArray)
    {
      glDeleteBuffers(1, &m_glIndexBuffer);
      glDeleteVertexArrays( 1, &m_glVertArray );
      glDeleteBuffers(1, &m_glVertBuffer);
      m_glIndexBuffer = 0;
      m_glVertArray = 0;
      m_glVertBuffer = 0;
    }
}

void
Triset::setGlobalMinMax(Vec gmin, Vec gmax)
{
  m_gmin = gmin;
  m_gmax = gmax;

  m_vboLoaded = false;
}

bool
Triset::load()
{
  if (m_fileLoaded)
    return true;

  bool loaded = false;

  if (StaticFunctions::checkExtension(m_fileName, ".ply"))
    loaded = loadPLY(m_fileName);
  else
    loaded = loadVBO(m_fileName);

  return loaded;
}

bool
Triset::loadPLY(QString flnm)
{
  if (m_fileLoaded)
    return true;

  typedef struct Vertex {
    float x,y,z;
    float r,g,b;
    float nx,ny,nz;
    void *other_props;       /* other properties */
  } Vertex;

  typedef struct Face {
    unsigned char nverts;    /* number of vertex indices in list */
    int *verts;              /* vertex index list */
    void *other_props;       /* other properties */
  } Face;

  PlyProperty vert_props[] = { /* list of property information for a vertex */
    {plyStrings[0], Float32, Float32, offsetof(Vertex,x), 0, 0, 0, 0},
    {plyStrings[1], Float32, Float32, offsetof(Vertex,y), 0, 0, 0, 0},
    {plyStrings[2], Float32, Float32, offsetof(Vertex,z), 0, 0, 0, 0},
    {plyStrings[6], Float32, Float32, offsetof(Vertex,r), 0, 0, 0, 0},
    {plyStrings[7], Float32, Float32, offsetof(Vertex,g), 0, 0, 0, 0},
    {plyStrings[8], Float32, Float32, offsetof(Vertex,b), 0, 0, 0, 0},
    {plyStrings[3], Float32, Float32, offsetof(Vertex,nx), 0, 0, 0, 0},
    {plyStrings[4], Float32, Float32, offsetof(Vertex,ny), 0, 0, 0, 0},
    {plyStrings[5], Float32, Float32, offsetof(Vertex,nz), 0, 0, 0, 0},
  };

  PlyProperty face_props[] = { /* list of property information for a face */
    {plyStrings[9], Int32, Int32, offsetof(Face,verts),
     1, Uint8, Uint8, offsetof(Face,nverts)},
  };


  /*** the PLY object ***/

  int nverts,nfaces;
  Vertex **vlist;
  Face **flist;

  PlyOtherProp *vert_other,*face_other;

  bool per_vertex_color = false;
  bool has_normals = false;

  int i,j;
  int elem_count;
  char *elem_name;
  PlyFile *in_ply;


  /*** Read in the original PLY object ***/
  FILE *fp = fopen(flnm.toLatin1().data(), "rb");

  in_ply  = read_ply (fp);

  for (i = 0; i < in_ply->num_elem_types; i++) {

    /* prepare to read the i'th list of elements */
    elem_name = setup_element_read_ply (in_ply, i, &elem_count);


    if (QString("vertex") == QString(elem_name)) {

      /* create a vertex list to hold all the vertices */
      vlist = (Vertex **) malloc (sizeof (Vertex *) * elem_count);
      nverts = elem_count;


      /* set up for getting vertex elements */

      setup_property_ply (in_ply, &vert_props[0]);
      setup_property_ply (in_ply, &vert_props[1]);
      setup_property_ply (in_ply, &vert_props[2]);
      
      for (j = 0; j < in_ply->elems[i]->nprops; j++) {
	PlyProperty *prop;
	prop = in_ply->elems[i]->props[j];
	if (QString("r") == QString(prop->name) ||
	    QString("red") == QString(prop->name)) {
	  setup_property_ply (in_ply, &vert_props[3]);
	  per_vertex_color = true;
	}
	if (QString("g") == QString(prop->name) ||
	    QString("green") == QString(prop->name)) {
	  setup_property_ply (in_ply, &vert_props[4]);
	  per_vertex_color = true;
	}
	if (QString("b") == QString(prop->name) ||
	    QString("blue") == QString(prop->name)) {
	  setup_property_ply (in_ply, &vert_props[5]);
	  per_vertex_color = true;
	}
	if (QString("nx") == QString(prop->name)) {
	  setup_property_ply (in_ply, &vert_props[6]);
	  has_normals = true;
	}
	if (QString("ny") == QString(prop->name)) {
	  setup_property_ply (in_ply, &vert_props[7]);
	  has_normals = true;
	}
	if (QString("nz") == QString(prop->name)) {
	  setup_property_ply (in_ply, &vert_props[8]);
	  has_normals = true;
	}
      }

      vert_other = get_other_properties_ply (in_ply, 
					     offsetof(Vertex,other_props));

      /* grab all the vertex elements */
      for (j = 0; j < elem_count; j++) {
        vlist[j] = (Vertex *) malloc (sizeof (Vertex));
        get_element_ply (in_ply, (void *) vlist[j]);
      }
    }
    else if (QString("face") == QString(elem_name)) {

      /* create a list to hold all the face elements */
      flist = (Face **) malloc (sizeof (Face *) * elem_count);
      nfaces = elem_count;

      /* set up for getting face elements */

      setup_property_ply (in_ply, &face_props[0]);
      face_other = get_other_properties_ply (in_ply, 
					     offsetof(Face,other_props));

      /* grab all the face elements */
      for (j = 0; j < elem_count; j++) {
        flist[j] = (Face *) malloc (sizeof (Face));
        get_element_ply (in_ply, (void *) flist[j]);
      }
    }
    else
      get_other_element_ply (in_ply);
  }

  close_ply (in_ply);
  free_ply (in_ply);

  m_vertices.resize(nverts);
  for(int i=0; i<nverts; i++)
    m_vertices[i] = Vec(vlist[i]->x,
			vlist[i]->y,
			vlist[i]->z);


  m_normals.clear();
  if (has_normals)
    {
      m_normals.resize(nverts);
      for(int i=0; i<nverts; i++)
	m_normals[i] = Vec(vlist[i]->nx,
			   vlist[i]->ny,
			   vlist[i]->nz);
    }

  m_vcolor.clear();
  if (per_vertex_color)
    {
      m_vcolor.resize(nverts);
      for(int i=0; i<nverts; i++)
	m_vcolor[i] = Vec(vlist[i]->r/255.0f,
			  vlist[i]->g/255.0f,
			  vlist[i]->b/255.0f);
    }

  // only triangles considered
  int ntri=0;
  for (int i=0; i<nfaces; i++)
    {
      if (flist[i]->nverts >= 3)
	ntri++;
    }
  m_triangles.resize(3*ntri);

  int tri=0;
  for(int i=0; i<nfaces; i++)
    {
      if (flist[i]->nverts >= 3)
	{
	  m_triangles[3*tri+0] = flist[i]->verts[0];
	  m_triangles[3*tri+1] = flist[i]->verts[1];
	  m_triangles[3*tri+2] = flist[i]->verts[2];
	  tri++;
	}
    }


  m_bmin = m_vertices[0];
  m_bmax = m_vertices[0];
  for(int i=0; i<nverts; i++)
    {
      m_bmin = StaticFunctions::minVec(m_bmin, m_vertices[i]);
      m_bmax = StaticFunctions::maxVec(m_bmax, m_vertices[i]);
    }

  m_nvert = m_vertices.count();
  m_ntri = m_triangles.count();

  QString vboflnm = m_fileName;
  vboflnm.chop(3);
  vboflnm += QString("vbo");
  saveVBO(vboflnm);
  
  m_fileLoaded = true;  

  return true;
}

void
Triset::genVBOs()
{
  glGenVertexArrays(1, &m_glVertArray);
  glGenBuffers(1, &m_glVertBuffer);
  glGenBuffers(1, &m_glIndexBuffer);      
}

QList<GLuint>
Triset::vbObject()
{
  QList<GLuint> vbo;
  vbo << m_glVertArray;
  vbo << m_glVertBuffer;
  vbo << m_glIndexBuffer;

  return vbo;
}

bool
Triset::loadVertexBufferData()
{
  if (m_vboLoaded)
    return true;
  
  return loadVertexBufferData(m_glVertArray,
			      m_glVertBuffer,
			      m_glIndexBuffer);
}

bool
Triset::loadVertexBufferData(GLuint glVA,
			     GLuint glVB,
			     GLuint glIB)
{
  if (!m_fileLoaded)
    return false;

  if (glVA == 0 ||
      glVB == 0 ||
      glIB == 0)
    return false;
  
  if (m_vboLoaded)
    return true;

  // update for m_gmin
  int nv = 9*m_nvert;
  for(int i=0; i<m_nvert; i++)
    {
      m_vertData[9*i+0] -= m_gmin.x;
      m_vertData[9*i+1] -= m_gmin.y;
      m_vertData[9*i+2] -= m_gmin.z;
    }

  glBindVertexArray(glVA);
  
  // Populate a vertex buffer
  glBindBuffer(GL_ARRAY_BUFFER, glVB);
  glBufferData(GL_ARRAY_BUFFER,
	       sizeof(float)*nv,
	       m_vertData,
	       GL_STATIC_DRAW);

  // Identify the components in the vertex buffer
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
			sizeof(float)*9, // stride
			(void *)0); // starting offset

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
			sizeof(float)*9,
			(char *)NULL + sizeof(float)*3);
  
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 
			sizeof(float)*9,
			(char *)NULL + sizeof(float)*6);

  // Create and populate the index buffer
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glIB);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	       sizeof(unsigned int)*m_ntri,
	       m_indexData,
	       GL_STATIC_DRAW);
  
  glBindVertexArray(0);

  delete [] m_vertData;
  delete [] m_indexData;
  m_vertData = 0;
  m_indexData = 0;
  
  m_vboLoaded = true;  

  return true;
}

bool
Triset::loadVBO(QString flnm)
{
  if (m_fileLoaded)
    return true;
 
  if (m_vertData) delete [] m_vertData;
  if (m_indexData) delete [] m_indexData;
  m_vertData = 0;
  m_indexData = 0;

  QFile fd(flnm);
  fd.open(QFile::ReadOnly);

  fd.read((char*)&m_nvert, sizeof(int));
  fd.read((char*)&m_ntri, sizeof(int));
  
  fd.read((char*)&m_bmin, 3*sizeof(float));
  fd.read((char*)&m_bmax, 3*sizeof(float));

  int nvnc = m_nvert*9;
  m_vertData = new float[nvnc];
  fd.read((char*)m_vertData, sizeof(float)*nvnc);

  //unsigned int *indexData;
  m_indexData = new unsigned int[m_ntri];
  fd.read((char*)m_indexData, sizeof(unsigned int)*m_ntri);

  fd.close();

  m_fileLoaded = true;  
  return true;
}

void
Triset::saveVBO(QString flnm)
{  
  int nvnc = 9*m_nvert;

  if (m_vertData) delete [] m_vertData;
  if (m_indexData) delete [] m_indexData;
  m_vertData = 0;
  m_indexData = 0;

  //---------------------
  m_vertData = new float[nvnc];
  for(int i=0; i<m_nvert; i++)
    {
      m_vertData[9*i + 0] = m_vertices[i].x;
      m_vertData[9*i + 1] = m_vertices[i].y;
      m_vertData[9*i + 2] = m_vertices[i].z;
      m_vertData[9*i + 3] = m_normals[i].x;
      m_vertData[9*i + 4] = m_normals[i].y;
      m_vertData[9*i + 5] = m_normals[i].z;
      m_vertData[9*i + 6] = m_vcolor[i].x;
      m_vertData[9*i + 7] = m_vcolor[i].y;
      m_vertData[9*i + 8] = m_vcolor[i].z;
    }

  m_indexData = new unsigned int[m_ntri];
  for(int i=0; i<m_ntri; i++)
    m_indexData[i] = m_triangles[i];
  //---------------------

  QFile fd(flnm);
  fd.open(QFile::WriteOnly);

  fd.write((char*)&m_nvert, sizeof(int));
  fd.write((char*)&m_ntri, sizeof(int));
  
  fd.write((char*)&m_bmin, 3*sizeof(float));
  fd.write((char*)&m_bmax, 3*sizeof(float));

  fd.write((char*)m_vertData, sizeof(float)*nvnc);
  fd.write((char*)m_indexData, sizeof(unsigned int)*m_ntri);

  fd.close();

  m_vertices.clear();
  m_normals.clear();
  m_vcolor.clear();
  m_triangles.clear();
}


void
Triset::draw()
{
  if (!m_vboLoaded)
    return;

  draw(m_glVertArray,
       m_glVertBuffer,
       m_glIndexBuffer);
}

void
Triset::draw(GLuint glVA,
	     GLuint glVB,
	     GLuint glIB)
{
  if (!m_vboLoaded)
    return;
  
  glBindVertexArray(glVA);

  glBindBuffer(GL_ARRAY_BUFFER, glVB);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glIB);  
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);

  glDrawElements(GL_TRIANGLES, m_ntri, GL_UNSIGNED_INT, 0);  

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);

  glBindVertexArray(0);
}
