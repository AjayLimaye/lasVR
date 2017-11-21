#include "glewinitialisation.h"
#include "staticfunctions.h"
#include "triset.h"
#include "ply.h"

#include <QMessageBox>

Triset::Triset()
{
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
}

Triset::~Triset()
{
  clear();
}

void
Triset::clear()
{
  m_time = -1;
  
  m_vertices.clear();
  m_normals.clear();
  m_vcolor.clear();
  m_triangles.clear();

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

  loadVertexBufferData();
}

bool
Triset::load()
{
  bool loaded = loadPLY(m_fileName);

  if (loaded)
    {
      loadVertexBufferData();
      return true;
    }
  return false; 
}

bool
Triset::loadPLY(QString flnm)
{
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

  return true;
}

void
Triset::loadVertexBufferData()
{
  int stride = 1;
  if (m_normals.count()) stride++; // per vertex normal
  if (m_vcolor.count()) stride++; // per vertex color
  
  int nvert = m_vertices.count();
  int nv = 3*stride*nvert;
  int ni = m_triangles.count();
  //---------------------

  //---------------------
  float *vertData;
  vertData = new float[nv];
  for(int i=0; i<nvert; i++)
    {
      vertData[9*i + 0] = m_vertices[i].x - m_gmin.x;
      vertData[9*i + 1] = m_vertices[i].y - m_gmin.y;
      vertData[9*i + 2] = m_vertices[i].z - m_gmin.z;
      vertData[9*i + 3] = m_normals[i].x;
      vertData[9*i + 4] = m_normals[i].y;
      vertData[9*i + 5] = m_normals[i].z;
      vertData[9*i + 6] = m_vcolor[i].x;
      vertData[9*i + 7] = m_vcolor[i].y;
      vertData[9*i + 8] = m_vcolor[i].z;
    }

//  for(int i=0; i<nvert; i++)
//    {
//      vertData[stride*3*i + 0] = m_vertices[i].x + m_position.x;
//      vertData[stride*3*i + 1] = m_vertices[i].y + m_position.y;
//      vertData[stride*3*i + 2] = m_vertices[i].z + m_position.z;
//
//      if (m_normals.count() > 0)
//	{
//	  vertData[stride*3*i + 3] = m_normals[i].x;
//	  vertData[stride*3*i + 4] = m_normals[i].y;
//	  vertData[stride*3*i + 5] = m_normals[i].z;
//	  if (m_vcolor.count() > 0)
//	    {
//	      vertData[stride*3*i + 6] = m_vcolor[i].x;
//	      vertData[stride*3*i + 7] = m_vcolor[i].y;
//	      vertData[stride*3*i + 8] = m_vcolor[i].z;
//	    }
//	}
//      else if (m_vcolor.count() > 0)
//	{
//	  vertData[stride*3*i + 3] = m_vcolor[i].x;
//	  vertData[stride*3*i + 4] = m_vcolor[i].y;
//	  vertData[stride*3*i + 5] = m_vcolor[i].z;
//	}
//    }

  unsigned int *indexData;
  indexData = new unsigned int[ni];
  for(int i=0; i<m_triangles.count(); i++)
    indexData[i] = m_triangles[i];
  //---------------------

  if(m_glVertArray)
    {
      glDeleteBuffers(1, &m_glIndexBuffer);
      glDeleteVertexArrays( 1, &m_glVertArray );
      glDeleteBuffers(1, &m_glVertBuffer);
      m_glIndexBuffer = 0;
      m_glVertArray = 0;
      m_glVertBuffer = 0;
    }

  
  glGenVertexArrays(1, &m_glVertArray);
  glBindVertexArray(m_glVertArray);
      
  // Populate a vertex buffer
  glGenBuffers(1, &m_glVertBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_glVertBuffer);
  glBufferData(GL_ARRAY_BUFFER,
	       sizeof(float)*nv,
	       vertData,
	       GL_STATIC_DRAW);

  // Identify the components in the vertex buffer
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
			sizeof(float)*3*stride, // stride
			(void *)0); // starting offset

  if (stride > 1)
    {
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
			    sizeof(float)*3*stride,
			    (char *)NULL + sizeof(float)*3);
    }
  
  if (stride > 2)
    {
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 
			    sizeof(float)*3*stride,
			    (char *)NULL + sizeof(float)*6);
    }

  // Create and populate the index buffer
  glGenBuffers(1, &m_glIndexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	       sizeof(unsigned int)*ni,
	       indexData,
	       GL_STATIC_DRAW);
  
  glBindVertexArray(0);

  delete [] vertData;
  delete [] indexData;
}

void
Triset::draw()
{
  int ni = m_triangles.count();

  glBindVertexArray(m_glVertArray);
  glBindBuffer(GL_ARRAY_BUFFER, m_glVertBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);  
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);

  glDrawElements(GL_TRIANGLES, ni, GL_UNSIGNED_INT, 0);  

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
  glBindVertexArray(0);
}
