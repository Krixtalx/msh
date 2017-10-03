/*
  ==============================================================================
  
  MSH_GFX.H - WIP!
  
  A single header library that wraps OpenGL. This library should be providing two
  APISs, with two purposes:
    - High Level API - Streamline common OpenGL operation
    - Low Level API - Layer of abstraction for different OpenGL context 
                      (3.3+, 2.1, ES), or even different rendering APIs

  What the low level api aims to be is close to Branimir Karadzic bgfx.
  What the high level api aims to be is close to Ricardo Cabello(mrdoob) three.js
  But in C11... ¯\_(ツ)_/¯ We'll see how it goes. Currently it is a bit of a
  hodgepodge and I would not recommend using it.

  To use the library you simply add:
  
  #define MSH_GFX_IMPLEMENTATION
  #include "msh_gfx.h"

  The define should only include once in your source. If you need to include 
  library in multiple places, simply use the include:

  #include "msh_gfx.h"

  ==============================================================================
  DEPENDENCIES

  This library depends on following standard headers:
    <stdlib.h>
    <assert.h>

  By default this library does not import these headers. Please see 
  docs/no_headers.md for explanation. Importing heades is enabled by:

  #define MSH_GFX_INCLUDE_HEADES

  ==============================================================================
  AUTHORS

    Maciej Halber (macikuh@gmail.com)

  ==============================================================================
  LICENSE

  This software is in the public domain. Where that dedication is not
  recognized, you are granted a perpetual, irrevocable license to copy,
  distribute, and modify this file as you see fit.

  ==============================================================================
  NOTES 

  1. This currently is a wrapper around the opengl. What it should be
     is a API agnostic renderer -> Similar to bgfx / nanovg.
  2. Currently windowing is a thin wrapper over GLFW. There is some 
     functionality added for handling multiple windows. This will be removed. 

  ==============================================================================
  TODO
  [ ] Texture and framebuffers resize
  [ ] Modify shader code to enable use of glProgramPipeline
  [ ] For geometry, need to experiment with glMapBufferRange
  [ ] Add informative error messages. Make them optional
  [ ] Add lower-level wrapper for geometry handling
  [ ] Add higher level stuff for wrapping materials
  [ ] Remove the mshgfx_window stuff and replace it with glfw3
  [ ] Think more about the viewport design
  [ ] Decide msh, mshgfx mgfx
  [ ] Cube maps
  [ ] Regular vs read framebuffers.

  ==============================================================================
  REFERENCES:

 */


/*
 * =============================================================================
 *       INCLUDES, TYPES AND DEFINES
 * =============================================================================
 */

#ifndef MSH_GFX_H
#define MSH_GFX_H

#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#ifdef __APPLE__
  #include <OpenGL/gl3.h>
#else if
  #include "glad/glad.h"
#endif

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#ifndef MSH_VEC_MATH

typedef union vec2
{
  struct { float x; float y; };
  float data[2];
} msh_vec2_t;

#endif


enum mshgfx_shader_type
{
  VERTEX_SHADER,
  FRAGMENT_SHADER,
  GEOMETRY_SHADER,
  TESS_CONTROL_SHADER,
  TESS_EVALUATION_SHADER
};

enum msh_geometry_properties_flags_
{
  POSITION  = 1 << 0, /* layout( location = 0 ) */
  NORMAL    = 1 << 1, /* layout( location = 1 ) */
  TANGENT   = 1 << 2, /* layout( location = 2 ) */
  TEX_COORD = 1 << 3, /* layout( location = 3 ) */
  COLOR_A   = 1 << 4, /* layout( location = 4 ) */
  COLOR_B   = 1 << 5, /* layout( location = 5 ) */
  COLOR_C   = 1 << 6, /* layout( location = 6 ) */
  COLOR_D   = 1 << 7, /* layout( location = 7 ) */
  STRUCTURED = 1 << 8,

  SIMPLE_MESH   = POSITION | NORMAL | STRUCTURED,
  POINTCLOUD    = POSITION
};

enum msh_texture_options_flags_
{
  MSH_NEAREST               = 1 << 0, 
  MSH_LINEAR                = 1 << 1,
  MSH_CLAMP_TO_EDGE         = 1 << 2,
  MSH_CLAMP_TO_BORDER       = 1 << 3,
  MSH_REPEAT                = 1 << 4,
  MSH_MIRRORED_REPEAT       = 1 << 5
};

enum msh_framebuffer_attachments_
{
  MSH_COLOR_ATTACHMENT0        = GL_COLOR_ATTACHMENT0,
  MSH_COLOR_ATTACHMENT1        = GL_COLOR_ATTACHMENT1,
  MSH_COLOR_ATTACHMENT2        = GL_COLOR_ATTACHMENT2,
  MSH_COLOR_ATTACHMENT3        = GL_COLOR_ATTACHMENT3,
  MSH_COLOR_ATTACHMENT4        = GL_COLOR_ATTACHMENT4,
  MSH_COLOR_ATTACHMENT5        = GL_COLOR_ATTACHMENT5,
  MSH_COLOR_ATTACHMENT6        = GL_COLOR_ATTACHMENT6,
  MSH_COLOR_ATTACHMENT7        = GL_COLOR_ATTACHMENT7,
  MSH_DEPTH_ATTACHMENT         = GL_DEPTH_ATTACHMENT,
  MSH_STENCIL_ATTACHMENT       = GL_STENCIL_ATTACHMENT,
  MSH_DEPTH_STENCIL_ATTACHMENT = GL_COLOR_ATTACHMENT7,
};


typedef struct msh_viewport
{
  msh_vec2_t p1;
  msh_vec2_t p2;
} msh_viewport_t;

typedef struct mshgfx_shader
{
  GLuint id;
  enum mshgfx_shader_type type;
  GLint compiled;
} mshgfx_shader_t;


typedef struct mshgfx_shader_prog
{
  GLuint id;
  GLint linked;
} mshgfx_shader_prog_t;

// NOTE(maciej): Should textures know that they are attached and fail?
typedef struct mshgfx_texture1d
{
  uint32_t id;
  int32_t width;
  int32_t n_comp;
  uint32_t type;
  uint32_t unit;
} mshgfx_texture1d_t;


typedef struct mshgfx_texture2d
{
  uint32_t id;
  int32_t width;
  int32_t height;
  int32_t n_comp;
  uint32_t type;
  uint32_t unit;
} mshgfx_texture2d_t;

typedef struct mshgfx_texture3d
{
  uint32_t id;
  int32_t width;
  int32_t height;
  int32_t depth;
  int32_t n_comp;
  uint32_t type;
  uint32_t unit;
} mshgfx_texture3d_t;

typedef struct msh_renderbuffer
{
  GLuint id;
  int32_t width;
  int32_t height;
  uint32_t storage_type;
} mshgfx_renderbuffer_t;

typedef struct msh_framebuffer
{
  GLuint id;
  int32_t width;
  int32_t height;
  mshgfx_texture2d_t * tex_attached;

  int32_t n_tex;
  mshgfx_renderbuffer_t * rb_attached;
  int32_t n_rb;
} mshgfx_framebuffer_t;

// TODO(maciej): Add better support for custom user data.
typedef struct mshgfx_geometry_data
{
  float * positions;
  float * normals;
  float * tangents;
  float *texcoords;
  uint8_t  * colors_a;
  uint8_t  * colors_b;
  uint8_t  * colors_c;
  uint8_t  * colors_d;
  uint32_t * indices;
  int32_t n_vertices;
  int32_t n_elements;
} mshgfx_geometry_data_t;

typedef int32_t msh_geometry_properties_flags;
typedef struct mshgfx_geometry
{
  uint32_t vao;
  uint32_t vbo;
  uint32_t ebo;
  int32_t n_indices;
  int32_t n_elements;
  int32_t buffer_size;
  msh_geometry_properties_flags flags;
} mshgfx_geometry_t;


/*
 * =============================================================================
 *       RENDERBUFFER 
 * =============================================================================
 */

int32_t mshgfx_renderbuffer_init( mshgfx_renderbuffer_t * rb, 
                                  int32_t width, int32_t height, 
                                  uint32_t storage_type );
int32_t mshgfx_renderbuffer_free( mshgfx_renderbuffer_t * rb );
/*
 * =============================================================================
 *       FRAMEBUFFER 
 * =============================================================================
 */

int32_t mshgfx_framebuffer_init( mshgfx_framebuffer_t * fb, 
                                 int32_t width, int32_t height );
int32_t mshgfx_framebuffer_bind( mshgfx_framebuffer_t * fb );
int32_t mshgfx_framebuffer_attach_textures( mshgfx_framebuffer_t *fb, 
                                        mshgfx_texture2d_t *tex,
                                        GLuint *attachment, 
                                        int32_t n_textures );
int32_t mshgfx_framebuffer_attach_renderbuffers( mshgfx_framebuffer_t *fb,
                                                 mshgfx_renderbuffer_t *rb,
                                                 GLuint *attachment,
                                                 int32_t n_renderbuffers );
int32_t mshgfx_framebuffer_resize( mshgfx_framebuffer_t *fb,
                                   int32_t width, int32_t height );
int32_t mshgfx_framebuffer_check_status( mshgfx_framebuffer_t * fb );
int32_t mshgfx_framebuffer_free( mshgfx_framebuffer_t *fb );



/*
 * =============================================================================
 *       TEXTURES
 * =============================================================================
 */
void mshgfx_texture1d_init( mshgfx_texture1d_t *tex,
                            const void *data, 
                            const int32_t type,
                            const int32_t w, 
                            const int32_t n_comp, 
                            const uint32_t unit,
                            const int32_t user_flags );
 

void mshgfx_texture1d_update( mshgfx_texture1d_t *tex, 
                              const void *data, const uint32_t type );

void mshgfx_texture1d_use( const mshgfx_texture1d_t *tex );
void mshgfx_texture1d_free( mshgfx_texture1d_t *tex );

void mshgfx_texture2d_init( mshgfx_texture2d_t *tex,
                            const void *data, 
                            const int32_t type,
                            const int32_t w, 
                            const int32_t h, 
                            const int32_t n_comp, 
                            const uint32_t unit,
                            const int32_t user_flags );
 

void mshgfx_texture2d_update( mshgfx_texture2d_t *tex, 
                              const void *data, const uint32_t type );

void mshgfx_texture2d_use( const mshgfx_texture2d_t *tex );
void mshgfx_texture2d_free( mshgfx_texture2d_t *tex );


void mshgfx_texture3d_init( mshgfx_texture3d_t *tex,
                            const void *data, 
                            const int32_t type,
                            const int32_t w, 
                            const int32_t h,
                            const int32_t d, 
                            const int32_t n_comp,
                            const uint32_t unit,
                            const int32_t user_flags );

void mshgfx_texture3d_update(  mshgfx_texture3d_t *tex, 
                               const void *data, const uint32_t type  );

void mshgfx_texture3d_use( const mshgfx_texture3d_t *tex );
void mshgfx_texture3d_free( mshgfx_texture3d_t *tex );

/*
 * =============================================================================
 *       VIEWPORTS 
 * =============================================================================
 */

int32_t  msh_viewport_init( msh_viewport_t *v, msh_vec2_t p1, msh_vec2_t p2 );
void msh_viewport_begin( const msh_viewport_t *v);
void msh_viewport_end();


void mshgfx_background_flat4f( float r, float g, float b, float a);
void mshgfx_background_gradient4f( float r1, float g1, float b1, float a1,
                                   float r2, float g2, float b2, float a2 );
void mshgfx_background_tex( mshgfx_texture2d_t *tex );

#ifdef MSH_VEC_MATH
void mshgfx_background_flat4v( msh_vec4_t col );
void mshgfx_background_gradient4fv( msh_vec4_t col1, msh_vec4_t col2 );
#endif /*MSH_VEC_MATH*/


/*
 * =============================================================================
 *       SHADERS
 * =============================================================================
 */

#define MSHGFX_SHADER_HEAD "#version 330 core\n"
#define MSHGFX_SHADER_STRINGIFY(x) #x

int32_t mshgfx_shader_prog_create_from_source_vf( mshgfx_shader_prog_t *p,
                                           const char *vs_source,
                                           const char *fs_source );

int32_t mshgfx_shader_prog_create_from_source_vgf( mshgfx_shader_prog_t *p,
                                            const char *vs_source,
                                            const char *gs_source,
                                            const char *fs_source );

int32_t mshgfx_shader_prog_create_from_files_vf( mshgfx_shader_prog_t *p,
                                          const char * vs_filename,
                                          const char * fs_filename );

int32_t mshgfx_shader_prog_create_from_files_vgf( mshgfx_shader_prog_t *p,
                                           const char * vs_filename,
                                           const char * gs_filename,
                                           const char * fs_filename );

int32_t mshgfx_shader_compile( mshgfx_shader_t *s, const char * source );

int32_t mshgfx_shader_prog_link_vf( mshgfx_shader_prog_t *p,
                             const mshgfx_shader_t *vs, 
                             const mshgfx_shader_t *fs );

int32_t mshgfx_shader_prog_link_vgf( mshgfx_shader_prog_t *p,
                              const mshgfx_shader_t *vs, 
                              const mshgfx_shader_t *gs, 
                              const mshgfx_shader_t *fs );

int32_t mshgfx_shader_prog_use( const mshgfx_shader_prog_t *p );


void mshgfx_shader_prog_set_uniform_1f( const mshgfx_shader_prog_t *p, 
                                     const char *attrib_name, 
                                     float x );
void mshgfx_shader_prog_set_uniform_1i( const mshgfx_shader_prog_t *p, 
                                     const char *attrib_name, 
                                     int32_t x );
void mshgfx_shader_prog_set_uniform_1u( const mshgfx_shader_prog_t *p, 
                                     const char *attrib_name, 
                                     uint32_t x );

void mshgfx_shader_prog_set_uniform_2f( const mshgfx_shader_prog_t *p, 
                                     const char *attrib_name, 
                                     float x, float y );
void mshgfx_shader_prog_set_uniform_2i( const mshgfx_shader_prog_t *p, 
                                     const char *attrib_name, 
                                     int32_t x, int32_t y );
void mshgfx_shader_prog_set_uniform_2u( const mshgfx_shader_prog_t *p, 
                                     const char *attrib_name, 
                                     uint32_t x, uint32_t y );

void mshgfx_shader_prog_set_uniform_3f( const mshgfx_shader_prog_t *p, 
                                     const char *attrib_name, 
                                     float x, float y, float z );
void mshgfx_shader_prog_set_uniform_3i( const mshgfx_shader_prog_t *p, 
                                     const char *attrib_name, 
                                     int32_t x, int32_t y, int32_t z );
void mshgfx_shader_prog_set_uniform_3u( const mshgfx_shader_prog_t *p, 
                                     const char *attrib_name, 
                                     uint32_t x, uint32_t y, 
                                     uint32_t z );

void mshgfx_shader_prog_set_uniform_4f( const mshgfx_shader_prog_t *p, 
                                     const char *attrib_name, 
                                     float x, float y, float z, float w );
void mshgfx_shader_prog_set_uniform_4i( const mshgfx_shader_prog_t *p, 
                                     const char *attrib_name, 
                                     int32_t x, int32_t y, int32_t z, int32_t w );
void mshgfx_shader_prog_set_uniform_4u( const mshgfx_shader_prog_t *p, 
                                     const char *attrib_name, 
                                     uint32_t x, uint32_t y, 
                                     uint32_t z, uint32_t w );

void mshgfx_shader_prog_set_uniform_fv( const mshgfx_shader_prog_t *p, 
                                     const char *attrib_name, 
                                     const float *val, 
                                     const uint32_t count );
void mshgfx_shader_prog_set_uniform_iv( const mshgfx_shader_prog_t *p, 
                                     const char *attrib_name, 
                                     const int32_t *val, 
                                     const uint32_t count );
void mshgfx_shader_prog_set_uniform_uv( const mshgfx_shader_prog_t *p, 
                                     const char *attrib_name, 
                                     const uint32_t *val, 
                                     const uint32_t count );

#ifdef MSH_VEC_MATH
void mshgfx_shader_prog_set_uniform_2fv( const mshgfx_shader_prog_t *p, 
                                 const char *attrib_name, const msh_vec2_t *v );
void mshgfx_shader_prog_set_uniform_2fvc( const mshgfx_shader_prog_t *p, 
                                 const char *attrib_name, const msh_vec2_t *v, 
                                 const uint32_t count );
void mshgfx_shader_prog_set_uniform_3fv( const mshgfx_shader_prog_t *p, 
                                 const char *attrib_name, const msh_vec3_t *v );
void mshgfx_shader_prog_set_uniform_3fvc( const mshgfx_shader_prog_t *p, 
                                 const char *attrib_name, const msh_vec3_t *v, 
                                 const uint32_t count );
void mshgfx_shader_prog_set_uniform_4fv( const mshgfx_shader_prog_t *p, 
                                  const char *attrib_name, const msh_vec4_t *v);
void mshgfx_shader_prog_set_uniform_4fvc( const mshgfx_shader_prog_t *p, 
                                  const char *attrib_name, const msh_vec4_t *v, 
                                  const uint32_t count );

void mshgfx_shader_prog_set_uniform_3fm( const mshgfx_shader_prog_t *p, 
                                 const char *attrib_name, const msh_mat3_t *m );
void mshgfx_shader_prog_set_uniform_3fmc( const mshgfx_shader_prog_t *p, 
                                 const char *attrib_name, const msh_mat3_t *m,
                                 const uint32_t count );
void mshgfx_shader_prog_set_uniform_4fm( const mshgfx_shader_prog_t *p, 
                                 const char *attrib_name, const msh_mat4_t *m );
void mshgfx_shader_prog_set_uniform_4fmc( const mshgfx_shader_prog_t *p, 
                                 const char *attrib_name, const msh_mat4_t *m,
                                 const uint32_t count );
#endif


/*
 * =============================================================================
 *       GPU GEOMETRY
 * =============================================================================
 */

int32_t mshgfx_geometry_update( const mshgfx_geometry_t * geo, 
                            const mshgfx_geometry_data_t * host_data, 
                            const int32_t flags );

int32_t mshgfx_geometry_init( mshgfx_geometry_t * geo, 
                          const mshgfx_geometry_data_t * host_data,   
                          const int32_t flags );

int32_t mshgfx_geometry_free( mshgfx_geometry_t * geo );

void mshgfx_geometry_draw( mshgfx_geometry_t * geo, uint32_t draw_mode );


#endif /* MSH_GFX_H */




#ifdef MSH_GFX_IMPLEMENTATION

/*
 * =============================================================================
 *       HELPERS
 * =============================================================================
 */

static void 
msh__check_gl_error(const char* str)
{
	uint32_t err;
	err = glGetError();
	if (err != GL_NO_ERROR) {
		fprintf(stderr, "Error %08x after %s\n", err, str);
		return;
	}
}

/*
 * =============================================================================
 *       RENDERBUFFER IMPLEMENTATION
 * =============================================================================
 */

int32_t 
mshgfx_renderbuffer_init( mshgfx_renderbuffer_t * rb, 
                          int32_t width, int32_t height, uint32_t storage_type )
{
  rb->width = width;
  rb->height = height;
  rb->storage_type = storage_type;
  glGenRenderbuffers(1, &rb->id);
  glBindRenderbuffer(GL_RENDERBUFFER, rb->id);
  glRenderbufferStorage(GL_RENDERBUFFER, storage_type, width, height );
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  return 1;
}

int32_t 
mshgfx_renderbuffer_free( mshgfx_renderbuffer_t * rb )
{
  glDeleteRenderbuffers(1, &rb->id);
  return 1;
}

/*
 * =============================================================================
 *       FRAMEBUFFERS IMPLEMENTATION
 * =============================================================================
 */


int32_t 
mshgfx_framebuffer_init( mshgfx_framebuffer_t *fb, int32_t width, int32_t height )
{
  fb->width = width;
  fb->height = height;
  fb->tex_attached = NULL;
  fb->n_tex = 0;
  fb->rb_attached = NULL;
  fb->n_rb = 0;

  glGenFramebuffers(1, &fb->id);
  glBindFramebuffer(GL_FRAMEBUFFER, fb->id);
  return 1;
}

int32_t 
mshgfx_framebuffer_bind( mshgfx_framebuffer_t *fb )
{
  if (fb) glBindFramebuffer(GL_FRAMEBUFFER, fb->id);
  else    glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return 1;
}

int32_t
mshgfx_framebuffer_attach_textures( mshgfx_framebuffer_t *fb, 
                                    mshgfx_texture2d_t *tex,
                                    uint32_t *attachment, 
                                    int32_t n_textures )
{
  if( !fb || !tex || !attachment ) return 0;
  for( int32_t i = 0 ; i < n_textures ; ++i )
  {
    // NOTE: We should test if texture sizes are the same.
    glFramebufferTexture( GL_FRAMEBUFFER, attachment[i], tex[i].id, 0 );
  }
  glDrawBuffers( n_textures, attachment );
  fb->tex_attached = tex;
  fb->n_tex = n_textures;
  return 1;
}

int32_t
mshgfx_framebuffer_attach_renderbuffers( mshgfx_framebuffer_t *fb,
                                         mshgfx_renderbuffer_t *rb,
                                         uint32_t *attachment, 
                                         int32_t n_renderbuffers )
{
  if( !fb || !rb || !attachment ) return 0;
  for( int32_t i = 0 ; i < n_renderbuffers ; ++i )
  {
    // NOTE: We should test if renderbuffer sizes are the same.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment[i], 
                                                     GL_RENDERBUFFER, rb[i].id);
  }
  fb->rb_attached = rb;
  fb->n_rb = n_renderbuffers;
  return 1;
}

int32_t 
mshgfx_framebuffer_check_status( mshgfx_framebuffer_t * fb )
{
  if( !fb ) return 0;
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    printf( "%s\n", "Framebuffer not complete!" );
  }
  return 0;
}

int32_t 
mshgfx_framebuffer_resize( mshgfx_framebuffer_t *fb, 
                           int32_t width, int32_t height )
{
  if(!fb) return 0;
  
  /* 
  // delete old textures
  if( fb->tex_attached ) 
  {
    for( int32_t i = 0 ; i < fb->n_tex ; ++i )
    {
      mshgfx_texture2d_t * tex = &(fb->tex_attached[i]);
      // NOTE(maciej): Might not work!
      tex->width = width;
      tex->height = height;
      mshgfx_texture2d_update( tex, NULL, tex->type );
    }
  }
  if( fb->rb_attached ) 
  {
    for( int32_t i = 0 ; i < fb->n_rb; ++i )
    {
      mshgfx_renderbuffer_t * rb = &(fb->rb_attached[i]);
      mshgfx_renderbuffer_free( rb );
      mshgfx_renderbuffer_init( &(fb->rb_attached[i]), width, height );
    }
    // NOTE: We do not know attachment type...
  }
  */

  // delete framebuffer
  glDeleteFramebuffers( 1, &fb->id );
  msh__check_gl_error("framebuffer resize"); 

  // reinitialize
  mshgfx_framebuffer_init( fb, width, height );
  return 1;
}

int32_t
mshgfx_framebuffer_terminate(mshgfx_framebuffer_t *fb)
{
  if(!fb) return 0;
  glDeleteFramebuffers( 1, &fb->id );
  msh__check_gl_error("framebuffer termination");
  return 1;
}


/*
 * =============================================================================
 *       VIEWPORTS IMPLEMENTATION
 * =============================================================================
 */

int32_t msh_viewport_init(msh_viewport_t *v, msh_vec2_t p1, msh_vec2_t p2)
{
  glEnable(GL_SCISSOR_TEST);
  v->p1 = p1;
  v->p2 = p2;
  return 1;
}

void msh_viewport_begin( const msh_viewport_t *v)
{
  glScissor((GLint)v->p1.x, (GLint)v->p1.y, (GLsizei)v->p2.x, (GLsizei)v->p2.y);
  glViewport((GLint)v->p1.x, (GLint)v->p1.y, (GLsizei)v->p2.x, (GLsizei)v->p2.y );
}

void msh_viewport_end()
{
  glScissor( 0, 0, 0, 0 );
  glViewport( 0, 0, 0, 0 );
}

void 
mshgfx_background_flat4f( float r, float g, float b, float a )
{
  glClearColor( r, g, b, a );
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void 
mshgfx_background_gradient4f( float r1, float g1, float b1, float a1,
                              float r2, float g2, float b2, float a2 )
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  
  static GLuint background_vao = 0;
  static mshgfx_shader_prog_t background_shader;
  if (background_vao == 0)
  {
    glGenVertexArrays(1, &background_vao);
  
    // vertex shader ( full screen quad by Morgan McGuire)
    char* vs_src = (char*) MSHGFX_SHADER_HEAD MSHGFX_SHADER_STRINGIFY
    (
      out vec2 v_texcoords;
      void main()
      {
        uint idx = uint( gl_VertexID % 3 );
        gl_Position = vec4(
            (float(   idx        & 1U ) ) * 4.0 - 1.0,
            (float( ( idx >> 1U )& 1U ) ) * 4.0 - 1.0,
            0.0, 1.0);
        v_texcoords = gl_Position.xy * 0.5 + 0.5;
      }
    );

    char* fs_src = (char*) MSHGFX_SHADER_HEAD MSHGFX_SHADER_STRINGIFY
    (
      in vec2 v_texcoords;
      uniform vec4 col_top;
      uniform vec4 col_bot;
      out vec4 frag_color;
      void main()
      {
        frag_color = v_texcoords.y * col_top + (1.0 - v_texcoords.y) * col_bot;
      }
    );
    mshgfx_shader_prog_create_from_source_vf( &background_shader, vs_src, fs_src );
  }

  mshgfx_shader_prog_use( &background_shader );
  mshgfx_shader_prog_set_uniform_4f( &background_shader, "col_top", r1, g1, b1, a1 );
  mshgfx_shader_prog_set_uniform_4f( &background_shader, "col_bot", r2, g2, b2, a2 );
  glBindVertexArray( background_vao );
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindVertexArray(0);

  glEnable(GL_DEPTH_TEST);
}

void 
mshgfx_background_tex( mshgfx_texture2d_t *tex )
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  static GLuint background_vao = 0;
  static mshgfx_shader_prog_t background_shader;

  if (background_vao == 0)
  {
    glGenVertexArrays(1, &background_vao);
  
    // vertex shader (full screen quad by Morgan McGuire)
    char* vs_src = (char*) MSHGFX_SHADER_HEAD MSHGFX_SHADER_STRINGIFY
    (
      out vec2 v_texcoords;
      void main()
      {
        uint idx = uint( gl_VertexID % 3 );
        gl_Position = vec4(
            (float(   idx        & 1U ) ) * 4.0 - 1.0,
            (float( ( idx >> 1U )& 1U ) ) * 4.0 - 1.0,
            0.0, 1.0);
        v_texcoords = gl_Position.xy * 0.5 + 0.5;
      }
    );

    char* fs_src = (char*) MSHGFX_SHADER_HEAD MSHGFX_SHADER_STRINGIFY
    (
      in vec2 v_texcoords;
      uniform sampler2D fb_tex;
      out vec4 frag_color;
      void main()
      {
        vec3 tex_color = texture( fb_tex, v_texcoords ).rgb;
        frag_color = vec4( tex_color.r, tex_color.g, tex_color.b, 1.0f );
      }
    );
    mshgfx_shader_prog_create_from_source_vf( &background_shader, vs_src, fs_src );
  }
  
  mshgfx_shader_prog_use( &background_shader );
  mshgfx_texture2d_use( tex );
  mshgfx_shader_prog_set_uniform_1i( &background_shader, "fb_tex", tex->unit );

  glBindVertexArray( background_vao );
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindVertexArray(0);
  glEnable(GL_DEPTH_TEST);
}

#ifdef MSH_VEC_MATH
void 
mshgfx_background_flat4fv( msh_vec4_t col )
{
  glClearColor( col.x, col.y, col.z, col.w );
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void 
mshgfx_background_gradient4fv( msh_vec4_t col_top, msh_vec4_t col_bot )
{
  mshgfx_background_gradient4f( col_top.x, col_top.y, col_top.z, col_top.w,
                             col_bot.x, col_bot.y, col_bot.z, col_bot.w );
}
#endif

/*
 * =============================================================================
 *       SHADERS IMPLEMENTATION
 * =============================================================================
 */


int32_t
msh__shader_text_file_read( const char * filename, char ** file_contents )
{
  FILE *fp;
  long l_size;

  fp = fopen ( filename , "r" );
  if( !fp ) perror(filename),exit(1);

  fseek( fp , 0L , SEEK_END);
  l_size = ftell( fp );
  rewind( fp );

  /* allocate memory for entire content */ 
  (*file_contents) = (char*)calloc( 1, l_size + 1 );
  if ( !(*file_contents) )
  { 
    fclose( fp ); 
    fputs( "memory alloc fails", stderr ); 
    exit( 1 );
  }

  /* copy the file into the buffer */
  if ( fread( (*file_contents), l_size, 1, fp) != 1 )
  {
    fclose(fp); 
    free( (*file_contents) ); 
    fputs( "entire read fails",stderr );
    exit( 1 );
  }

  fclose(fp);
  return 1;
}


int32_t
mshgfx_shader_compile( mshgfx_shader_t *s, const char * source )
{
  switch ( s->type ) {
      case VERTEX_SHADER:
          s->id = glCreateShader( GL_VERTEX_SHADER );
          break;
      case FRAGMENT_SHADER:
          s->id = glCreateShader( GL_FRAGMENT_SHADER );
          break;

    #ifndef __EMSCRIPTEN__
      case GEOMETRY_SHADER:
          s->id = glCreateShader( GL_GEOMETRY_SHADER );
          break;
      case TESS_CONTROL_SHADER:
          s->id = glCreateShader( GL_TESS_CONTROL_SHADER );
          break;
      case TESS_EVALUATION_SHADER:
          s->id = glCreateShader( GL_TESS_EVALUATION_SHADER );
          break;
    #endif

      default:
          return 0;
  }

  glShaderSource(s->id, 1, &source, NULL);

  glCompileShader(s->id);
  glGetShaderiv(s->id, GL_COMPILE_STATUS, &s->compiled);

  if(!s->compiled)
  {
    GLint log_len;
    glGetShaderiv(s->id, GL_INFO_LOG_LENGTH, &log_len);
    if ( log_len > 0 )
    {
      char* log_str = (char*)malloc(log_len);
      GLsizei written;
      glGetShaderInfoLog(s->id, log_len, &written, log_str);
      char * mshgfx_shader_type_str = NULL;
      switch(s->type)
      {
        case VERTEX_SHADER:
          mshgfx_shader_type_str = (char*)"Vertex Shader";
          break;
        case FRAGMENT_SHADER:
          mshgfx_shader_type_str = (char*)"Fragment Shader";
          break;
        case GEOMETRY_SHADER:
          mshgfx_shader_type_str = (char*)"Geometry Shader";
          break;
        case TESS_CONTROL_SHADER:
          mshgfx_shader_type_str = (char*)"Tess Control Shader";
          break;
        case TESS_EVALUATION_SHADER:
          mshgfx_shader_type_str = (char*)"Tess Evaluation Shader";
          break;
      }
      printf( "%s Compilation Failure :\n%s\n", mshgfx_shader_type_str, log_str );
      free( log_str );
    }
    return 0;
  }

  return s->compiled;
}

int32_t 
mshgfx_shader_prog_link_vf( mshgfx_shader_prog_t *p,
                         const mshgfx_shader_t *vs, 
                         const mshgfx_shader_t *fs )
{
  /* NOTE: change to asserts? */
  if ( !vs->compiled || !fs->compiled )
  {
    printf( "Compile shaders before linking program!\n" );
    return 1;
  }

  p->id = glCreateProgram();
  if ( p->id == 0 )
  {
    printf ( " Failed to create program (%d)!\n", p->id );
    return 0;
  }

  glAttachShader( p->id, vs->id );
  glAttachShader( p->id, fs->id );

  glLinkProgram( p->id );
  glGetProgramiv( p->id, GL_LINK_STATUS, &p->linked );

  if ( !p->linked )
  {
    GLint log_len;
    glGetProgramiv( p->id, GL_INFO_LOG_LENGTH, &log_len );
    if( log_len > 0 )
    {
      char* log_str = (char*) malloc( log_len );
      GLsizei written;
      glGetProgramInfoLog( p->id, log_len, &written, log_str );
      printf( "Program Linking Failure : %s\n", log_str );
      free( log_str );
    }
  }

  glDetachShader( p->id, fs->id );
  glDetachShader( p->id, vs->id );

  glDeleteShader( fs->id );
  glDeleteShader( vs->id );

  return p->linked;
}


int32_t
mshgfx_shader_prog_link_vgf( mshgfx_shader_prog_t *p,
                          const mshgfx_shader_t *vs, 
                          const mshgfx_shader_t *gs, 
                          const mshgfx_shader_t *fs )
{
  /* NOTE: change to asserts? */
  if ( !vs->compiled || !fs->compiled )
  {
    printf( "Compile shaders before linking program!\n" );
    return 1;
  }

  p->id = glCreateProgram();
  if ( p->id == 0 )
  {
    printf ( " Failed to create program!\n" );
    return 0;
  }

  glAttachShader( p->id, vs->id );
  glAttachShader( p->id, gs->id );
  glAttachShader( p->id, fs->id );

  glLinkProgram( p->id );
  glGetProgramiv( p->id, GL_LINK_STATUS, &p->linked );

  if ( !p->linked )
  {
    GLint log_len;
    glGetProgramiv( p->id, GL_INFO_LOG_LENGTH, &log_len );
    if( log_len > 0 )
    {
      char* log_str = (char*) malloc( log_len );
      GLsizei written;
      glGetProgramInfoLog( p->id, log_len, &written, log_str );
      printf( "Program Linking Failure : %s\n", log_str );
      free( log_str );
    }
  }

  glDetachShader( p->id, fs->id );
  glDetachShader( p->id, gs->id );
  glDetachShader( p->id, vs->id );

  glDeleteShader( fs->id );
  glDeleteShader( gs->id );
  glDeleteShader( vs->id );

  return p->linked;
}

int32_t
mshgfx_shader_prog_use( const mshgfx_shader_prog_t *p )
{
  if ( p->linked ) {
    glUseProgram( p->id );
    return 1;
  }
  return 0;
}

int32_t
mshgfx_shader_prog_create_from_source_vf( mshgfx_shader_prog_t *p,
                                       const char *vs_src,
                                       const char *fs_src )
{

  mshgfx_shader_t vs, fs;
  vs.type = VERTEX_SHADER;
  fs.type = FRAGMENT_SHADER;

  if ( !mshgfx_shader_compile( &vs, vs_src ) )        return 0;
  if ( !mshgfx_shader_compile( &fs, fs_src ) )        return 0;
  if ( !mshgfx_shader_prog_link_vf( p, &vs, &fs ) )   return 0;

  return 1;
}

int32_t
mshgfx_shader_prog_create_from_source_vgf( mshgfx_shader_prog_t *p,
                                        const char * vs_src,
                                        const char * gs_src,
                                        const char * fs_src )
{
  mshgfx_shader_t vs, fs, gs;
  vs.type = VERTEX_SHADER;
  fs.type = FRAGMENT_SHADER;
  gs.type = GEOMETRY_SHADER;

  if ( !mshgfx_shader_compile( &vs, vs_src ) )            return 0;
  if ( !mshgfx_shader_compile( &gs, gs_src ) )            return 0;
  if ( !mshgfx_shader_compile( &fs, fs_src ) )            return 0;
  if ( !mshgfx_shader_prog_link_vgf( p, &vs, &gs, &fs ) ) return 0;

  return 1;
}

int32_t
mshgfx_shader_prog_create_from_files_vf( mshgfx_shader_prog_t *p,
                                      const char * vs_filename,
                                      const char * fs_filename )
{
  char *vs_source, *fs_source;
  if( !msh__shader_text_file_read( vs_filename, &vs_source ) ||
      !msh__shader_text_file_read( fs_filename, &fs_source ) )
  {
    return 0;
  }

  if( !mshgfx_shader_prog_create_from_source_vf( p, vs_source, fs_source ))
  {
    return 0;
  }

  free( vs_source );
  free( fs_source );

  return 1;
}

int32_t
mshgfx_shader_prog_create_from_files_vgf( mshgfx_shader_prog_t *p,
                                       const char * vs_filename,
                                       const char * gs_filename,
                                       const char * fs_filename )
{
  char *vs_source, *fs_source, *gs_source;
  if( !msh__shader_text_file_read( vs_filename, &vs_source ) ||
      !msh__shader_text_file_read( gs_filename, &gs_source ) ||
      !msh__shader_text_file_read( fs_filename, &fs_source ) )
  {
    return 0;
  }

  if( !mshgfx_shader_prog_create_from_source_vgf( p, 
                                              vs_source, 
                                              gs_source,
                                              fs_source ) )
  {
    return 0;
  }

  free( vs_source );
  free( fs_source );

  return 1;
}

void
mshgfx_shader_prog_set_uniform_1f( const mshgfx_shader_prog_t *p, 
                                const char *attrib_name, 
                                float x )
{
  GLuint location = glGetUniformLocation( p->id, attrib_name );
  glUniform1f( location, (GLfloat)x );
}

void
mshgfx_shader_prog_set_uniform_1i( const mshgfx_shader_prog_t *p, 
                               const char *attrib_name, 
                               int32_t x )
{
  GLuint location = glGetUniformLocation ( p->id, attrib_name );
  glUniform1i( location, (GLint)x );
}

void
mshgfx_shader_prog_set_uniform_1u( const mshgfx_shader_prog_t *p, 
                                const char *attrib_name, 
                                uint32_t x )
{
  GLuint location = glGetUniformLocation ( p->id, attrib_name );
  glUniform1ui( location, (GLuint)x );
}

void
mshgfx_shader_prog_set_uniform_2f( const mshgfx_shader_prog_t *p, 
                                const char *attrib_name, 
                                float x, float y )
{
  GLuint location = glGetUniformLocation( p->id, attrib_name );
  glUniform2f( location, x, y );
}

void
mshgfx_shader_prog_set_uniform_2i( const mshgfx_shader_prog_t *p, 
                                const char *attrib_name, 
                                int32_t x, int32_t y )
{
  GLuint location = glGetUniformLocation( p->id, attrib_name );
  glUniform2i( location, x, y );
}

void
mshgfx_shader_prog_set_uniform_2u( const mshgfx_shader_prog_t *p, 
                                const char *attrib_name, 
                                uint32_t x, uint32_t y )
{
  GLuint location = glGetUniformLocation( p->id, attrib_name );
  glUniform2ui( location, x, y );
}


void
mshgfx_shader_prog_set_uniform_3f( const mshgfx_shader_prog_t *p, 
                                const char *attrib_name, 
                                float x, float y, float z )
{
  GLuint location = glGetUniformLocation( p->id, attrib_name );
  glUniform3f( location, x, y, z );
}

void
mshgfx_shader_prog_set_uniform_3i( const mshgfx_shader_prog_t *p, 
                                const char *attrib_name, 
                                int32_t x, int32_t y, int32_t z )
{
  GLuint location = glGetUniformLocation( p->id, attrib_name );
  glUniform3i( location, x, y, z );
}

void
mshgfx_shader_prog_set_uniform_3u( const mshgfx_shader_prog_t *p, 
                                const char *attrib_name, 
                                uint32_t x, uint32_t y, uint32_t z )
{
  GLuint location = glGetUniformLocation( p->id, attrib_name );
  glUniform3ui( location, x, y, z );
}

void
mshgfx_shader_prog_set_uniform_4f( const mshgfx_shader_prog_t *p, 
                                const char *attrib_name, 
                                float x, float y, float z, float w )
{
  GLuint location = glGetUniformLocation( p->id, attrib_name );
  glUniform4f( location, x, y, z, w );
}

void
mshgfx_shader_prog_set_uniform_4i( const mshgfx_shader_prog_t *p, 
                                const char *attrib_name, 
                                int32_t x, int32_t y, int32_t z, int32_t w )
{
  GLuint location = glGetUniformLocation( p->id, attrib_name );
  glUniform4i( location, x, y, z, w );
}

void
mshgfx_shader_prog_set_uniform_4u( const mshgfx_shader_prog_t *p, 
                                const char *attrib_name, 
                                uint32_t x, uint32_t y, 
                                uint32_t z, uint32_t w )
{
  GLuint location = glGetUniformLocation( p->id, attrib_name );
  glUniform4ui( location, x, y, z, w );
}

#ifdef MSH_VEC_MATH
void
mshgfx_shader_prog_set_uniform_2fv( const mshgfx_shader_prog_t *p, 
                                   const char *attrib_name, const msh_vec2_t* v)
{
  GLuint location = glGetUniformLocation( p->id, attrib_name );
  float vec[2] = { (float)v->data[0], (float)v->data[1] };
  glUniform2fv( location, 1, &(vec[0]) );
}

// TODO: Fix this!!!
void
mshgfx_shader_prog_set_uniform_2fvc( const mshgfx_shader_prog_t *p, 
                                  const char *attrib_name, const msh_vec2_t* v, 
                                  const uint32_t count )
{
  GLuint location = glGetUniformLocation( p->id, attrib_name );
  float vec[2] = { (float)v->data[0], (float)v->data[1] };
  glUniform2fv( location, count, &( vec[0] ) );
}

void
mshgfx_shader_prog_set_uniform_3fv( const mshgfx_shader_prog_t *p, 
                                 const char *attrib_name, const msh_vec3_t * v)
{
  GLuint location = glGetUniformLocation( p->id, attrib_name );
  float vec[3] = { (float)v->data[0], (float)v->data[1], (float)v->data[2]  };
  glUniform3fv( location, 1, &(vec[0]) );
}

// TODO: Fix this!!!
void
mshgfx_shader_prog_set_uniform_3fvc( const mshgfx_shader_prog_t *p, 
                                  const char *attrib_name, const msh_vec3_t * v, 
                                  const uint32_t count )
{
  GLuint location = glGetUniformLocation( p->id, attrib_name );
  float vec[3] = { (float)v->data[0], (float)v->data[1], (float)v->data[2]  };
  glUniform3fv( location, count, &(vec[0] ) );
}

void
mshgfx_shader_prog_set_uniform_4fv( const mshgfx_shader_prog_t *p, 
                                  const char *attrib_name, const msh_vec4_t *v )
{
  GLuint location = glGetUniformLocation( p->id, attrib_name );
  float vec[4] = { (float)v->data[0], (float)v->data[1], (float)v->data[2], (float)v->data[3] };
  glUniform4fv( location, 1, &(vec[0]) );
}

void
mshgfx_shader_prog_set_uniform_4fvc( const mshgfx_shader_prog_t *p, 
                                  const char *attrib_name, const msh_vec4_t *v, 
                                  const uint32_t count )
{
  GLuint location = glGetUniformLocation( p->id, attrib_name );
  float vec[4] = { (float)v->data[0], (float)v->data[1], (float)v->data[2], (float)v->data[3] };
  glUniform4fv( location, count, &(vec[0]) );
}

void
mshgfx_shader_prog_set_uniform_3fm( const mshgfx_shader_prog_t *p, 
                                 const char *attrib_name, const msh_mat3_t *m )
{
  GLuint location = glGetUniformLocation( p->id, attrib_name );
  float mat[9] = { (float)m->data[0], (float)m->data[1], (float)m->data[2], 
                   (float)m->data[3], (float)m->data[4], (float)m->data[5], 
                   (float)m->data[6], (float)m->data[7], (float)m->data[8] };
  glUniformMatrix3fv( location, 1, GL_FALSE, &(mat[0]) );
}

void
mshgfx_shader_prog_set_uniform_3fmc( const mshgfx_shader_prog_t *p, 
                                  const char *attrib_name, const msh_mat3_t *m, 
                                  const uint32_t count )
{
  GLuint location = glGetUniformLocation( p->id, attrib_name );
  float mat[9] = { (float)m->data[0], (float)m->data[1], (float)m->data[2], 
                   (float)m->data[3], (float)m->data[4], (float)m->data[5], 
                   (float)m->data[6], (float)m->data[7], (float)m->data[8] };
  glUniformMatrix3fv( location, count, GL_FALSE, &(mat[0]) );
}

void
mshgfx_shader_prog_set_uniform_4fm( const mshgfx_shader_prog_t *p, 
                                 const char *attrib_name, const msh_mat4_t *m )
{
  GLuint location = glGetUniformLocation( p->id, attrib_name );
  float mat[16] = { (float)m->data[0], (float)m->data[1], (float)m->data[2], (float)m->data[3],
                   (float)m->data[4], (float)m->data[5], (float)m->data[6], (float)m->data[7],
                   (float)m->data[8], (float)m->data[9], (float)m->data[10], (float)m->data[11],
                   (float)m->data[12], (float)m->data[13], (float)m->data[14], (float)m->data[15] };
  glUniformMatrix4fv( location, 1, GL_FALSE, &(mat[0]) );
}

void
mshgfx_shader_prog_set_uniform_4fmc( const mshgfx_shader_prog_t *p, 
                                  const char *attrib_name, const msh_mat4_t *m, 
                                  const uint32_t count  )
{
  GLuint location = glGetUniformLocation( p->id, attrib_name );
  float mat[16] = { (float)m->data[0], (float)m->data[1], (float)m->data[2], (float)m->data[3],
                   (float)m->data[4], (float)m->data[5], (float)m->data[6], (float)m->data[7],
                   (float)m->data[8], (float)m->data[9], (float)m->data[10], (float)m->data[11],
                   (float)m->data[12], (float)m->data[13], (float)m->data[14], (float)m->data[15] };
  glUniformMatrix4fv( location, count, GL_FALSE, &(mat[0]) );
}

#endif /* MSH_VEC_MATH */



/*
 * =============================================================================
 *       GPU GEOMETRY IMPLEMENTATION
 * =============================================================================
 */

/* 
   calulates offset of requested flag into buffer stored in geo,
   depending on what has been requested 
*/
uint64_t
msh__gpu_geo_get_offset( const mshgfx_geometry_t * geo, 
                        const int32_t flag )
{
  uint64_t offset = 0;
  if ( geo->flags & POSITION )
  {
    if ( flag & POSITION ) return offset * geo->n_indices;  
    offset += 3 * sizeof(float); 
  }
  if ( geo->flags & NORMAL )
  {
    if ( flag & NORMAL ) return offset * geo->n_indices;
    offset += 3 * sizeof(float); 
  }
  if ( geo->flags & TANGENT )
  {
    if ( flag & TANGENT ) return offset * geo->n_indices;
    offset += 3 * sizeof(float); 
  }
  if ( geo->flags & TEX_COORD )
  {
    if ( flag & TEX_COORD ) return offset * geo->n_indices;
    offset += 2 * sizeof(float); 
  }
  if ( geo->flags & COLOR_A )
  {
    if ( flag & COLOR_A ) return offset * geo->n_indices;
    offset += 4 * sizeof(uint8_t); 
  }
  if ( geo->flags & COLOR_B )
  {
    if ( flag & COLOR_B ) return offset * geo->n_indices;
    offset += 4 * sizeof(uint8_t); 
  }
  if ( geo->flags & COLOR_C )
  {
    if ( flag & COLOR_C ) return offset * geo->n_indices;
    offset += 4 * sizeof(uint8_t); 
  }
  if ( geo->flags & COLOR_D )
  {
    if ( flag & COLOR_D ) return offset * geo->n_indices;
    offset += 4 * sizeof(uint8_t); 
  }
  return offset * geo->n_indices;
}

//TODO(maciej):Simplify the flags checking.
int32_t 
mshgfx_geometry_update( const mshgfx_geometry_t * geo,
                        const mshgfx_geometry_data_t * host_data, 
                         const int32_t flags )
{
  
  if( !(flags & geo->flags) )
  {
    /* printf( "Flag mismatch" __LINE__, "Requested flag update was not"
      "part of initialization" ); */
    return 0;
  }

  glBindVertexArray( geo->vao );
  glBindBuffer( GL_ARRAY_BUFFER, geo->vbo );
 
  if ( flags & POSITION )
  {
    uint64_t offset = msh__gpu_geo_get_offset( geo, flags & POSITION );
    uint64_t current_size = 3 * sizeof(float) * geo->n_indices;
    glBufferSubData( GL_ARRAY_BUFFER, offset, 
                                      current_size, 
                                      &(host_data->positions[0]) );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 
                           0, (void*) offset );
  }

  if ( flags & NORMAL )
  {
    uint64_t offset = msh__gpu_geo_get_offset( geo, flags & NORMAL);
    uint64_t current_size = 3 * sizeof(float) * geo->n_indices;
    glBufferSubData( GL_ARRAY_BUFFER, offset, 
                                      current_size, 
                                      &(host_data->normals[0]) );
    glEnableVertexAttribArray( 1 );
    glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 
                           0, (void*) offset );
  }

  if ( flags & TANGENT )
  {
    uint64_t offset = msh__gpu_geo_get_offset( geo, flags & TANGENT);
    uint64_t current_size = 3 * sizeof(float) * geo->n_indices;
    glBufferSubData( GL_ARRAY_BUFFER, offset, 
                                      current_size, 
                                      &(host_data->tangents[0]) );
    glEnableVertexAttribArray( 2 );
    glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, 
                           0, (void*) offset );
  }

  if ( flags & TEX_COORD )
  {
    uint64_t offset = msh__gpu_geo_get_offset( geo, flags & TEX_COORD);
    uint64_t current_size = 2 * sizeof(float) * geo->n_indices;
    glBufferSubData( GL_ARRAY_BUFFER, offset, 
                                      current_size, 
                                      &(host_data->texcoords[0]) );
    glEnableVertexAttribArray( 3 );
    glVertexAttribPointer( 3, 2, GL_FLOAT, GL_FALSE, 
                           0, (void*) offset );
  }

  if ( flags & COLOR_A )
  {
    uint64_t offset = msh__gpu_geo_get_offset( geo, flags & COLOR_A);
    uint64_t current_size = 4 * sizeof(uint8_t) * geo->n_indices;
    glBufferSubData( GL_ARRAY_BUFFER, offset, 
                                      current_size, 
                                      &(host_data->colors_a[0]) );
    glEnableVertexAttribArray( 4 );
    glVertexAttribPointer( 4, 4, GL_UNSIGNED_BYTE, GL_TRUE, 
                            0, (void*) offset );
  }

  if ( flags & COLOR_B )
  {
    uint64_t offset = msh__gpu_geo_get_offset( geo, flags & COLOR_B);
    uint64_t current_size = 4 * sizeof(uint8_t) * geo->n_indices;
    glBufferSubData( GL_ARRAY_BUFFER, offset, 
                                      current_size, 
                                      &(host_data->colors_b[0]) );
    glEnableVertexAttribArray( 5 );
    glVertexAttribPointer( 5, 4, GL_UNSIGNED_BYTE, GL_TRUE, 
                            0, (void*) offset );
  }

  if ( flags & COLOR_C )
  {
    uint64_t offset = msh__gpu_geo_get_offset( geo, flags & COLOR_C);
    uint64_t current_size = 4 * sizeof(uint8_t) * geo->n_indices;
    glBufferSubData( GL_ARRAY_BUFFER, offset, 
                                      current_size, 
                                      &(host_data->colors_c[0]) );
    glEnableVertexAttribArray( 6 );
    glVertexAttribPointer( 6, 4, GL_UNSIGNED_BYTE, GL_TRUE, 
                            0, (void*) offset );
  }

  if ( flags & COLOR_D )
  {
    uint64_t offset = msh__gpu_geo_get_offset( geo, flags & COLOR_D);
    uint64_t current_size = 4 * sizeof(uint8_t) * geo->n_indices;
    glBufferSubData( GL_ARRAY_BUFFER, offset, 
                                      current_size, 
                                      &(host_data->colors_d)[0] );
    glEnableVertexAttribArray( 7 );
    glVertexAttribPointer( 7, 4, GL_UNSIGNED_BYTE, GL_TRUE, 
                            0, (void*) offset );
  }

  glBindVertexArray(0);
  glBindBuffer( GL_ARRAY_BUFFER, 0 );

  return 1;
}


/* TODO: Errors if creating data fails!! */
int32_t 
mshgfx_geometry_init( mshgfx_geometry_t * geo, 
                      const mshgfx_geometry_data_t * host_data, 
                      const int32_t flags )
{
  /* Always use position */
  assert( flags & POSITION );

  glGenVertexArrays( 1, &(geo->vao)  );
  glGenBuffers( 1, &(geo->vbo) );
  if ( flags & STRUCTURED )
  {
    glGenBuffers( 1, &(geo->ebo) );
  }

  /* Initialize empty buffer */
  glBindVertexArray( geo->vao );
  glBindBuffer( GL_ARRAY_BUFFER, geo->vbo );

  unsigned long buf_size = 0;
  if ( flags & POSITION )  buf_size += 3 * sizeof(float); 
  if ( flags & NORMAL )    buf_size += 3 * sizeof(float);
  if ( flags & TANGENT )   buf_size += 3 * sizeof(float);
  if ( flags & TEX_COORD ) buf_size += 2 * sizeof(float);
  if ( flags & COLOR_A )   buf_size += 4 * sizeof(uint8_t);
  if ( flags & COLOR_B )   buf_size += 4 * sizeof(uint8_t);
  if ( flags & COLOR_C )   buf_size += 4 * sizeof(uint8_t);
  if ( flags & COLOR_D )   buf_size += 4 * sizeof(uint8_t);
  buf_size *= host_data->n_vertices;
  
  glBufferData( GL_ARRAY_BUFFER, buf_size, NULL, GL_STATIC_DRAW);

  geo->flags       = flags;
  geo->n_indices   = host_data->n_vertices;
  geo->n_elements  = host_data->n_elements;
  geo->buffer_size = buf_size;

  if ( !mshgfx_geometry_update( geo, host_data, flags ) )
  {
    return 0;
  }

  if ( flags & STRUCTURED )
  {  
    glBindVertexArray( geo->vao );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, geo->ebo );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 
                  geo->n_elements * sizeof( uint32_t ), 
                  &(host_data->indices[0]), 
                  GL_STATIC_DRAW ); 
  }

  glBindVertexArray(0);
  glBindBuffer( GL_ARRAY_BUFFER, 0 );
  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 ); 
  return 1;
}

int32_t 
mshgfx_geometry_free( mshgfx_geometry_t * geo )
{
  glDeleteBuffers(1, &(geo->vbo) );
  if ( geo->flags & STRUCTURED )
  {
    glDeleteBuffers(1, &(geo->ebo) );
  }
  glDeleteVertexArrays( 1, &(geo->vao) );
  geo->vao = -1;
  geo->vbo = -1;
  geo->ebo = -1;
  geo->n_indices  = -1;
  geo->n_elements = -1;
  return 1;
}

void 
mshgfx_geometry_draw( mshgfx_geometry_t * geo, 
                      uint32_t draw_mode ) 
{
  msh_geometry_properties_flags flags = geo->flags;

  glBindVertexArray( geo->vao );
  
  if ( flags & STRUCTURED )
  {

    glDrawElements( draw_mode, geo->n_elements, GL_UNSIGNED_INT, 0 );
  }
  else
  {
    glDrawArrays( draw_mode, 0, geo->n_indices );
  }

  glBindVertexArray( 0 );
}

/*
 * =============================================================================
 *       TEXTURES IMPLEMENTATION
 * =============================================================================
 */

/* TODO (maciej): Format defining macro? */

void 
mshgfx_texture1d_update( mshgfx_texture1d_t *tex, 
                         const void *data, const uint32_t type )
{
  glActiveTexture ( GL_TEXTURE0 + tex->unit );
  glBindTexture( GL_TEXTURE_2D, tex->id );

  GLint internal_format;
  uint32_t format = 0;
  switch( tex->n_comp )
  {
    case 1:
      format = GL_RED;
      if( type == GL_UNSIGNED_BYTE )  internal_format = GL_R8;
      if( type == GL_UNSIGNED_SHORT ) internal_format = GL_R16; 
      if( type == GL_UNSIGNED_INT )   internal_format = GL_R32UI;
      if( type == GL_FLOAT )          internal_format = GL_R32F;
      break;
    case 2:
      format = GL_RG;
      if( type == GL_UNSIGNED_BYTE )  internal_format = GL_RG;
      if( type == GL_UNSIGNED_SHORT ) internal_format = GL_RG16;
      if( type == GL_UNSIGNED_INT )   internal_format = GL_RG32UI;
      if( type == GL_FLOAT )          internal_format = GL_RG32F;
      break;
    case 3:
      format = GL_RGB;
      if( type == GL_UNSIGNED_BYTE )  internal_format = GL_RGB8;
      if( type == GL_UNSIGNED_SHORT ) internal_format = GL_RGB16;
      if( type == GL_UNSIGNED_INT )   internal_format = GL_RGB32UI;
      if( type == GL_FLOAT )          internal_format = GL_RGB32F;
      break;
    case 4:
      format = GL_RGBA;
      if( type == GL_UNSIGNED_BYTE )  internal_format = GL_RGBA8;
      if( type == GL_UNSIGNED_SHORT ) internal_format = GL_RGBA16;
      if( type == GL_UNSIGNED_INT )   internal_format = GL_RGBA32UI;
      if( type == GL_FLOAT )          internal_format = GL_RGBA32F;
      break;
    default:
      internal_format = GL_RGBA;
      format = GL_RGB;
  }

  glTexImage1D( GL_TEXTURE_1D, 0, internal_format, 
                tex->width, 0, format, type, data );
  glBindTexture( GL_TEXTURE_1D, 0 );
}

void 
mshgfx_texture1d_init( mshgfx_texture1d_t *tex,
                       const void *data,
                       const int32_t type,
                       const int32_t w, 
                       const int32_t n_comp,  
                       const uint32_t unit,
                       const int32_t user_flags )
{
  tex->width  = w;
  tex->n_comp = n_comp; 
  tex->type   = type;
  tex->unit   = unit;
  glGenTextures( 1, &tex->id );
  
  glActiveTexture ( GL_TEXTURE0 + tex->unit );
  glBindTexture( GL_TEXTURE_1D, tex->id );

  GLuint filtering = GL_NEAREST;
  GLuint wrapping   = GL_CLAMP_TO_EDGE;
  if ( user_flags & MSH_NEAREST )              filtering = GL_NEAREST; 
  if ( user_flags & MSH_LINEAR )               filtering = GL_LINEAR;
  if ( user_flags & MSH_CLAMP_TO_EDGE )        wrapping = GL_CLAMP_TO_EDGE;       
  if ( user_flags & MSH_CLAMP_TO_BORDER )      wrapping = GL_CLAMP_TO_BORDER;     
  if ( user_flags & MSH_REPEAT )               wrapping = GL_REPEAT;              
  if ( user_flags & MSH_MIRRORED_REPEAT )      wrapping = GL_MIRRORED_REPEAT;     

  glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, filtering );
  glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, filtering );
  glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, wrapping );

  mshgfx_texture1d_update( tex, data, type );

  glBindTexture( GL_TEXTURE_1D, 0 );
}

void 
mshgfx_texture1d_use( const mshgfx_texture1d_t *tex)
{
    glActiveTexture( GL_TEXTURE0 + tex->unit );
    glBindTexture( GL_TEXTURE_1D, tex->id );
}

void 
mshgfx_texture1d_free( mshgfx_texture1d_t *tex )
{
    glDeleteTextures( 1, &tex->id );
    tex->id = 0;
}


void 
mshgfx_texture2d_update( mshgfx_texture2d_t *tex, 
                         const void *data, const uint32_t type )
{
  glActiveTexture ( GL_TEXTURE0 + tex->unit );
  glBindTexture( GL_TEXTURE_2D, tex->id );

  GLint internal_format;
  uint32_t format = 0;
  switch( tex->n_comp )
  {
    case 1:
      format = GL_RED;
      if( type == GL_UNSIGNED_BYTE )  internal_format = GL_R8;
      if( type == GL_UNSIGNED_SHORT ) internal_format = GL_R16; 
      if( type == GL_UNSIGNED_INT )   internal_format = GL_R32UI;
      if( type == GL_FLOAT )          internal_format = GL_R32F;
      break;
    case 2:
      format = GL_RG;
      if( type == GL_UNSIGNED_BYTE )  internal_format = GL_RG;
      if( type == GL_UNSIGNED_SHORT ) internal_format = GL_RG16;
      if( type == GL_UNSIGNED_INT )   internal_format = GL_RG32UI;
      if( type == GL_FLOAT )          internal_format = GL_RG32F;
      break;
    case 3:
      format = GL_RGB;
      if( type == GL_UNSIGNED_BYTE )  internal_format = GL_RGB8;
      if( type == GL_UNSIGNED_SHORT ) internal_format = GL_RGB16;
      if( type == GL_UNSIGNED_INT )   internal_format = GL_RGB32UI;
      if( type == GL_FLOAT )          internal_format = GL_RGB32F;
      break;
    case 4:
      format = GL_RGBA;
      if( type == GL_UNSIGNED_BYTE )  internal_format = GL_RGBA8;
      if( type == GL_UNSIGNED_SHORT ) internal_format = GL_RGBA16;
      if( type == GL_UNSIGNED_INT )   internal_format = GL_RGBA32UI;
      if( type == GL_FLOAT )          internal_format = GL_RGBA32F;
      break;
    default:
      internal_format = GL_RGBA;
      format = GL_RGB;
  }

  glTexImage2D( GL_TEXTURE_2D, 0, internal_format, 
                tex->width, tex->height, 0, format, type, data );
  glBindTexture( GL_TEXTURE_2D, 0 );
}

void 
mshgfx_texture2d_init( mshgfx_texture2d_t *tex,
                       const void *data,
                       const int32_t type,
                       const int32_t w, 
                       const int32_t h, 
                       const int32_t n_comp,  
                       const uint32_t unit,
                       const int32_t user_flags )
{
  tex->width  = w;
  tex->height = h;
  tex->n_comp = n_comp; 
  tex->type   = type;
  tex->unit   = unit;
  glGenTextures( 1, &tex->id );
  
  glActiveTexture ( GL_TEXTURE0 + tex->unit );
  glBindTexture( GL_TEXTURE_2D, tex->id );

  GLuint filtering = GL_NEAREST;
  GLuint wrapping   = GL_CLAMP_TO_EDGE;
  if ( user_flags & MSH_NEAREST )              filtering = GL_NEAREST; 
  if ( user_flags & MSH_LINEAR )               filtering = GL_LINEAR;
  if ( user_flags & MSH_CLAMP_TO_EDGE )        wrapping = GL_CLAMP_TO_EDGE;       
  if ( user_flags & MSH_CLAMP_TO_BORDER )      wrapping = GL_CLAMP_TO_BORDER;     
  if ( user_flags & MSH_REPEAT )               wrapping = GL_REPEAT;              
  if ( user_flags & MSH_MIRRORED_REPEAT )      wrapping = GL_MIRRORED_REPEAT;     

  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapping );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapping );

  mshgfx_texture2d_update( tex, data, type );

  glBindTexture( GL_TEXTURE_2D, 0 );
}

void 
mshgfx_texture2d_use( const mshgfx_texture2d_t *tex)
{
    glActiveTexture( GL_TEXTURE0 + tex->unit );
    glBindTexture( GL_TEXTURE_2D, tex->id );
}

void 
mshgfx_texture2d_free( mshgfx_texture2d_t *tex )
{
    glDeleteTextures( 1, &tex->id );
    tex->id = 0;
}

void 
mshgfx_texture3d_update( mshgfx_texture3d_t *tex, 
                         const void *data, const uint32_t type )
{
  glActiveTexture ( GL_TEXTURE0 + tex->unit );
  glBindTexture( GL_TEXTURE_3D, tex->id );

  GLint internal_format;
  uint32_t format = 0;
  switch( tex->n_comp )
  {
    case 1:
      format = GL_RED;
      if( type == GL_UNSIGNED_BYTE )  internal_format = GL_R8;
      if( type == GL_UNSIGNED_SHORT ) internal_format = GL_R16;
      if( type == GL_UNSIGNED_INT )   internal_format = GL_R32UI;
      if( type == GL_FLOAT )          internal_format = GL_R32F;
      break;
    case 2:
      format = GL_RG;
      if( type == GL_UNSIGNED_BYTE )  internal_format = GL_RG;
      if( type == GL_UNSIGNED_SHORT ) internal_format = GL_RG16;
      if( type == GL_UNSIGNED_INT )   internal_format = GL_RG32UI;
      if( type == GL_FLOAT )          internal_format = GL_RG32F;
      break;
    case 3:
      format = GL_RGB;
      if( type == GL_UNSIGNED_BYTE )  internal_format = GL_RGB8;
      if( type == GL_UNSIGNED_SHORT ) internal_format = GL_RGB16;
      if( type == GL_UNSIGNED_INT )   internal_format = GL_RGB32UI;
      if( type == GL_FLOAT )          internal_format = GL_RGB32F;
      break;
    case 4:
      format = GL_RGBA;
      if( type == GL_UNSIGNED_BYTE )  internal_format = GL_RGBA8;
      if( type == GL_UNSIGNED_SHORT ) internal_format = GL_RGBA16;
      if( type == GL_UNSIGNED_INT )   internal_format = GL_RGBA32UI;
      if( type == GL_FLOAT )          internal_format = GL_RGBA32F;
      break;
    default:
      internal_format = GL_RGBA;
      format = GL_RGB;
  }


  glTexImage3D( GL_TEXTURE_3D, 0, internal_format, 
                tex->width, tex->height, tex->depth, 0, format, type, data );
  glBindTexture( GL_TEXTURE_3D, 0 );
}



void 
mshgfx_texture3d_init( mshgfx_texture3d_t *tex,
                       const void *data, 
                       const int32_t type,
                       const int32_t w, 
                       const int32_t h,
                       const int32_t d, 
                       const int32_t n_comp,  
                       const uint32_t unit,
                       const int32_t user_flags )
{
  tex->width  = w;
  tex->height = h;
  tex->depth  = d;
  tex->n_comp = n_comp; 
  tex->type   = type;
  tex->unit   = unit;
  glGenTextures( 1, &tex->id );
  
  glActiveTexture ( GL_TEXTURE0 + tex->unit );
  glBindTexture( GL_TEXTURE_3D, tex->id );

  GLuint filtering = GL_NEAREST;
  GLuint wrapping   = GL_CLAMP_TO_EDGE;
  if ( user_flags & MSH_NEAREST )              filtering = GL_NEAREST; 
  if ( user_flags & MSH_LINEAR )               filtering = GL_LINEAR;
  if ( user_flags & MSH_CLAMP_TO_EDGE )        wrapping = GL_CLAMP_TO_EDGE;       
  if ( user_flags & MSH_CLAMP_TO_BORDER )      wrapping = GL_CLAMP_TO_BORDER;     
  if ( user_flags & MSH_REPEAT )               wrapping = GL_REPEAT;              
  if ( user_flags & MSH_MIRRORED_REPEAT )      wrapping = GL_MIRRORED_REPEAT;     

  glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, filtering );
  glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, filtering );
  glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, wrapping );
  glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, wrapping );
  glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, wrapping );

  mshgfx_texture3d_update( tex, data, type );

  glBindTexture( GL_TEXTURE_3D, 0 );
}


void 
mshgfx_texture3d_use( const mshgfx_texture3d_t *tex)
{
    glActiveTexture( GL_TEXTURE0 + tex->unit );
    glBindTexture( GL_TEXTURE_3D, tex->id );
}

void 
mshgfx_texture3d_free( mshgfx_texture3d_t *tex )
{
    glDeleteTextures( 1, &tex->id );
    tex->id = 0;
}



#endif /* MSH_GFX_IMPLEMENTATION */
