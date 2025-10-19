/* Conditional compilation for bgfx vs OpenGL backend */
#if defined(CGNS_ENABLE_BGFX) && !defined(TKOGL_NEEDS_REAL_GL)
/* When using bgfx backend, we still need some GL constants for compatibility */
/* but we include the render backend instead of actual OpenGL headers */
/* Exception: tkogl widget needs real OpenGL headers for widget management */
#include "render_backend.h"

/* Define GL constants and types used in cgnsplot for compatibility */
#ifndef GL_LINES
/* Primitive types */
#define GL_LINES                  CGNS_PRIM_LINES
#define GL_TRIANGLES              CGNS_PRIM_TRIANGLES
#define GL_QUADS                  CGNS_PRIM_QUADS
#define GL_POLYGON                CGNS_PRIM_POLYGON
/* Note: TRIANGLE_FAN not yet in render backend, using TRIANGLES for now */
#define GL_TRIANGLE_FAN           CGNS_PRIM_TRIANGLES

/* State flags */
#define GL_LIGHTING               0x0B50
#define GL_DEPTH_TEST             0x0B71
#define GL_BLEND                  0x0BE2

/* Shading models */
#define GL_FLAT                   CGNS_SHADE_FLAT
#define GL_SMOOTH                 CGNS_SHADE_SMOOTH

/* Display list modes */
#define GL_COMPILE                0x1300

/* Polygon modes */
#define GL_LINE                   0x1B01
#define GL_FILL                   0x1B02

/* Material parameters */
#define GL_FRONT_AND_BACK         0x0408
#define GL_AMBIENT_AND_DIFFUSE    0x1602

/* Buffer bits */
#define GL_COLOR_BUFFER_BIT       0x00004000
#define GL_DEPTH_BUFFER_BIT       0x00000100

/* Blending */
#define GL_SRC_ALPHA              0x0302
#define GL_ONE_MINUS_SRC_ALPHA    0x0303

/* Boolean */
#define GL_FALSE                  0
#define GL_TRUE                   1

/* Pixel store */
#define GL_UNPACK_ALIGNMENT       0x0CF5

/* Types */
typedef unsigned char GLubyte;
#endif

#else
/* Use standard OpenGL headers */
#ifdef _WIN32
#include <GL/gl.h>
#include <GL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#endif
#endif /* CGNS_ENABLE_BGFX */

