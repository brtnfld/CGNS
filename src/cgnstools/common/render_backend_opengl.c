/**
 * render_backend_opengl.c
 *
 * OpenGL backend implementation for CGNS visualization tools.
 * Provides legacy OpenGL immediate-mode rendering support.
 *
 * This is essentially a thin wrapper around existing OpenGL calls,
 * allowing the codebase to gradually migrate while maintaining
 * full backward compatibility.
 */

#include "render_backend.h"
#include "gl_config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ========================================================================
 * Internal Data Structures
 * ======================================================================== */

/**
 * OpenGL-specific context state
 */
struct cgns_render_context {
    void* platform_data;           /* Platform window handle */

    /* Current state tracking */
    cgns_primitive_type_t current_primitive;
    float current_normal[3];
    float current_color[4];
    int in_begin_end;

    /* Display list state */
    int recording_list;
    unsigned int active_list;

    /* Error handling */
    char error_msg[256];
};

/* Global backend state */
static cgns_render_backend_t g_active_backend = CGNS_RENDER_BACKEND_OPENGL;
static const char* g_no_error = NULL;

/* ========================================================================
 * Helper Functions
 * ======================================================================== */

/**
 * Convert CGNS primitive type to OpenGL enum
 */
static GLenum primitive_to_gl(cgns_primitive_type_t type)
{
    switch (type) {
        case CGNS_PRIM_LINES:     return GL_LINES;
        case CGNS_PRIM_TRIANGLES: return GL_TRIANGLES;
        case CGNS_PRIM_QUADS:     return GL_QUADS;
        case CGNS_PRIM_POLYGON:   return GL_POLYGON;
        default:                  return GL_TRIANGLES;
    }
}

/**
 * Set error message
 */
static void set_error(cgns_render_context_t* ctx, const char* msg)
{
    if (ctx && msg) {
        strncpy(ctx->error_msg, msg, sizeof(ctx->error_msg) - 1);
        ctx->error_msg[sizeof(ctx->error_msg) - 1] = '\0';
    }
}

/* ========================================================================
 * Backend Selection and Configuration
 * ======================================================================== */

cgns_render_backend_t cgns_render_get_backend(void)
{
    return g_active_backend;
}

int cgns_render_set_backend(cgns_render_backend_t backend)
{
    if (backend != CGNS_RENDER_BACKEND_OPENGL) {
        /* For Phase 1, only OpenGL is implemented */
        return -1;
    }
    g_active_backend = backend;
    return 0;
}

const char* cgns_render_backend_name(cgns_render_backend_t backend)
{
    switch (backend) {
        case CGNS_RENDER_BACKEND_OPENGL: return "OpenGL";
        case CGNS_RENDER_BACKEND_BGFX:   return "bgfx";
        default:                         return "Unknown";
    }
}

int cgns_render_backend_available(cgns_render_backend_t backend)
{
    switch (backend) {
        case CGNS_RENDER_BACKEND_OPENGL:
            /* OpenGL is always available in this build */
            return 1;
        case CGNS_RENDER_BACKEND_BGFX:
            /* Phase 1: bgfx not yet implemented */
            return 0;
        default:
            return 0;
    }
}

/* ========================================================================
 * Backend Lifecycle Management
 * ======================================================================== */

cgns_render_context_t* cgns_render_initialize(void* platform_data)
{
    cgns_render_context_t* ctx;

    ctx = (cgns_render_context_t*)malloc(sizeof(cgns_render_context_t));
    if (!ctx) {
        return NULL;
    }

    memset(ctx, 0, sizeof(cgns_render_context_t));
    ctx->platform_data = platform_data;

    /* Initialize state */
    ctx->current_color[0] = 1.0f;
    ctx->current_color[1] = 1.0f;
    ctx->current_color[2] = 1.0f;
    ctx->current_color[3] = 1.0f;

    ctx->current_normal[0] = 0.0f;
    ctx->current_normal[1] = 0.0f;
    ctx->current_normal[2] = 1.0f;

    ctx->in_begin_end = 0;
    ctx->recording_list = 0;

    /* OpenGL context is managed by tkogl, so we don't need to create it here */

    return ctx;
}

void cgns_render_shutdown(cgns_render_context_t* ctx)
{
    if (ctx) {
        free(ctx);
    }
}

int cgns_render_make_current(cgns_render_context_t* ctx)
{
    if (!ctx) {
        return -1;
    }

    /* OpenGL context switching is handled by tkogl/platform code */
    /* This is a no-op for the abstraction layer */

    return 0;
}

/* ========================================================================
 * Frame Management
 * ======================================================================== */

void cgns_render_begin_frame(cgns_render_context_t* ctx)
{
    /* No-op for OpenGL - frame management handled by tkogl */
    (void)ctx;
}

void cgns_render_end_frame(cgns_render_context_t* ctx)
{
    /* No-op for OpenGL - buffer swap handled by tkogl */
    (void)ctx;
    glFlush();
}

void cgns_render_clear(cgns_render_context_t* ctx,
                       float r, float g, float b, float a)
{
    (void)ctx;
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/* ========================================================================
 * Viewport and Transformation
 * ======================================================================== */

void cgns_render_set_viewport(cgns_render_context_t* ctx,
                               const cgns_viewport_t* viewport)
{
    (void)ctx;
    if (viewport) {
        glViewport(viewport->x, viewport->y, viewport->width, viewport->height);
    }
}

void cgns_render_set_projection(cgns_render_context_t* ctx,
                                 const float* matrix)
{
    (void)ctx;
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(matrix);
}

void cgns_render_set_view(cgns_render_context_t* ctx,
                          const float* matrix)
{
    (void)ctx;
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(matrix);
}

void cgns_render_set_model(cgns_render_context_t* ctx,
                           const float* matrix)
{
    (void)ctx;
    glMatrixMode(GL_MODELVIEW);
    glMultMatrixf(matrix);
}

/* ========================================================================
 * Render State Management
 * ======================================================================== */

void cgns_render_enable(cgns_render_context_t* ctx, cgns_render_state_t state)
{
    (void)ctx;

    if (state & CGNS_STATE_LIGHTING) {
        glEnable(GL_LIGHTING);
    }
    if (state & CGNS_STATE_DEPTH_TEST) {
        glEnable(GL_DEPTH_TEST);
    }
    if (state & CGNS_STATE_BLEND) {
        glEnable(GL_BLEND);
    }
    if (state & CGNS_STATE_CULL_FACE) {
        glEnable(GL_CULL_FACE);
    }
}

void cgns_render_disable(cgns_render_context_t* ctx, cgns_render_state_t state)
{
    (void)ctx;

    if (state & CGNS_STATE_LIGHTING) {
        glDisable(GL_LIGHTING);
    }
    if (state & CGNS_STATE_DEPTH_TEST) {
        glDisable(GL_DEPTH_TEST);
    }
    if (state & CGNS_STATE_BLEND) {
        glDisable(GL_BLEND);
    }
    if (state & CGNS_STATE_CULL_FACE) {
        glDisable(GL_CULL_FACE);
    }
}

void cgns_render_set_shade_model(cgns_render_context_t* ctx,
                                  cgns_shade_model_t model)
{
    (void)ctx;

    switch (model) {
        case CGNS_SHADE_FLAT:
            glShadeModel(GL_FLAT);
            break;
        case CGNS_SHADE_SMOOTH:
            glShadeModel(GL_SMOOTH);
            break;
    }
}

void cgns_render_set_polygon_mode(cgns_render_context_t* ctx,
                                   cgns_polygon_mode_t mode)
{
    (void)ctx;

    GLenum gl_mode;
    switch (mode) {
        case CGNS_POLY_FILL:
            gl_mode = GL_FILL;
            break;
        case CGNS_POLY_LINE:
            gl_mode = GL_LINE;
            break;
        case CGNS_POLY_POINT:
            gl_mode = GL_POINT;
            break;
        default:
            gl_mode = GL_FILL;
    }

    glPolygonMode(GL_FRONT_AND_BACK, gl_mode);
}

void cgns_render_set_color3f(cgns_render_context_t* ctx,
                             float r, float g, float b)
{
    if (ctx) {
        ctx->current_color[0] = r;
        ctx->current_color[1] = g;
        ctx->current_color[2] = b;
        ctx->current_color[3] = 1.0f;
    }
    glColor3f(r, g, b);
}

void cgns_render_set_color4f(cgns_render_context_t* ctx,
                             float r, float g, float b, float a)
{
    if (ctx) {
        ctx->current_color[0] = r;
        ctx->current_color[1] = g;
        ctx->current_color[2] = b;
        ctx->current_color[3] = a;
    }
    glColor4f(r, g, b, a);
}

void cgns_render_set_material(cgns_render_context_t* ctx,
                              const cgns_material_t* material)
{
    (void)ctx;

    if (material) {
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material->ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material->diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material->specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, material->emission);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, material->shininess);
    }
}

/* ========================================================================
 * Immediate-Mode Style Rendering
 * ======================================================================== */

void cgns_render_begin(cgns_render_context_t* ctx, cgns_primitive_type_t type)
{
    if (ctx) {
        ctx->current_primitive = type;
        ctx->in_begin_end = 1;
    }

    glBegin(primitive_to_gl(type));
}

void cgns_render_vertex3fv(cgns_render_context_t* ctx, const float* v)
{
    (void)ctx;
    glVertex3fv(v);
}

void cgns_render_vertex3f(cgns_render_context_t* ctx,
                         float x, float y, float z)
{
    (void)ctx;
    glVertex3f(x, y, z);
}

void cgns_render_normal3fv(cgns_render_context_t* ctx, const float* n)
{
    if (ctx) {
        ctx->current_normal[0] = n[0];
        ctx->current_normal[1] = n[1];
        ctx->current_normal[2] = n[2];
    }
    glNormal3fv(n);
}

void cgns_render_normal3f(cgns_render_context_t* ctx,
                         float nx, float ny, float nz)
{
    if (ctx) {
        ctx->current_normal[0] = nx;
        ctx->current_normal[1] = ny;
        ctx->current_normal[2] = nz;
    }
    glNormal3f(nx, ny, nz);
}

void cgns_render_end(cgns_render_context_t* ctx)
{
    if (ctx) {
        ctx->in_begin_end = 0;
    }
    glEnd();
}

/* ========================================================================
 * Batch Rendering
 * ======================================================================== */

void cgns_render_draw_batch(cgns_render_context_t* ctx,
                            cgns_primitive_type_t type,
                            const cgns_vertex_t* vertices,
                            size_t count)
{
    size_t i;

    if (!vertices || count == 0) {
        return;
    }

    cgns_render_begin(ctx, type);

    for (i = 0; i < count; i++) {
        const cgns_vertex_t* v = &vertices[i];
        glColor4fv(v->color);
        glNormal3fv(v->normal);
        glVertex3fv(v->position);
    }

    cgns_render_end(ctx);
}

/* ========================================================================
 * Display Lists
 * ======================================================================== */

unsigned int cgns_render_gen_list(cgns_render_context_t* ctx)
{
    (void)ctx;
    return glGenLists(1);
}

void cgns_render_new_list(cgns_render_context_t* ctx, unsigned int list)
{
    if (ctx) {
        ctx->recording_list = 1;
        ctx->active_list = list;
    }
    glNewList(list, GL_COMPILE);
}

void cgns_render_end_list(cgns_render_context_t* ctx)
{
    if (ctx) {
        ctx->recording_list = 0;
    }
    glEndList();
}

void cgns_render_call_list(cgns_render_context_t* ctx, unsigned int list)
{
    (void)ctx;
    glCallList(list);
}

void cgns_render_delete_list(cgns_render_context_t* ctx, unsigned int list)
{
    (void)ctx;
    glDeleteLists(list, 1);
}

/* ========================================================================
 * Error Handling
 * ======================================================================== */

const char* cgns_render_get_error(cgns_render_context_t* ctx)
{
    GLenum err;

    if (!ctx) {
        return "Invalid context";
    }

    /* Check for OpenGL errors */
    err = glGetError();
    if (err != GL_NO_ERROR) {
        switch (err) {
            case GL_INVALID_ENUM:
                set_error(ctx, "OpenGL: Invalid enum");
                break;
            case GL_INVALID_VALUE:
                set_error(ctx, "OpenGL: Invalid value");
                break;
            case GL_INVALID_OPERATION:
                set_error(ctx, "OpenGL: Invalid operation");
                break;
            case GL_OUT_OF_MEMORY:
                set_error(ctx, "OpenGL: Out of memory");
                break;
            default:
                set_error(ctx, "OpenGL: Unknown error");
                break;
        }
        return ctx->error_msg;
    }

    return g_no_error;
}
