/**
 * render_backend.h
 *
 * Graphics API abstraction layer for CGNS visualization tools.
 * Provides a unified interface for OpenGL (legacy) and bgfx (modern) backends.
 *
 * Phase 1: Foundation - Backend Abstraction Layer
 * This allows cgnsplot and cgnsview to work with multiple rendering backends
 * while maintaining backward compatibility with existing OpenGL code.
 */

#ifndef CGNS_RENDER_BACKEND_H
#define CGNS_RENDER_BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

/* Include platform-specific headers */
#if defined(__WIN32__) || defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   undef WIN32_LEAN_AND_MEAN
#endif

#include <stddef.h>
#include <stdint.h>

/* ========================================================================
 * Backend Selection and Configuration
 * ======================================================================== */

/**
 * Supported rendering backends.
 * OPENGL: Legacy OpenGL (1.x/2.x immediate mode) - maximum compatibility
 * BGFX:   Modern multi-platform abstraction (Vulkan/Metal/DX12/OpenGL)
 */
typedef enum {
    CGNS_RENDER_BACKEND_OPENGL = 0,  /* Default - legacy support */
    CGNS_RENDER_BACKEND_BGFX = 1     /* Modern multi-API backend */
} cgns_render_backend_t;

/**
 * Rendering primitive types.
 * Maps to both OpenGL and bgfx primitive types.
 */
typedef enum {
    CGNS_PRIM_POINTS = 0,
    CGNS_PRIM_LINES = 1,
    CGNS_PRIM_TRIANGLES = 2,
    CGNS_PRIM_QUADS = 3,         /* Will be triangulated for bgfx */
    CGNS_PRIM_POLYGON = 4        /* Will be triangulated for bgfx */
} cgns_primitive_type_t;

/**
 * Shading models
 */
typedef enum {
    CGNS_SHADE_FLAT = 0,
    CGNS_SHADE_SMOOTH = 1
} cgns_shade_model_t;

/**
 * Polygon modes
 */
typedef enum {
    CGNS_POLY_FILL = 0,
    CGNS_POLY_LINE = 1,
    CGNS_POLY_POINT = 2
} cgns_polygon_mode_t;

/**
 * Render state flags
 */
typedef enum {
    CGNS_STATE_LIGHTING = (1 << 0),
    CGNS_STATE_DEPTH_TEST = (1 << 1),
    CGNS_STATE_BLEND = (1 << 2),
    CGNS_STATE_CULL_FACE = (1 << 3)
} cgns_render_state_t;

/* ========================================================================
 * Data Structures
 * ======================================================================== */

/**
 * Opaque context handle for the rendering backend.
 * Contains backend-specific state (OpenGL context or bgfx state).
 */
typedef struct cgns_render_context cgns_render_context_t;

/**
 * Vertex format for immediate-mode style rendering.
 * Compatible with legacy glVertex3fv/glNormal3fv pattern.
 */
typedef struct {
    float position[3];
    float normal[3];
    float color[4];
} cgns_vertex_t;

/**
 * Material properties
 */
typedef struct {
    float ambient[4];
    float diffuse[4];
    float specular[4];
    float emission[4];
    float shininess;
} cgns_material_t;

/**
 * Light properties
 */
typedef struct {
    float position[4];
    float ambient[4];
    float diffuse[4];
    float specular[4];
} cgns_light_t;

/**
 * View/Camera parameters
 */
typedef struct {
    int x, y;                /* Viewport origin */
    int width, height;       /* Viewport dimensions */
    float aspect_ratio;      /* If > 0, maintain aspect ratio */
} cgns_viewport_t;

/* ========================================================================
 * Backend Lifecycle Management
 * ======================================================================== */

/**
 * Get the currently active backend type.
 * @return The active backend (OPENGL or BGFX)
 */
cgns_render_backend_t cgns_render_get_backend(void);

/**
 * Set the desired backend type.
 * Must be called before cgns_render_initialize().
 * @param backend The backend to use
 * @return 0 on success, non-zero on error
 */
int cgns_render_set_backend(cgns_render_backend_t backend);

/**
 * Initialize the rendering backend.
 * Creates context and allocates resources.
 *
 * @param platform_data Platform-specific window handle/context
 *                      - For X11: Display*
 *                      - For Win32: HWND
 *                      - For bgfx: native window handle
 * @return Context handle on success, NULL on failure
 */
cgns_render_context_t* cgns_render_initialize(void* platform_data);

/**
 * Shutdown the rendering backend and free resources.
 * @param ctx Context to destroy
 */
void cgns_render_shutdown(cgns_render_context_t* ctx);

/**
 * Make the given context current for rendering.
 * @param ctx Context to activate
 * @return 0 on success, non-zero on error
 */
int cgns_render_make_current(cgns_render_context_t* ctx);

/* ========================================================================
 * Frame Management
 * ======================================================================== */

/**
 * Begin a new frame.
 * Must be called before any rendering commands.
 * @param ctx Rendering context
 */
void cgns_render_begin_frame(cgns_render_context_t* ctx);

/**
 * End the current frame and present/swap buffers.
 * @param ctx Rendering context
 */
void cgns_render_end_frame(cgns_render_context_t* ctx);

/**
 * Clear the framebuffer.
 * @param ctx Rendering context
 * @param r Red component (0.0 - 1.0)
 * @param g Green component (0.0 - 1.0)
 * @param b Blue component (0.0 - 1.0)
 * @param a Alpha component (0.0 - 1.0)
 */
void cgns_render_clear(cgns_render_context_t* ctx,
                       float r, float g, float b, float a);

/* ========================================================================
 * Viewport and Transformation
 * ======================================================================== */

/**
 * Set the viewport.
 * @param ctx Rendering context
 * @param viewport Viewport parameters
 */
void cgns_render_set_viewport(cgns_render_context_t* ctx,
                               const cgns_viewport_t* viewport);

/**
 * Set projection matrix (for now, pass-through to OpenGL style).
 * In bgfx backend, this will be converted to appropriate matrix.
 * @param ctx Rendering context
 * @param matrix 16-element column-major matrix
 */
void cgns_render_set_projection(cgns_render_context_t* ctx,
                                 const float* matrix);

/**
 * Set view matrix.
 * @param ctx Rendering context
 * @param matrix 16-element column-major matrix
 */
void cgns_render_set_view(cgns_render_context_t* ctx,
                          const float* matrix);

/**
 * Set model matrix.
 * @param ctx Rendering context
 * @param matrix 16-element column-major matrix
 */
void cgns_render_set_model(cgns_render_context_t* ctx,
                           const float* matrix);

/* ========================================================================
 * Render State Management
 * ======================================================================== */

/**
 * Enable a render state.
 * @param ctx Rendering context
 * @param state State to enable
 */
void cgns_render_enable(cgns_render_context_t* ctx, cgns_render_state_t state);

/**
 * Disable a render state.
 * @param ctx Rendering context
 * @param state State to disable
 */
void cgns_render_disable(cgns_render_context_t* ctx, cgns_render_state_t state);

/**
 * Set shading model.
 * @param ctx Rendering context
 * @param model Shading model (flat or smooth)
 */
void cgns_render_set_shade_model(cgns_render_context_t* ctx,
                                  cgns_shade_model_t model);

/**
 * Set polygon rendering mode.
 * @param ctx Rendering context
 * @param mode Polygon mode (fill, line, or point)
 */
void cgns_render_set_polygon_mode(cgns_render_context_t* ctx,
                                   cgns_polygon_mode_t mode);

/**
 * Set current drawing color.
 * @param ctx Rendering context
 * @param r Red component (0.0 - 1.0)
 * @param g Green component (0.0 - 1.0)
 * @param b Blue component (0.0 - 1.0)
 */
void cgns_render_set_color3f(cgns_render_context_t* ctx,
                             float r, float g, float b);

/**
 * Set current drawing color with alpha.
 * @param ctx Rendering context
 * @param r Red component (0.0 - 1.0)
 * @param g Green component (0.0 - 1.0)
 * @param b Blue component (0.0 - 1.0)
 * @param a Alpha component (0.0 - 1.0)
 */
void cgns_render_set_color4f(cgns_render_context_t* ctx,
                             float r, float g, float b, float a);

/**
 * Set material properties.
 * @param ctx Rendering context
 * @param material Material properties
 */
void cgns_render_set_material(cgns_render_context_t* ctx,
                              const cgns_material_t* material);

/* ========================================================================
 * Immediate-Mode Style Rendering
 * These functions provide compatibility with legacy OpenGL immediate mode.
 * ======================================================================== */

/**
 * Begin drawing primitives.
 * @param ctx Rendering context
 * @param type Primitive type
 */
void cgns_render_begin(cgns_render_context_t* ctx, cgns_primitive_type_t type);

/**
 * Submit a vertex.
 * @param ctx Rendering context
 * @param v 3-component vertex position
 */
void cgns_render_vertex3fv(cgns_render_context_t* ctx, const float* v);

/**
 * Submit a vertex (individual components).
 * @param ctx Rendering context
 * @param x X coordinate
 * @param y Y coordinate
 * @param z Z coordinate
 */
void cgns_render_vertex3f(cgns_render_context_t* ctx, float x, float y, float z);

/**
 * Set normal for subsequent vertices.
 * @param ctx Rendering context
 * @param n 3-component normal vector
 */
void cgns_render_normal3fv(cgns_render_context_t* ctx, const float* n);

/**
 * Set normal for subsequent vertices (individual components).
 * @param ctx Rendering context
 * @param nx X component
 * @param ny Y component
 * @param nz Z component
 */
void cgns_render_normal3f(cgns_render_context_t* ctx, float nx, float ny, float nz);

/**
 * End drawing primitives and submit to GPU.
 * @param ctx Rendering context
 */
void cgns_render_end(cgns_render_context_t* ctx);

/* ========================================================================
 * Batch Rendering (More Efficient)
 * For large meshes, batch submission is more efficient than immediate mode.
 * ======================================================================== */

/**
 * Draw a batch of vertices.
 * More efficient than immediate mode for large meshes.
 *
 * @param ctx Rendering context
 * @param type Primitive type
 * @param vertices Array of vertex data
 * @param count Number of vertices
 */
void cgns_render_draw_batch(cgns_render_context_t* ctx,
                            cgns_primitive_type_t type,
                            const cgns_vertex_t* vertices,
                            size_t count);

/* ========================================================================
 * Display Lists (for OpenGL backend)
 * ======================================================================== */

/**
 * Generate a new display list ID.
 * @param ctx Rendering context
 * @return Display list ID, or 0 on error
 */
unsigned int cgns_render_gen_list(cgns_render_context_t* ctx);

/**
 * Begin recording a display list.
 * @param ctx Rendering context
 * @param list Display list ID
 */
void cgns_render_new_list(cgns_render_context_t* ctx, unsigned int list);

/**
 * End recording a display list.
 * @param ctx Rendering context
 */
void cgns_render_end_list(cgns_render_context_t* ctx);

/**
 * Execute a display list.
 * @param ctx Rendering context
 * @param list Display list ID
 */
void cgns_render_call_list(cgns_render_context_t* ctx, unsigned int list);

/**
 * Delete a display list.
 * @param ctx Rendering context
 * @param list Display list ID
 */
void cgns_render_delete_list(cgns_render_context_t* ctx, unsigned int list);

/* ========================================================================
 * Utility Functions
 * ======================================================================== */

/**
 * Get backend name as string.
 * @param backend Backend type
 * @return Human-readable backend name
 */
const char* cgns_render_backend_name(cgns_render_backend_t backend);

/**
 * Check if a backend is available on this system.
 * @param backend Backend to check
 * @return 1 if available, 0 otherwise
 */
int cgns_render_backend_available(cgns_render_backend_t backend);

/**
 * Get last error message from rendering backend.
 * @param ctx Rendering context
 * @return Error string, or NULL if no error
 */
const char* cgns_render_get_error(cgns_render_context_t* ctx);

#ifdef __cplusplus
}
#endif

#endif /* CGNS_RENDER_BACKEND_H */
