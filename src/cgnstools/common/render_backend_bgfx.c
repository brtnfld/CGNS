/**
 * render_backend_bgfx.c
 *
 * bgfx backend implementation for CGNS visualization tools.
 * Provides modern multi-platform rendering (Vulkan/Metal/DX12/OpenGL).
 *
 * PHASE 1: STUB IMPLEMENTATION
 * This file contains the foundation for bgfx integration.
 * Full implementation will be completed in Phase 2.
 *
 * To enable bgfx backend:
 * 1. Install bgfx library (https://github.com/bkaradzic/bgfx)
 * 2. Set CGNS_ENABLE_BGFX=ON in CMake
 * 3. Set backend at runtime: cgns_render_set_backend(CGNS_RENDER_BACKEND_BGFX)
 */

#include "render_backend.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef CGNS_ENABLE_BGFX
/* bgfx C99 API */
#include <bgfx/c99/bgfx.h>

/* Include compiled shaders */
#include "shaders/vs_basic_glsl.h"
#include "shaders/fs_smooth_glsl.h"
#include "shaders/fs_flat_glsl.h"
#include "shaders/fs_unlit_glsl.h"

/* Include textured shaders (Phase 3) */
#include "shaders/vs_textured_glsl.h"
#include "shaders/fs_textured_smooth_glsl.h"
#include "shaders/fs_textured_flat_glsl.h"
#include "shaders/fs_textured_unlit_glsl.h"
#endif

/* ========================================================================
 * PHASE 1 STUB IMPLEMENTATION
 *
 * This provides the function signatures and error handling for bgfx,
 * but returns "not implemented" errors. This allows:
 * - The abstraction layer to compile
 * - Applications to gracefully fall back to OpenGL
 * - Incremental development of bgfx support in Phase 2
 * ======================================================================== */

#ifndef CGNS_ENABLE_BGFX

/* When bgfx is not enabled, all functions return errors */

static const char* BGFX_NOT_AVAILABLE =
    "bgfx backend not available - rebuild with CGNS_ENABLE_BGFX=ON";

cgns_render_context_t* cgns_render_initialize(void* platform_data)
{
    (void)platform_data;
    fprintf(stderr, "%s\n", BGFX_NOT_AVAILABLE);
    return NULL;
}

void cgns_render_shutdown(cgns_render_context_t* ctx)
{
    (void)ctx;
}

int cgns_render_make_current(cgns_render_context_t* ctx)
{
    (void)ctx;
    return -1;
}

/* Additional stub functions would go here... */

#else /* CGNS_ENABLE_BGFX */

/* ========================================================================
 * Backend Selection (for bgfx build)
 * ======================================================================== */

static cgns_render_backend_t g_active_backend = CGNS_RENDER_BACKEND_BGFX;

cgns_render_backend_t cgns_render_get_backend(void)
{
    return g_active_backend;
}

int cgns_render_set_backend(cgns_render_backend_t backend)
{
    if (backend != CGNS_RENDER_BACKEND_BGFX) {
        /* For bgfx build, only bgfx is available */
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
            return 0; /* Not available in bgfx build */
        case CGNS_RENDER_BACKEND_BGFX:
            return 1; /* bgfx is available */
        default:
            return 0;
    }
}

/* ========================================================================
 * PHASE 2 IMPLEMENTATION NOTES
 *
 * When implementing bgfx support, the following structure is recommended:
 *
 * 1. Context Management:
 *    - Initialize bgfx with appropriate renderer type
 *    - Create vertex/index buffers for mesh data
 *    - Compile shaders (GLSL -> SPIR-V conversion)
 *
 * 2. Immediate Mode Emulation:
 *    - Buffer vertices during cgns_render_begin/end
 *    - Submit batch when cgns_render_end is called
 *    - Use dynamic vertex buffers for efficiency
 *
 * 3. Display List Emulation:
 *    - Store recorded commands in memory
 *    - Replay on cgns_render_call_list
 *    - Use transient buffers for small lists
 *
 * 4. Shader System:
 *    - Basic vertex/fragment shaders for:
 *      * Flat shading (per-face normals)
 *      * Smooth shading (per-vertex normals)
 *      * Wireframe rendering
 *      * Unlit rendering (lines, outlines)
 *
 * 5. State Management:
 *    - Track OpenGL-style state (matrices, colors, materials)
 *    - Convert to bgfx uniforms before submission
 *    - Minimize state changes for performance
 * ======================================================================== */

/**
 * Display list structure for recorded rendering commands
 */
typedef struct {
    unsigned int id;
    cgns_vertex_t* vertices;
    size_t vertex_count;
    cgns_primitive_type_t primitive_type;
    cgns_material_t material;
    int lighting_enabled;
    cgns_shade_model_t shade_model;

    /* Phase 3: Texture state */
    unsigned int texture_id;        /* Bound texture at record time */
    int texture_enabled;            /* Was texture enabled when recorded? */
    int texture_blend_mode;         /* Blend mode (0=modulate, 1=replace, 2=decal) */
} display_list_t;

/**
 * bgfx-specific context state
 */
typedef struct {
    /* Platform data */
    void* platform_data;

    /* bgfx state */
    bgfx_vertex_layout_t vertex_layout;
    bgfx_vertex_layout_t vertex_layout_textured; /* Phase 3: with texture coords */
    bgfx_program_handle_t program_smooth;  /* Smooth shading program */
    bgfx_program_handle_t program_flat;    /* Flat shading program */
    bgfx_program_handle_t program_unlit;   /* Unlit program */
    bgfx_program_handle_t program_textured_smooth;  /* Phase 3: Textured smooth */
    bgfx_program_handle_t program_textured_flat;    /* Phase 3: Textured flat */
    bgfx_program_handle_t program_textured_unlit;   /* Phase 3: Textured unlit */
    bgfx_program_handle_t current_program; /* Currently active program */

    /* Uniform handles */
    bgfx_uniform_handle_t u_modelViewProj;
    bgfx_uniform_handle_t u_model;
    bgfx_uniform_handle_t u_lightDir;
    bgfx_uniform_handle_t u_ambientLight;
    bgfx_uniform_handle_t u_diffuseLight;
    bgfx_uniform_handle_t u_specularLight;
    bgfx_uniform_handle_t u_materialAmbient;
    bgfx_uniform_handle_t u_materialDiffuse;
    bgfx_uniform_handle_t u_materialSpecular;
    bgfx_uniform_handle_t u_materialShininess;
    bgfx_uniform_handle_t u_enableLighting;
    bgfx_uniform_handle_t u_cameraPos;
    bgfx_uniform_handle_t u_enableTexture;  /* Phase 3: texture enable/blend mode */
    bgfx_uniform_handle_t s_texture;        /* Phase 3: texture sampler */

    /* Immediate mode emulation */
    cgns_vertex_t* vertex_buffer;
    size_t vertex_count;
    size_t vertex_capacity;
    cgns_primitive_type_t current_primitive;

    /* Current state (for uniforms) */
    float projection_matrix[16];
    float view_matrix[16];
    float model_matrix[16];
    float current_normal[3];
    float current_color[4];
    float current_texcoord[2];  /* Phase 3: current texture coordinates */

    /* Material state */
    cgns_material_t current_material;

    /* Render state */
    int lighting_enabled;
    int depth_test_enabled;
    int blend_enabled;
    cgns_shade_model_t shade_model;
    cgns_polygon_mode_t polygon_mode;

    /* Texture state (Phase 3) */
    bgfx_texture_handle_t bound_textures[8];  /* Up to 8 texture units */
    int texture_enabled;
    int texture_blend_mode;  /* 0=modulate, 1=replace, 2=decal */

    /* Display list emulation */
    display_list_t* display_lists;
    size_t display_list_count;
    size_t display_list_capacity;
    unsigned int recording_list_id;
    int recording;

    /* Error handling */
    char error_msg[256];
} bgfx_context_t;

/* ========================================================================
 * Helper Functions (Phase 2)
 * ======================================================================== */

/**
 * Create shader from embedded bytecode
 */
static bgfx_shader_handle_t create_shader(const uint8_t* data, uint32_t size)
{
    const bgfx_memory_t* mem = bgfx_copy(data, size);
    return bgfx_create_shader(mem);
}

/**
 * Create shader program from vertex and fragment shaders
 */
static bgfx_program_handle_t create_program(bgfx_shader_handle_t vsh,
                                             bgfx_shader_handle_t fsh)
{
    return bgfx_create_program(vsh, fsh, true); /* true = destroy shaders when program is destroyed */
}

/**
 * Initialize vertex layout for our vertex format (without texture coordinates)
 */
static void init_vertex_layout(bgfx_vertex_layout_t* layout)
{
    bgfx_vertex_layout_begin(layout, BGFX_RENDERER_TYPE_NOOP);

    /* Position: 3 floats */
    bgfx_vertex_layout_add(layout,
                          BGFX_ATTRIB_POSITION,
                          3,
                          BGFX_ATTRIB_TYPE_FLOAT,
                          false,
                          false);

    /* Normal: 3 floats */
    bgfx_vertex_layout_add(layout,
                          BGFX_ATTRIB_NORMAL,
                          3,
                          BGFX_ATTRIB_TYPE_FLOAT,
                          false,
                          false);

    /* Color: 4 floats */
    bgfx_vertex_layout_add(layout,
                          BGFX_ATTRIB_COLOR0,
                          4,
                          BGFX_ATTRIB_TYPE_FLOAT,
                          false,
                          false);

    bgfx_vertex_layout_end(layout);
}

/**
 * Initialize vertex layout with texture coordinates (Phase 3)
 */
static void init_vertex_layout_textured(bgfx_vertex_layout_t* layout)
{
    bgfx_vertex_layout_begin(layout, BGFX_RENDERER_TYPE_NOOP);

    /* Position: 3 floats */
    bgfx_vertex_layout_add(layout,
                          BGFX_ATTRIB_POSITION,
                          3,
                          BGFX_ATTRIB_TYPE_FLOAT,
                          false,
                          false);

    /* Normal: 3 floats */
    bgfx_vertex_layout_add(layout,
                          BGFX_ATTRIB_NORMAL,
                          3,
                          BGFX_ATTRIB_TYPE_FLOAT,
                          false,
                          false);

    /* Color: 4 floats */
    bgfx_vertex_layout_add(layout,
                          BGFX_ATTRIB_COLOR0,
                          4,
                          BGFX_ATTRIB_TYPE_FLOAT,
                          false,
                          false);

    /* Texture coordinates: 2 floats (Phase 3) */
    bgfx_vertex_layout_add(layout,
                          BGFX_ATTRIB_TEXCOORD0,
                          2,
                          BGFX_ATTRIB_TYPE_FLOAT,
                          false,
                          false);

    bgfx_vertex_layout_end(layout);
}

/**
 * Triangulate quads to triangles
 * Converts 4 vertices (quad) into 6 vertices (2 triangles)
 * Returns new vertex count
 */
static size_t triangulate_quads(cgns_vertex_t* buffer, size_t vertex_count)
{
    if (vertex_count % 4 != 0) {
        fprintf(stderr, "Warning: Quad count is not divisible by 4\n");
        return vertex_count;
    }

    size_t quad_count = vertex_count / 4;
    size_t new_count = quad_count * 6; /* 2 triangles per quad */

    /* Process quads from back to front to avoid overwriting */
    for (int q = (int)quad_count - 1; q >= 0; q--) {
        cgns_vertex_t* quad = &buffer[q * 4];
        cgns_vertex_t* tri = &buffer[q * 6];

        /* First triangle: v0, v1, v2 */
        tri[0] = quad[0];
        tri[1] = quad[1];
        tri[2] = quad[2];

        /* Second triangle: v0, v2, v3 */
        tri[3] = quad[0];
        tri[4] = quad[2];
        tri[5] = quad[3];
    }

    return new_count;
}

/**
 * Triangulate polygon (n-sided) to triangles using fan triangulation
 * Returns new vertex count
 */
static size_t triangulate_polygon(cgns_vertex_t* buffer, size_t vertex_count)
{
    if (vertex_count < 3) {
        return vertex_count; /* Can't triangulate */
    }

    /* Fan triangulation: creates (n-2) triangles from n vertices */
    size_t triangle_count = vertex_count - 2;
    size_t new_count = triangle_count * 3;

    /* Store first vertex */
    cgns_vertex_t v0 = buffer[0];

    /* Create triangles: (v0, vi, vi+1) for i=1..n-2 */
    for (size_t i = 0; i < triangle_count; i++) {
        buffer[i * 3 + 0] = v0;
        buffer[i * 3 + 1] = buffer[i + 1];
        buffer[i * 3 + 2] = buffer[i + 2];
    }

    return new_count;
}

/**
 * Allocate and initialize bgfx context
 */
static bgfx_context_t* alloc_bgfx_context(void)
{
    bgfx_context_t* ctx = (bgfx_context_t*)malloc(sizeof(bgfx_context_t));
    if (!ctx) return NULL;

    memset(ctx, 0, sizeof(bgfx_context_t));

    /* Initialize vertex buffer for immediate mode */
    ctx->vertex_capacity = 4096; /* Start with space for 4K vertices */
    ctx->vertex_buffer = (cgns_vertex_t*)malloc(
        ctx->vertex_capacity * sizeof(cgns_vertex_t));

    if (!ctx->vertex_buffer) {
        free(ctx);
        return NULL;
    }

    /* Initialize default state */
    ctx->current_color[0] = 1.0f;
    ctx->current_color[1] = 1.0f;
    ctx->current_color[2] = 1.0f;
    ctx->current_color[3] = 1.0f;

    ctx->current_normal[0] = 0.0f;
    ctx->current_normal[1] = 0.0f;
    ctx->current_normal[2] = 1.0f;

    ctx->current_texcoord[0] = 0.0f;  /* Phase 3 */
    ctx->current_texcoord[1] = 0.0f;  /* Phase 3 */

    /* Initialize matrices to identity */
    for (int i = 0; i < 16; i++) {
        ctx->projection_matrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        ctx->view_matrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        ctx->model_matrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }

    /* Initialize default material */
    ctx->current_material.ambient[0] = 0.2f;
    ctx->current_material.ambient[1] = 0.2f;
    ctx->current_material.ambient[2] = 0.2f;
    ctx->current_material.ambient[3] = 1.0f;

    ctx->current_material.diffuse[0] = 0.8f;
    ctx->current_material.diffuse[1] = 0.8f;
    ctx->current_material.diffuse[2] = 0.8f;
    ctx->current_material.diffuse[3] = 1.0f;

    ctx->current_material.specular[0] = 0.0f;
    ctx->current_material.specular[1] = 0.0f;
    ctx->current_material.specular[2] = 0.0f;
    ctx->current_material.specular[3] = 1.0f;

    ctx->current_material.shininess = 0.0f;

    /* Enable depth test by default */
    ctx->depth_test_enabled = 1;
    ctx->lighting_enabled = 0;
    ctx->blend_enabled = 0;
    ctx->shade_model = CGNS_SHADE_SMOOTH;
    ctx->polygon_mode = CGNS_POLY_FILL;

    /* Initialize display list storage */
    ctx->display_list_capacity = 16;
    ctx->display_lists = (display_list_t*)calloc(ctx->display_list_capacity, sizeof(display_list_t));
    ctx->display_list_count = 0;
    ctx->recording = 0;
    ctx->recording_list_id = 0;

    return ctx;
}

/* ========================================================================
 * Public API Implementation (Phase 2 Template)
 * ======================================================================== */

cgns_render_context_t* cgns_render_initialize(void* platform_data)
{
    bgfx_context_t* ctx;
    bgfx_shader_handle_t vsh, fsh_smooth, fsh_flat, fsh_unlit;
    bgfx_shader_handle_t vsh_tex, fsh_tex_smooth, fsh_tex_flat, fsh_tex_unlit;

    ctx = alloc_bgfx_context();
    if (!ctx) {
        fprintf(stderr, "Failed to allocate bgfx context\n");
        return NULL;
    }

    ctx->platform_data = platform_data;

    /* Initialize bgfx library */
    bgfx_init_t init;
    bgfx_init_ctor(&init);

    /* Set platform data if provided (for GPU rendering to window) */
    if (platform_data != NULL) {
        cgns_platform_data_t* pd = (cgns_platform_data_t*)platform_data;

        #if !defined(__WIN32__) && !defined(_WIN32)
        /* X11/Linux: Set native display and window handle */
        bgfx_platform_data_t bgfx_pd;
        memset(&bgfx_pd, 0, sizeof(bgfx_pd));
        bgfx_pd.ndt = pd->display;
        bgfx_pd.nwh = pd->window;
        bgfx_set_platform_data(&bgfx_pd);
        #elif defined(__WIN32__) || defined(_WIN32)
        /* Win32: Set window handle */
        bgfx_platform_data_t bgfx_pd;
        memset(&bgfx_pd, 0, sizeof(bgfx_pd));
        bgfx_pd.nwh = pd->window;
        bgfx_set_platform_data(&bgfx_pd);
        #elif defined(__APPLE__)
        /* macOS: Set native window handle */
        bgfx_platform_data_t bgfx_pd;
        memset(&bgfx_pd, 0, sizeof(bgfx_pd));
        bgfx_pd.nwh = pd->window;
        bgfx_set_platform_data(&bgfx_pd);
        #endif

        init.type = BGFX_RENDERER_TYPE_COUNT; /* Auto-select best renderer (OpenGL/Vulkan/etc.) */
        printf("bgfx GPU mode: rendering to window\n");
    } else {
        /* For headless/test mode (NULL platform_data), use NOOP renderer */
        init.type = BGFX_RENDERER_TYPE_NOOP;
        printf("bgfx headless mode: using NOOP renderer\n");
    }

    init.resolution.width = 1280;
    init.resolution.height = 720;
    init.resolution.reset = BGFX_RESET_VSYNC;

    if (!bgfx_init(&init)) {
        free(ctx->vertex_buffer);
        free(ctx);
        fprintf(stderr, "Failed to initialize bgfx\n");
        return NULL;
    }

    printf("bgfx initialized - Renderer: %s\n",
           bgfx_get_renderer_name(bgfx_get_renderer_type()));

    /* Initialize vertex layouts */
    init_vertex_layout(&ctx->vertex_layout);
    init_vertex_layout_textured(&ctx->vertex_layout_textured);

    /* Create non-textured shaders */
    vsh = create_shader(vs_basic_glsl, sizeof(vs_basic_glsl));
    fsh_smooth = create_shader(fs_smooth_glsl, sizeof(fs_smooth_glsl));
    fsh_flat = create_shader(fs_flat_glsl, sizeof(fs_flat_glsl));
    fsh_unlit = create_shader(fs_unlit_glsl, sizeof(fs_unlit_glsl));

    /* Create non-textured shader programs */
    ctx->program_smooth = create_program(vsh, fsh_smooth);
    ctx->program_flat = create_program(vsh, fsh_flat);
    ctx->program_unlit = create_program(vsh, fsh_unlit);

    /* Create textured shaders (Phase 3) */
    vsh_tex = create_shader(vs_textured_glsl, sizeof(vs_textured_glsl));
    fsh_tex_smooth = create_shader(fs_textured_smooth_glsl, sizeof(fs_textured_smooth_glsl));
    fsh_tex_flat = create_shader(fs_textured_flat_glsl, sizeof(fs_textured_flat_glsl));
    fsh_tex_unlit = create_shader(fs_textured_unlit_glsl, sizeof(fs_textured_unlit_glsl));

    /* Create textured shader programs (Phase 3) */
    ctx->program_textured_smooth = create_program(vsh_tex, fsh_tex_smooth);
    ctx->program_textured_flat = create_program(vsh_tex, fsh_tex_flat);
    ctx->program_textured_unlit = create_program(vsh_tex, fsh_tex_unlit);

    ctx->current_program = ctx->program_smooth; /* Default to smooth shading */

    /* Create uniform handles */
    ctx->u_lightDir = bgfx_create_uniform("u_lightDir", BGFX_UNIFORM_TYPE_VEC4, 1);
    ctx->u_ambientLight = bgfx_create_uniform("u_ambientLight", BGFX_UNIFORM_TYPE_VEC4, 1);
    ctx->u_diffuseLight = bgfx_create_uniform("u_diffuseLight", BGFX_UNIFORM_TYPE_VEC4, 1);
    ctx->u_specularLight = bgfx_create_uniform("u_specularLight", BGFX_UNIFORM_TYPE_VEC4, 1);
    ctx->u_materialAmbient = bgfx_create_uniform("u_materialAmbient", BGFX_UNIFORM_TYPE_VEC4, 1);
    ctx->u_materialDiffuse = bgfx_create_uniform("u_materialDiffuse", BGFX_UNIFORM_TYPE_VEC4, 1);
    ctx->u_materialSpecular = bgfx_create_uniform("u_materialSpecular", BGFX_UNIFORM_TYPE_VEC4, 1);
    ctx->u_materialShininess = bgfx_create_uniform("u_materialShininess", BGFX_UNIFORM_TYPE_VEC4, 1);
    ctx->u_enableLighting = bgfx_create_uniform("u_enableLighting", BGFX_UNIFORM_TYPE_VEC4, 1);
    ctx->u_cameraPos = bgfx_create_uniform("u_cameraPos", BGFX_UNIFORM_TYPE_VEC4, 1);

    /* Create texture uniforms (Phase 3) */
    ctx->u_enableTexture = bgfx_create_uniform("u_enableTexture", BGFX_UNIFORM_TYPE_VEC4, 1);
    ctx->s_texture = bgfx_create_uniform("s_texture", BGFX_UNIFORM_TYPE_SAMPLER, 1);

    /* Initialize texture state */
    for (int i = 0; i < 8; i++) {
        ctx->bound_textures[i].idx = UINT16_MAX;  /* Invalid handle */
    }
    ctx->texture_enabled = 0;
    ctx->texture_blend_mode = 0;  /* Default to modulate */
    ctx->current_texcoord[0] = 0.0f;
    ctx->current_texcoord[1] = 0.0f;

    /* Set default render state */
    bgfx_set_view_clear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
    bgfx_set_view_rect(0, 0, 0, 1280, 720);

    printf("bgfx backend fully initialized (Phase 3: texture support enabled)\n");

    return (cgns_render_context_t*)ctx;
}

void cgns_render_shutdown(cgns_render_context_t* ctx)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;

    if (!bgfx_ctx) return;

    /* Destroy uniforms */
    bgfx_destroy_uniform(bgfx_ctx->u_lightDir);
    bgfx_destroy_uniform(bgfx_ctx->u_ambientLight);
    bgfx_destroy_uniform(bgfx_ctx->u_diffuseLight);
    bgfx_destroy_uniform(bgfx_ctx->u_specularLight);
    bgfx_destroy_uniform(bgfx_ctx->u_materialAmbient);
    bgfx_destroy_uniform(bgfx_ctx->u_materialDiffuse);
    bgfx_destroy_uniform(bgfx_ctx->u_materialSpecular);
    bgfx_destroy_uniform(bgfx_ctx->u_materialShininess);
    bgfx_destroy_uniform(bgfx_ctx->u_enableLighting);
    bgfx_destroy_uniform(bgfx_ctx->u_cameraPos);

    /* Destroy texture uniforms (Phase 3) */
    bgfx_destroy_uniform(bgfx_ctx->u_enableTexture);
    bgfx_destroy_uniform(bgfx_ctx->s_texture);

    /* Destroy programs (shaders are automatically destroyed) */
    bgfx_destroy_program(bgfx_ctx->program_smooth);
    bgfx_destroy_program(bgfx_ctx->program_flat);
    bgfx_destroy_program(bgfx_ctx->program_unlit);

    /* Destroy textured programs (Phase 3) */
    bgfx_destroy_program(bgfx_ctx->program_textured_smooth);
    bgfx_destroy_program(bgfx_ctx->program_textured_flat);
    bgfx_destroy_program(bgfx_ctx->program_textured_unlit);

    /* Shutdown bgfx */
    bgfx_shutdown();

    /* Free CPU-side buffers */
    if (bgfx_ctx->vertex_buffer) {
        free(bgfx_ctx->vertex_buffer);
    }

    /* Free display lists */
    if (bgfx_ctx->display_lists) {
        for (size_t i = 0; i < bgfx_ctx->display_list_count; i++) {
            if (bgfx_ctx->display_lists[i].vertices) {
                free(bgfx_ctx->display_lists[i].vertices);
            }
        }
        free(bgfx_ctx->display_lists);
    }

    free(bgfx_ctx);

    printf("bgfx backend shutdown complete\n");
}

void cgns_render_begin_frame(cgns_render_context_t* ctx)
{
    (void)ctx;
    /* bgfx automatically handles frame synchronization */
    /* Just need to ensure touch view 0 to preserve framebuffer */
    bgfx_touch(0);
}

void cgns_render_end_frame(cgns_render_context_t* ctx)
{
    (void)ctx;
    /* Advance to next frame. Rendering thread will be kicked to process submitted rendering primitives */
    bgfx_frame(false);
}

/* ========================================================================
 * Immediate Mode Emulation (Phase 2 Template)
 * ======================================================================== */

void cgns_render_begin(cgns_render_context_t* ctx,
                            cgns_primitive_type_t type)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx) return;

    bgfx_ctx->current_primitive = type;
    bgfx_ctx->vertex_count = 0;
}

void cgns_render_vertex3fv(cgns_render_context_t* ctx, const float* v)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx || !v) return;

    /* Grow buffer if needed */
    if (bgfx_ctx->vertex_count >= bgfx_ctx->vertex_capacity) {
        size_t new_capacity = bgfx_ctx->vertex_capacity * 2;
        cgns_vertex_t* new_buffer = (cgns_vertex_t*)realloc(
            bgfx_ctx->vertex_buffer,
            new_capacity * sizeof(cgns_vertex_t));

        if (!new_buffer) {
            fprintf(stderr, "Failed to grow vertex buffer\n");
            return;
        }

        bgfx_ctx->vertex_buffer = new_buffer;
        bgfx_ctx->vertex_capacity = new_capacity;
    }

    /* Add vertex to buffer */
    cgns_vertex_t* vtx = &bgfx_ctx->vertex_buffer[bgfx_ctx->vertex_count++];
    vtx->position[0] = v[0];
    vtx->position[1] = v[1];
    vtx->position[2] = v[2];

    memcpy(vtx->normal, bgfx_ctx->current_normal, sizeof(float) * 3);
    memcpy(vtx->color, bgfx_ctx->current_color, sizeof(float) * 4);
    memcpy(vtx->texcoord, bgfx_ctx->current_texcoord, sizeof(float) * 2); /* Phase 3 */
}

void cgns_render_end(cgns_render_context_t* ctx)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    bgfx_transient_vertex_buffer_t tvb;
    uint64_t state;
    float mvp_matrix[16];
    float lighting_enable[4];
    size_t actual_vertex_count;
    bgfx_vertex_layout_t* layout;
    bgfx_program_handle_t program;

    if (!bgfx_ctx || bgfx_ctx->vertex_count == 0) return;

    /* Triangulate quads/polygons if needed */
    actual_vertex_count = bgfx_ctx->vertex_count;
    switch (bgfx_ctx->current_primitive) {
        case CGNS_PRIM_QUADS:
            actual_vertex_count = triangulate_quads(bgfx_ctx->vertex_buffer, actual_vertex_count);
            break;
        case CGNS_PRIM_POLYGON:
            actual_vertex_count = triangulate_polygon(bgfx_ctx->vertex_buffer, actual_vertex_count);
            break;
        default:
            /* No triangulation needed */
            break;
    }

    /* Choose vertex layout and shader program based on texture state (Phase 3) */
    if (bgfx_ctx->texture_enabled) {
        layout = &bgfx_ctx->vertex_layout_textured;
        /* Select textured program based on shade model */
        if (bgfx_ctx->shade_model == CGNS_SHADE_SMOOTH && bgfx_ctx->lighting_enabled) {
            program = bgfx_ctx->program_textured_smooth;
        } else if (bgfx_ctx->shade_model == CGNS_SHADE_FLAT && bgfx_ctx->lighting_enabled) {
            program = bgfx_ctx->program_textured_flat;
        } else {
            program = bgfx_ctx->program_textured_unlit;
        }
    } else {
        layout = &bgfx_ctx->vertex_layout;
        /* Select non-textured program (original behavior) */
        if (bgfx_ctx->shade_model == CGNS_SHADE_SMOOTH && bgfx_ctx->lighting_enabled) {
            program = bgfx_ctx->program_smooth;
        } else if (bgfx_ctx->shade_model == CGNS_SHADE_FLAT && bgfx_ctx->lighting_enabled) {
            program = bgfx_ctx->program_flat;
        } else {
            program = bgfx_ctx->program_unlit;
        }
    }

    /* Check if transient buffer is available */
    uint32_t avail = bgfx_get_avail_transient_vertex_buffer(actual_vertex_count, layout);
    if (avail < actual_vertex_count) {
        fprintf(stderr, "Not enough transient vertex buffer space (need %zu, have %u)\n",
                actual_vertex_count, avail);
        bgfx_ctx->vertex_count = 0;
        return;
    }

    /* Allocate transient vertex buffer */
    bgfx_alloc_transient_vertex_buffer(&tvb, actual_vertex_count, layout);

    /* Copy vertices to transient buffer */
    memcpy(tvb.data, bgfx_ctx->vertex_buffer,
           actual_vertex_count * sizeof(cgns_vertex_t));

    /* Set vertex buffer */
    bgfx_set_transient_vertex_buffer(0, &tvb, 0, actual_vertex_count);

    /* Compute model-view-projection matrix */
    /* TODO: For now, use identity matrix if matrices are not set */
    /* In real usage, we'd multiply: projection * view * model */
    /* For simplicity, pass identity for now - will be implemented properly later */
    for (int i = 0; i < 16; i++) {
        mvp_matrix[i] = (i % 5 == 0) ? 1.0f : 0.0f; /* Identity matrix */
    }

    /* Set matrix uniforms (bgfx built-ins u_model and u_modelViewProj are set automatically) */
    /* We'll use bgfx's built-in matrix uniforms via bgfx_set_transform */
    bgfx_set_transform(bgfx_ctx->model_matrix, 1);

    /* Set lighting uniforms */
    lighting_enable[0] = bgfx_ctx->lighting_enabled ? 1.0f : 0.0f;
    lighting_enable[1] = 0.0f;
    lighting_enable[2] = 0.0f;
    lighting_enable[3] = 0.0f;
    bgfx_set_uniform(bgfx_ctx->u_enableLighting, lighting_enable, 1);

    /* Set material uniforms */
    if (bgfx_ctx->lighting_enabled) {
        bgfx_set_uniform(bgfx_ctx->u_materialAmbient, bgfx_ctx->current_material.ambient, 1);
        bgfx_set_uniform(bgfx_ctx->u_materialDiffuse, bgfx_ctx->current_material.diffuse, 1);
        bgfx_set_uniform(bgfx_ctx->u_materialSpecular, bgfx_ctx->current_material.specular, 1);

        float shininess[4] = {bgfx_ctx->current_material.shininess, 0.0f, 0.0f, 0.0f};
        bgfx_set_uniform(bgfx_ctx->u_materialShininess, shininess, 1);

        /* Set default light direction (top-down) */
        float light_dir[4] = {0.0f, -1.0f, 0.0f, 0.0f};
        bgfx_set_uniform(bgfx_ctx->u_lightDir, light_dir, 1);

        /* Set default ambient light */
        float ambient[4] = {0.2f, 0.2f, 0.2f, 1.0f};
        bgfx_set_uniform(bgfx_ctx->u_ambientLight, ambient, 1);

        /* Set default diffuse light */
        float diffuse[4] = {0.8f, 0.8f, 0.8f, 1.0f};
        bgfx_set_uniform(bgfx_ctx->u_diffuseLight, diffuse, 1);

        /* Set default specular light */
        float specular[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        bgfx_set_uniform(bgfx_ctx->u_specularLight, specular, 1);

        /* Set camera position for specular calculations */
        float camera_pos[4] = {0.0f, 0.0f, 5.0f, 0.0f};
        bgfx_set_uniform(bgfx_ctx->u_cameraPos, camera_pos, 1);
    }

    /* Set render state */
    state = 0
        | BGFX_STATE_WRITE_RGB
        | BGFX_STATE_WRITE_A;

    /* Depth test state */
    if (bgfx_ctx->depth_test_enabled) {
        state |= BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS;
    }

    /* Blend state */
    if (bgfx_ctx->blend_enabled) {
        state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
    }

    /* Culling */
    state |= BGFX_STATE_CULL_CW;

    /* Handle different primitive types */
    switch (bgfx_ctx->current_primitive) {
        case CGNS_PRIM_LINES:
            state |= BGFX_STATE_PT_LINES;
            break;
        case CGNS_PRIM_POINTS:
            state |= BGFX_STATE_PT_POINTS;
            break;
        case CGNS_PRIM_TRIANGLES:
        case CGNS_PRIM_QUADS:
        case CGNS_PRIM_POLYGON:
        default:
            state |= BGFX_STATE_PT_TRISTRIP; /* Default to triangles */
            break;
    }

    bgfx_set_state(state, 0);

    /* Set texture uniforms if texture is enabled (Phase 3) */
    if (bgfx_ctx->texture_enabled) {
        float tex_enable[4] = {
            1.0f,  /* Enable texture */
            (float)bgfx_ctx->texture_blend_mode,  /* Blend mode */
            0.0f,
            0.0f
        };
        bgfx_set_uniform(bgfx_ctx->u_enableTexture, tex_enable, 1);

        /* Bind texture to sampler */
        if (bgfx_ctx->bound_textures[0].idx != UINT16_MAX) {
            bgfx_set_texture(0, bgfx_ctx->s_texture, bgfx_ctx->bound_textures[0], UINT32_MAX);
        }
    } else {
        /* Disable texture */
        float tex_enable[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        bgfx_set_uniform(bgfx_ctx->u_enableTexture, tex_enable, 1);
    }

    /* Submit draw call with selected program (Phase 3: may be textured) */
    bgfx_submit(0, program, 0, BGFX_DISCARD_ALL);

    bgfx_ctx->vertex_count = 0;
}

/* ========================================================================
 * State Management Functions
 * ======================================================================== */

void cgns_render_clear(cgns_render_context_t* ctx,
                            float r, float g, float b, float a)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx) return;

    /* Convert RGBA float to RGBA32 hex format */
    uint32_t rgba = 0
        | ((uint32_t)(r * 255.0f) << 24)
        | ((uint32_t)(g * 255.0f) << 16)
        | ((uint32_t)(b * 255.0f) << 8)
        | ((uint32_t)(a * 255.0f));

    bgfx_set_view_clear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, rgba, 1.0f, 0);
}

void cgns_render_set_viewport(cgns_render_context_t* ctx,
                                   const cgns_viewport_t* viewport)
{
    (void)ctx;
    if (!viewport) return;
    bgfx_set_view_rect(0, (uint16_t)viewport->x, (uint16_t)viewport->y,
                       (uint16_t)viewport->width, (uint16_t)viewport->height);
}

void cgns_render_set_projection(cgns_render_context_t* ctx,
                                     const float* matrix)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx || !matrix) return;

    memcpy(bgfx_ctx->projection_matrix, matrix, sizeof(float) * 16);
}

void cgns_render_set_view(cgns_render_context_t* ctx,
                               const float* matrix)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx || !matrix) return;

    memcpy(bgfx_ctx->view_matrix, matrix, sizeof(float) * 16);
}

void cgns_render_set_model(cgns_render_context_t* ctx,
                                const float* matrix)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx || !matrix) return;

    memcpy(bgfx_ctx->model_matrix, matrix, sizeof(float) * 16);
}

void cgns_render_normal3fv(cgns_render_context_t* ctx, const float* n)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx || !n) return;

    memcpy(bgfx_ctx->current_normal, n, sizeof(float) * 3);
}

void cgns_render_normal3f(cgns_render_context_t* ctx,
                               float nx, float ny, float nz)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx) return;

    bgfx_ctx->current_normal[0] = nx;
    bgfx_ctx->current_normal[1] = ny;
    bgfx_ctx->current_normal[2] = nz;
}

void cgns_render_set_color3f(cgns_render_context_t* ctx,
                                  float r, float g, float b)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx) return;

    bgfx_ctx->current_color[0] = r;
    bgfx_ctx->current_color[1] = g;
    bgfx_ctx->current_color[2] = b;
    bgfx_ctx->current_color[3] = 1.0f; /* Default alpha */
}

void cgns_render_set_color4f(cgns_render_context_t* ctx,
                                  float r, float g, float b, float a)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx) return;

    bgfx_ctx->current_color[0] = r;
    bgfx_ctx->current_color[1] = g;
    bgfx_ctx->current_color[2] = b;
    bgfx_ctx->current_color[3] = a;
}

void cgns_render_set_material(cgns_render_context_t* ctx,
                                   const cgns_material_t* material)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx || !material) return;

    memcpy(&bgfx_ctx->current_material, material, sizeof(cgns_material_t));
}

void cgns_render_enable(cgns_render_context_t* ctx,
                             cgns_render_state_t state)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx) return;

    switch (state) {
        case CGNS_STATE_LIGHTING:
            bgfx_ctx->lighting_enabled = 1;
            break;
        case CGNS_STATE_DEPTH_TEST:
            bgfx_ctx->depth_test_enabled = 1;
            break;
        case CGNS_STATE_BLEND:
            bgfx_ctx->blend_enabled = 1;
            break;
    }
}

void cgns_render_disable(cgns_render_context_t* ctx,
                              cgns_render_state_t state)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx) return;

    switch (state) {
        case CGNS_STATE_LIGHTING:
            bgfx_ctx->lighting_enabled = 0;
            break;
        case CGNS_STATE_DEPTH_TEST:
            bgfx_ctx->depth_test_enabled = 0;
            break;
        case CGNS_STATE_BLEND:
            bgfx_ctx->blend_enabled = 0;
            break;
    }
}

void cgns_render_set_shade_model(cgns_render_context_t* ctx,
                                      cgns_shade_model_t model)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx) return;

    bgfx_ctx->shade_model = model;

    /* Switch shader program based on shade model */
    switch (model) {
        case CGNS_SHADE_SMOOTH:
            bgfx_ctx->current_program = bgfx_ctx->program_smooth;
            break;
        case CGNS_SHADE_FLAT:
            bgfx_ctx->current_program = bgfx_ctx->program_flat;
            break;
    }
}

void cgns_render_set_polygon_mode(cgns_render_context_t* ctx,
                                       cgns_polygon_mode_t mode)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx) return;

    bgfx_ctx->polygon_mode = mode;

    /* For wireframe mode, we'll use the unlit shader */
    if (mode == CGNS_POLY_LINE) {
        bgfx_ctx->current_program = bgfx_ctx->program_unlit;
    } else {
        /* Restore based on shade model */
        cgns_render_set_shade_model(ctx, bgfx_ctx->shade_model);
    }
}

/* ========================================================================
 * Additional Immediate Mode Functions
 * ======================================================================== */

void cgns_render_vertex3f(cgns_render_context_t* ctx,
                               float x, float y, float z)
{
    float v[3] = {x, y, z};
    cgns_render_vertex3fv(ctx, v);
}

int cgns_render_make_current(cgns_render_context_t* ctx)
{
    /* bgfx doesn't require a "make current" operation */
    (void)ctx;
    return 0; /* Success */
}

/* ========================================================================
 * Batch Rendering (TODO: Full implementation in later phase)
 * ======================================================================== */

void cgns_render_draw_batch(cgns_render_context_t* ctx,
                            cgns_primitive_type_t type,
                            const cgns_vertex_t* vertices,
                            size_t count)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    bgfx_transient_vertex_buffer_t tvb;
    uint64_t state;
    size_t actual_vertex_count = count;
    cgns_vertex_t* temp_vertices = NULL;

    if (!bgfx_ctx || !vertices || count == 0) return;

    /* Triangulate if needed */
    if (type == CGNS_PRIM_QUADS || type == CGNS_PRIM_POLYGON) {
        /* Allocate temp buffer for triangulated vertices */
        size_t max_triangulated = (type == CGNS_PRIM_QUADS) ? (count / 4) * 6 : count * 3;
        temp_vertices = (cgns_vertex_t*)malloc(max_triangulated * sizeof(cgns_vertex_t));
        if (!temp_vertices) {
            fprintf(stderr, "Failed to allocate triangulation buffer\n");
            return;
        }

        /* Copy vertices for triangulation */
        memcpy(temp_vertices, vertices, count * sizeof(cgns_vertex_t));

        /* Triangulate */
        if (type == CGNS_PRIM_QUADS) {
            actual_vertex_count = triangulate_quads(temp_vertices, count);
        } else {
            actual_vertex_count = triangulate_polygon(temp_vertices, count);
        }

        vertices = temp_vertices;
    }

    /* Choose vertex layout based on texture state (Phase 3) */
    bgfx_vertex_layout_t* layout = bgfx_ctx->texture_enabled ?
                                     &bgfx_ctx->vertex_layout_textured :
                                     &bgfx_ctx->vertex_layout;

    /* Check if transient buffer is available */
    uint32_t avail = bgfx_get_avail_transient_vertex_buffer(actual_vertex_count, layout);
    if (avail < actual_vertex_count) {
        fprintf(stderr, "Not enough transient vertex buffer space for batch\n");
        free(temp_vertices);
        return;
    }

    /* Allocate transient vertex buffer */
    bgfx_alloc_transient_vertex_buffer(&tvb, actual_vertex_count, layout);

    /* Copy vertices */
    memcpy(tvb.data, vertices, actual_vertex_count * sizeof(cgns_vertex_t));

    /* Set vertex buffer */
    bgfx_set_transient_vertex_buffer(0, &tvb, 0, actual_vertex_count);

    /* Set matrix transform */
    bgfx_set_transform(bgfx_ctx->model_matrix, 1);

    /* Set lighting uniforms */
    float lighting_enable[4] = {bgfx_ctx->lighting_enabled ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f};
    bgfx_set_uniform(bgfx_ctx->u_enableLighting, lighting_enable, 1);

    if (bgfx_ctx->lighting_enabled) {
        bgfx_set_uniform(bgfx_ctx->u_materialAmbient, bgfx_ctx->current_material.ambient, 1);
        bgfx_set_uniform(bgfx_ctx->u_materialDiffuse, bgfx_ctx->current_material.diffuse, 1);
        bgfx_set_uniform(bgfx_ctx->u_materialSpecular, bgfx_ctx->current_material.specular, 1);

        float shininess[4] = {bgfx_ctx->current_material.shininess, 0.0f, 0.0f, 0.0f};
        bgfx_set_uniform(bgfx_ctx->u_materialShininess, shininess, 1);

        float light_dir[4] = {0.0f, -1.0f, 0.0f, 0.0f};
        bgfx_set_uniform(bgfx_ctx->u_lightDir, light_dir, 1);

        float ambient[4] = {0.2f, 0.2f, 0.2f, 1.0f};
        bgfx_set_uniform(bgfx_ctx->u_ambientLight, ambient, 1);

        float diffuse[4] = {0.8f, 0.8f, 0.8f, 1.0f};
        bgfx_set_uniform(bgfx_ctx->u_diffuseLight, diffuse, 1);

        float specular[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        bgfx_set_uniform(bgfx_ctx->u_specularLight, specular, 1);

        float camera_pos[4] = {0.0f, 0.0f, 5.0f, 0.0f};
        bgfx_set_uniform(bgfx_ctx->u_cameraPos, camera_pos, 1);
    }

    /* Set texture uniforms if texture is enabled (Phase 3) */
    if (bgfx_ctx->texture_enabled) {
        float tex_enable[4] = {
            1.0f,  /* enabled */
            (float)bgfx_ctx->texture_blend_mode,  /* blend mode */
            0.0f, 0.0f
        };
        bgfx_set_uniform(bgfx_ctx->u_enableTexture, tex_enable, 1);

        /* Bind texture if available */
        if (bgfx_ctx->bound_textures[0].idx != UINT16_MAX) {
            bgfx_set_texture(0, bgfx_ctx->s_texture,
                            bgfx_ctx->bound_textures[0], UINT32_MAX);
        }
    } else {
        /* Disable texture in shader */
        float tex_enable[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        bgfx_set_uniform(bgfx_ctx->u_enableTexture, tex_enable, 1);
    }

    /* Set render state */
    state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A;

    if (bgfx_ctx->depth_test_enabled) {
        state |= BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS;
    }

    if (bgfx_ctx->blend_enabled) {
        state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
    }

    state |= BGFX_STATE_CULL_CW;

    /* Handle primitive types */
    switch (type) {
        case CGNS_PRIM_LINES:
            state |= BGFX_STATE_PT_LINES;
            break;
        case CGNS_PRIM_POINTS:
            state |= BGFX_STATE_PT_POINTS;
            break;
        default:
            state |= BGFX_STATE_PT_TRISTRIP;
            break;
    }

    bgfx_set_state(state, 0);

    /* Select shader program based on texture and lighting state (Phase 3) */
    bgfx_program_handle_t program;
    if (bgfx_ctx->texture_enabled) {
        /* Use textured shader programs */
        if (bgfx_ctx->shade_model == CGNS_SHADE_SMOOTH && bgfx_ctx->lighting_enabled) {
            program = bgfx_ctx->program_textured_smooth;
        } else if (bgfx_ctx->shade_model == CGNS_SHADE_FLAT && bgfx_ctx->lighting_enabled) {
            program = bgfx_ctx->program_textured_flat;
        } else {
            program = bgfx_ctx->program_textured_unlit;
        }
    } else {
        /* Use non-textured shader programs */
        if (bgfx_ctx->shade_model == CGNS_SHADE_SMOOTH && bgfx_ctx->lighting_enabled) {
            program = bgfx_ctx->program_smooth;
        } else if (bgfx_ctx->shade_model == CGNS_SHADE_FLAT && bgfx_ctx->lighting_enabled) {
            program = bgfx_ctx->program_flat;
        } else {
            program = bgfx_ctx->program_unlit;
        }
    }

    /* Submit draw call */
    bgfx_submit(0, program, 0, BGFX_DISCARD_ALL);

    /* Cleanup */
    free(temp_vertices);
}

/* ========================================================================
 * Display Lists
 * ======================================================================== */

unsigned int cgns_render_gen_list(cgns_render_context_t* ctx)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    static unsigned int next_list_id = 1;

    if (!bgfx_ctx) return 0;

    /* Generate unique display list ID */
    return next_list_id++;
}

void cgns_render_new_list(cgns_render_context_t* ctx, unsigned int list)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    size_t i;

    if (!bgfx_ctx) return;

    /* Find existing display list or create new one */
    display_list_t* dl = NULL;
    for (i = 0; i < bgfx_ctx->display_list_count; i++) {
        if (bgfx_ctx->display_lists[i].id == list) {
            dl = &bgfx_ctx->display_lists[i];
            /* Free existing vertices if any */
            if (dl->vertices) {
                free(dl->vertices);
                dl->vertices = NULL;
            }
            dl->vertex_count = 0;
            break;
        }
    }

    /* If not found, create new display list */
    if (!dl) {
        /* Grow array if needed */
        if (bgfx_ctx->display_list_count >= bgfx_ctx->display_list_capacity) {
            bgfx_ctx->display_list_capacity *= 2;
            display_list_t* new_lists = (display_list_t*)realloc(
                bgfx_ctx->display_lists,
                bgfx_ctx->display_list_capacity * sizeof(display_list_t)
            );
            if (!new_lists) {
                fprintf(stderr, "Failed to grow display list array\n");
                return;
            }
            bgfx_ctx->display_lists = new_lists;
        }

        /* Initialize new display list */
        dl = &bgfx_ctx->display_lists[bgfx_ctx->display_list_count];
        memset(dl, 0, sizeof(display_list_t));
        dl->id = list;
        bgfx_ctx->display_list_count++;
    }

    /* Start recording */
    bgfx_ctx->recording = 1;
    bgfx_ctx->recording_list_id = list;

    /* Clear vertex buffer to start fresh recording */
    bgfx_ctx->vertex_count = 0;
}

void cgns_render_end_list(cgns_render_context_t* ctx)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    size_t i;

    if (!bgfx_ctx || !bgfx_ctx->recording) return;

    /* Find the display list we're recording to */
    display_list_t* dl = NULL;
    for (i = 0; i < bgfx_ctx->display_list_count; i++) {
        if (bgfx_ctx->display_lists[i].id == bgfx_ctx->recording_list_id) {
            dl = &bgfx_ctx->display_lists[i];
            break;
        }
    }

    if (!dl) {
        fprintf(stderr, "Display list %u not found during end_list\n",
                bgfx_ctx->recording_list_id);
        bgfx_ctx->recording = 0;
        return;
    }

    /* Copy recorded vertices to display list */
    if (bgfx_ctx->vertex_count > 0) {
        dl->vertices = (cgns_vertex_t*)malloc(bgfx_ctx->vertex_count * sizeof(cgns_vertex_t));
        if (dl->vertices) {
            memcpy(dl->vertices, bgfx_ctx->vertex_buffer,
                   bgfx_ctx->vertex_count * sizeof(cgns_vertex_t));
            dl->vertex_count = bgfx_ctx->vertex_count;
        } else {
            fprintf(stderr, "Failed to allocate display list vertices\n");
            dl->vertex_count = 0;
        }
    }

    /* Save current state */
    dl->primitive_type = bgfx_ctx->current_primitive;
    memcpy(&dl->material, &bgfx_ctx->current_material, sizeof(cgns_material_t));
    dl->lighting_enabled = bgfx_ctx->lighting_enabled;
    dl->shade_model = bgfx_ctx->shade_model;

    /* Save texture state (Phase 3) */
    dl->texture_enabled = bgfx_ctx->texture_enabled;
    dl->texture_blend_mode = bgfx_ctx->texture_blend_mode;
    /* Save texture ID if one is bound */
    if (bgfx_ctx->texture_enabled && bgfx_ctx->bound_textures[0].idx != UINT16_MAX) {
        dl->texture_id = (unsigned int)bgfx_ctx->bound_textures[0].idx;
    } else {
        dl->texture_id = 0;  /* No texture bound */
    }

    /* Stop recording */
    bgfx_ctx->recording = 0;
    bgfx_ctx->recording_list_id = 0;
    bgfx_ctx->vertex_count = 0;
}

void cgns_render_call_list(cgns_render_context_t* ctx, unsigned int list)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    size_t i;

    if (!bgfx_ctx) return;

    /* Find the display list */
    display_list_t* dl = NULL;
    for (i = 0; i < bgfx_ctx->display_list_count; i++) {
        if (bgfx_ctx->display_lists[i].id == list) {
            dl = &bgfx_ctx->display_lists[i];
            break;
        }
    }

    if (!dl || !dl->vertices || dl->vertex_count == 0) {
        fprintf(stderr, "Display list %u not found or empty\n", list);
        return;
    }

    /* Save current state */
    cgns_material_t saved_material;
    int saved_lighting = bgfx_ctx->lighting_enabled;
    cgns_shade_model_t saved_shade = bgfx_ctx->shade_model;

    /* Save texture state (Phase 3) */
    int saved_texture_enabled = bgfx_ctx->texture_enabled;
    int saved_texture_blend_mode = bgfx_ctx->texture_blend_mode;
    bgfx_texture_handle_t saved_texture = bgfx_ctx->bound_textures[0];

    memcpy(&saved_material, &bgfx_ctx->current_material, sizeof(cgns_material_t));

    /* Restore display list state */
    memcpy(&bgfx_ctx->current_material, &dl->material, sizeof(cgns_material_t));
    bgfx_ctx->lighting_enabled = dl->lighting_enabled;
    bgfx_ctx->shade_model = dl->shade_model;

    /* Restore texture state (Phase 3) */
    bgfx_ctx->texture_enabled = dl->texture_enabled;
    bgfx_ctx->texture_blend_mode = dl->texture_blend_mode;
    if (dl->texture_id != 0) {
        bgfx_ctx->bound_textures[0].idx = (uint16_t)dl->texture_id;
    } else {
        bgfx_ctx->bound_textures[0].idx = UINT16_MAX;  /* No texture */
    }

    /* Render the display list using batch rendering */
    cgns_render_draw_batch(ctx, dl->primitive_type, dl->vertices,
                           dl->vertex_count);

    /* Restore previous state */
    memcpy(&bgfx_ctx->current_material, &saved_material, sizeof(cgns_material_t));
    bgfx_ctx->lighting_enabled = saved_lighting;
    bgfx_ctx->shade_model = saved_shade;

    /* Restore texture state (Phase 3) */
    bgfx_ctx->texture_enabled = saved_texture_enabled;
    bgfx_ctx->texture_blend_mode = saved_texture_blend_mode;
    bgfx_ctx->bound_textures[0] = saved_texture;
}

void cgns_render_delete_list(cgns_render_context_t* ctx, unsigned int list)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    size_t i;

    if (!bgfx_ctx) return;

    /* Find and delete the display list */
    for (i = 0; i < bgfx_ctx->display_list_count; i++) {
        if (bgfx_ctx->display_lists[i].id == list) {
            /* Free vertices */
            if (bgfx_ctx->display_lists[i].vertices) {
                free(bgfx_ctx->display_lists[i].vertices);
            }

            /* Shift remaining display lists down */
            if (i < bgfx_ctx->display_list_count - 1) {
                memmove(&bgfx_ctx->display_lists[i],
                        &bgfx_ctx->display_lists[i + 1],
                        (bgfx_ctx->display_list_count - i - 1) * sizeof(display_list_t));
            }

            bgfx_ctx->display_list_count--;
            return;
        }
    }

    fprintf(stderr, "Display list %u not found for deletion\n", list);
}

/* ========================================================================
 * Texture Support (Phase 3)
 * ======================================================================== */

void cgns_render_texcoord2f(cgns_render_context_t* ctx, float u, float v)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx) return;

    bgfx_ctx->current_texcoord[0] = u;
    bgfx_ctx->current_texcoord[1] = v;
}

void cgns_render_texcoord2fv(cgns_render_context_t* ctx, const float* uv)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx || !uv) return;

    bgfx_ctx->current_texcoord[0] = uv[0];
    bgfx_ctx->current_texcoord[1] = uv[1];
}

unsigned int cgns_render_create_texture(cgns_render_context_t* ctx,
                                         int width, int height,
                                         cgns_texture_format_t format,
                                         const unsigned char* data)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx || !data || width <= 0 || height <= 0) {
        return 0;
    }

    /* Determine bgfx texture format */
    bgfx_texture_format_t bgfx_format;
    switch (format) {
        case CGNS_TEX_FORMAT_RGB:
            bgfx_format = BGFX_TEXTURE_FORMAT_RGB8;
            break;
        case CGNS_TEX_FORMAT_RGBA:
            bgfx_format = BGFX_TEXTURE_FORMAT_RGBA8;
            break;
        case CGNS_TEX_FORMAT_LUMINANCE:
            bgfx_format = BGFX_TEXTURE_FORMAT_R8;
            break;
        case CGNS_TEX_FORMAT_ALPHA:
            bgfx_format = BGFX_TEXTURE_FORMAT_R8;
            break;
        default:
            fprintf(stderr, "Unsupported texture format: %d\n", format);
            return 0;
    }

    /* Calculate data size */
    size_t bytes_per_pixel = 0;
    switch (format) {
        case CGNS_TEX_FORMAT_RGB:       bytes_per_pixel = 3; break;
        case CGNS_TEX_FORMAT_RGBA:      bytes_per_pixel = 4; break;
        case CGNS_TEX_FORMAT_LUMINANCE: bytes_per_pixel = 1; break;
        case CGNS_TEX_FORMAT_ALPHA:     bytes_per_pixel = 1; break;
    }
    size_t data_size = width * height * bytes_per_pixel;

    /* Copy texture data to bgfx memory */
    const bgfx_memory_t* mem = bgfx_copy(data, (uint32_t)data_size);

    /* Create texture */
    bgfx_texture_handle_t texture = bgfx_create_texture_2d(
        (uint16_t)width,
        (uint16_t)height,
        false,  /* No mipmaps */
        1,      /* Layer count */
        bgfx_format,
        BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE,
        mem
    );

    if (texture.idx == UINT16_MAX) {
        fprintf(stderr, "Failed to create texture (%dx%d, format %d)\n",
                width, height, format);
        return 0;
    }

    /* Return handle index as unsigned int */
    return (unsigned int)texture.idx;
}

int cgns_render_update_texture(cgns_render_context_t* ctx,
                                 unsigned int texture,
                                 int width, int height,
                                 cgns_texture_format_t format,
                                 const unsigned char* data)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx || !data || width <= 0 || height <= 0 || texture == 0) {
        return -1;
    }

    bgfx_texture_handle_t tex_handle = { (uint16_t)texture };
    if (tex_handle.idx == UINT16_MAX) {
        fprintf(stderr, "Invalid texture handle: %u\n", texture);
        return -1;
    }

    /* Calculate data size */
    size_t bytes_per_pixel = 0;
    switch (format) {
        case CGNS_TEX_FORMAT_RGB:       bytes_per_pixel = 3; break;
        case CGNS_TEX_FORMAT_RGBA:      bytes_per_pixel = 4; break;
        case CGNS_TEX_FORMAT_LUMINANCE: bytes_per_pixel = 1; break;
        case CGNS_TEX_FORMAT_ALPHA:     bytes_per_pixel = 1; break;
        default: return -1;
    }
    size_t data_size = width * height * bytes_per_pixel;

    /* Copy texture data */
    const bgfx_memory_t* mem = bgfx_copy(data, (uint32_t)data_size);

    /* Update texture */
    bgfx_update_texture_2d(
        tex_handle,
        0,      /* Layer */
        0,      /* Mip level */
        0, 0,   /* x, y offset */
        (uint16_t)width,
        (uint16_t)height,
        mem,
        UINT16_MAX  /* Pitch (auto-calculate) */
    );

    return 0;
}

void cgns_render_bind_texture(cgns_render_context_t* ctx,
                                unsigned int texture,
                                int unit)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx) return;

    if (unit < 0 || unit >= 8) {
        fprintf(stderr, "Invalid texture unit: %d (must be 0-7)\n", unit);
        return;
    }

    if (texture == 0) {
        /* Unbind texture */
        bgfx_ctx->bound_textures[unit].idx = UINT16_MAX;  /* Invalid handle */
        bgfx_ctx->texture_enabled = 0;
    } else {
        /* Bind texture */
        bgfx_ctx->bound_textures[unit].idx = (uint16_t)texture;
        bgfx_ctx->texture_enabled = 1;
    }
}

void cgns_render_set_texture_filter(cgns_render_context_t* ctx,
                                     unsigned int texture,
                                     cgns_texture_filter_t min_filter,
                                     cgns_texture_filter_t mag_filter)
{
    /* Note: bgfx texture filtering is set via sampler flags during creation.
     * For now, this is a no-op. In a full implementation, we'd recreate
     * the texture with new sampler flags or use sampler objects. */
    (void)ctx;
    (void)texture;
    (void)min_filter;
    (void)mag_filter;
}

void cgns_render_set_texture_wrap(cgns_render_context_t* ctx,
                                   unsigned int texture,
                                   cgns_texture_wrap_t wrap_s,
                                   cgns_texture_wrap_t wrap_t)
{
    /* Note: bgfx texture wrapping is set via sampler flags during creation.
     * For now, this is a no-op. See note in set_texture_filter. */
    (void)ctx;
    (void)texture;
    (void)wrap_s;
    (void)wrap_t;
}

void cgns_render_delete_texture(cgns_render_context_t* ctx,
                                 unsigned int texture)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx || texture == 0) return;

    bgfx_texture_handle_t tex_handle = { (uint16_t)texture };
    if (tex_handle.idx != UINT16_MAX) {
        bgfx_destroy_texture(tex_handle);
    }

    /* Unbind from any texture units */
    for (int i = 0; i < 8; i++) {
        if (bgfx_ctx->bound_textures[i].idx == texture) {
            bgfx_ctx->bound_textures[i].idx = UINT16_MAX;  /* Invalid handle */
        }
    }
}

/*
 * Set texture blend mode (Phase 3)
 */
void cgns_render_set_texture_blend_mode(cgns_render_context_t* ctx,
                                         cgns_texture_blend_t mode)
{
    bgfx_context_t* bgfx_ctx = (bgfx_context_t*)ctx;
    if (!bgfx_ctx) return;

    bgfx_ctx->texture_blend_mode = (int)mode;
}

/*
 * Set current color (vector version) - Phase 3 helper
 */
void cgns_render_color4fv(cgns_render_context_t* ctx, const float* rgba)
{
    if (!ctx || !rgba) return;
    cgns_render_set_color4f(ctx, rgba[0], rgba[1], rgba[2], rgba[3]);
}

/*
 * Create orthographic projection matrix - Phase 3 helper
 */
void cgns_render_matrix_ortho(float* matrix,
                               float left, float right,
                               float bottom, float top,
                               float near, float far)
{
    if (!matrix) return;

    /* Column-major 4x4 orthographic projection matrix */
    float rl = right - left;
    float tb = top - bottom;
    float fn = far - near;

    matrix[0] = 2.0f / rl;
    matrix[1] = 0.0f;
    matrix[2] = 0.0f;
    matrix[3] = 0.0f;

    matrix[4] = 0.0f;
    matrix[5] = 2.0f / tb;
    matrix[6] = 0.0f;
    matrix[7] = 0.0f;

    matrix[8] = 0.0f;
    matrix[9] = 0.0f;
    matrix[10] = -2.0f / fn;
    matrix[11] = 0.0f;

    matrix[12] = -(right + left) / rl;
    matrix[13] = -(top + bottom) / tb;
    matrix[14] = -(far + near) / fn;
    matrix[15] = 1.0f;
}

/*
 * Create identity matrix - Phase 3 helper
 */
void cgns_render_matrix_identity(float* matrix)
{
    if (!matrix) return;

    /* Column-major 4x4 identity matrix */
    matrix[0] = 1.0f; matrix[4] = 0.0f; matrix[8]  = 0.0f; matrix[12] = 0.0f;
    matrix[1] = 0.0f; matrix[5] = 1.0f; matrix[9]  = 0.0f; matrix[13] = 0.0f;
    matrix[2] = 0.0f; matrix[6] = 0.0f; matrix[10] = 1.0f; matrix[14] = 0.0f;
    matrix[3] = 0.0f; matrix[7] = 0.0f; matrix[11] = 0.0f; matrix[15] = 1.0f;
}

/*
 * Convenience function: render a frame - Phase 3 helper
 */
void cgns_render_frame(cgns_render_context_t* ctx)
{
    if (!ctx) return;

    cgns_render_begin_frame(ctx);
    cgns_render_clear(ctx, 0.2f, 0.2f, 0.3f, 1.0f);  /* Dark blue-gray background */
    cgns_render_end_frame(ctx);
}

#endif /* CGNS_ENABLE_BGFX */

/* ========================================================================
 * Phase 1 Documentation
 * ======================================================================== */

/*
 * INTEGRATION PLAN FOR PHASE 2:
 *
 * 1. Shader Development:
 *    Create GLSL shaders and convert to SPIR-V/Metal/HLSL using shaderc:
 *    - vs_basic.sc (basic vertex shader)
 *    - fs_flat.sc (flat shading fragment shader)
 *    - fs_smooth.sc (smooth shading fragment shader)
 *    - fs_unlit.sc (unlit rendering for lines)
 *
 * 2. Build System:
 *    Add bgfx and bx (bgfx dependency) to CMake:
 *    - Find or build bgfx
 *    - Compile shaders during build
 *    - Link against bgfx library
 *
 * 3. Testing:
 *    Verify bgfx backend with simple test cases:
 *    - Render a triangle
 *    - Render a cube with lighting
 *    - Render complex CFD mesh
 *    - Test on Windows (DX12), Linux (Vulkan), macOS (Metal)
 *
 * 4. Performance Optimization:
 *    - Use transient buffers for small batches
 *    - Use static buffers for display lists
 *    - Minimize state changes
 *    - Batch similar primitives
 *
 * 5. Feature Parity:
 *    Ensure all OpenGL features are supported:
 *    - All primitive types
 *    - Material system
 *    - Lighting (at least 1-2 lights)
 *    - Transparency/blending
 *    - Display lists
 */
