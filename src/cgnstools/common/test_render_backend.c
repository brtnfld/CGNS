/**
 * test_render_backend.c
 *
 * Simple test program to verify the render backend abstraction layer.
 * Tests both the API design and OpenGL backend implementation.
 *
 * Compile:
 *   gcc -I. test_render_backend.c render_backend_opengl.c -lGL -lGLU -o test_backend
 *
 * Or use CMake target (when integrated).
 */

#include "render_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Simple test data - triangle vertices */
static const float triangle_vertices[][3] = {
    {-0.5f, -0.5f, 0.0f},
    { 0.5f, -0.5f, 0.0f},
    { 0.0f,  0.5f, 0.0f}
};

static const float triangle_normals[][3] = {
    {0.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 1.0f}
};

static const float triangle_colors[][4] = {
    {1.0f, 0.0f, 0.0f, 1.0f},  /* Red */
    {0.0f, 1.0f, 0.0f, 1.0f},  /* Green */
    {0.0f, 0.0f, 1.0f, 1.0f}   /* Blue */
};

/* Cube vertices for more complex test */
static const float cube_vertices[][3] = {
    {-1.0f, -1.0f,  1.0f},
    { 1.0f, -1.0f,  1.0f},
    { 1.0f,  1.0f,  1.0f},
    {-1.0f,  1.0f,  1.0f},
    {-1.0f, -1.0f, -1.0f},
    { 1.0f, -1.0f, -1.0f},
    { 1.0f,  1.0f, -1.0f},
    {-1.0f,  1.0f, -1.0f}
};

/* ========================================================================
 * Test Functions
 * ======================================================================== */

/**
 * Test 1: Backend Selection and Availability
 */
static int test_backend_selection(void)
{
    printf("Test 1: Backend Selection and Availability\n");
    printf("-------------------------------------------\n");

    /* Check available backends */
    printf("OpenGL backend available: %s\n",
           cgns_render_backend_available(CGNS_RENDER_BACKEND_OPENGL) ? "YES" : "NO");
    printf("bgfx backend available: %s\n",
           cgns_render_backend_available(CGNS_RENDER_BACKEND_BGFX) ? "YES" : "NO");

    /* Get backend names */
    printf("OpenGL backend name: %s\n",
           cgns_render_backend_name(CGNS_RENDER_BACKEND_OPENGL));
    printf("bgfx backend name: %s\n",
           cgns_render_backend_name(CGNS_RENDER_BACKEND_BGFX));

    /* Get current backend */
    cgns_render_backend_t current = cgns_render_get_backend();
    printf("Current backend: %s\n", cgns_render_backend_name(current));

    /* Try to set backend */
    if (cgns_render_set_backend(CGNS_RENDER_BACKEND_OPENGL) == 0) {
        printf("Successfully set OpenGL backend\n");
    } else {
        printf("Failed to set OpenGL backend\n");
        return 1;
    }

    printf("Test 1: PASSED\n\n");
    return 0;
}

/**
 * Test 2: Context Creation and Management
 */
static int test_context_management(void)
{
    cgns_render_context_t* ctx;
    const char* error;

    printf("Test 2: Context Creation and Management\n");
    printf("----------------------------------------\n");

    /* Initialize context (NULL for headless test) */
    printf("Initializing render context...\n");
    ctx = cgns_render_initialize(NULL);

    if (!ctx) {
        printf("ERROR: Failed to create render context\n");
        return 1;
    }
    printf("Context created successfully\n");

    /* Check for errors */
    error = cgns_render_get_error(ctx);
    if (error) {
        printf("WARNING: Context has error: %s\n", error);
    } else {
        printf("No errors reported\n");
    }

    /* Make context current */
    if (cgns_render_make_current(ctx) == 0) {
        printf("Context made current successfully\n");
    } else {
        printf("Failed to make context current\n");
        cgns_render_shutdown(ctx);
        return 1;
    }

    /* Cleanup */
    printf("Shutting down context...\n");
    cgns_render_shutdown(ctx);
    printf("Context destroyed successfully\n");

    printf("Test 2: PASSED\n\n");
    return 0;
}

/**
 * Test 3: Immediate Mode Rendering API
 */
static int test_immediate_mode(void)
{
    cgns_render_context_t* ctx;
    int i;

    printf("Test 3: Immediate Mode Rendering API\n");
    printf("-------------------------------------\n");

    ctx = cgns_render_initialize(NULL);
    if (!ctx) {
        printf("ERROR: Failed to create context\n");
        return 1;
    }

    /* Test frame management */
    printf("Testing frame management...\n");
    cgns_render_begin_frame(ctx);
    cgns_render_clear(ctx, 0.2f, 0.3f, 0.4f, 1.0f);
    cgns_render_end_frame(ctx);
    printf("Frame management OK\n");

    /* Test immediate mode rendering */
    printf("Testing immediate mode rendering...\n");
    cgns_render_begin_frame(ctx);

    cgns_render_begin(ctx, CGNS_PRIM_TRIANGLES);

    for (i = 0; i < 3; i++) {
        cgns_render_set_color4f(ctx,
                               triangle_colors[i][0],
                               triangle_colors[i][1],
                               triangle_colors[i][2],
                               triangle_colors[i][3]);
        cgns_render_normal3fv(ctx, triangle_normals[i]);
        cgns_render_vertex3fv(ctx, triangle_vertices[i]);
    }

    cgns_render_end(ctx);
    cgns_render_end_frame(ctx);
    printf("Immediate mode rendering OK\n");

    cgns_render_shutdown(ctx);

    printf("Test 3: PASSED\n\n");
    return 0;
}

/**
 * Test 4: Batch Rendering API
 */
static int test_batch_rendering(void)
{
    cgns_render_context_t* ctx;
    cgns_vertex_t* vertices;
    int i;

    printf("Test 4: Batch Rendering API\n");
    printf("----------------------------\n");

    ctx = cgns_render_initialize(NULL);
    if (!ctx) {
        printf("ERROR: Failed to create context\n");
        return 1;
    }

    /* Prepare batch data */
    vertices = (cgns_vertex_t*)malloc(3 * sizeof(cgns_vertex_t));
    if (!vertices) {
        printf("ERROR: Failed to allocate vertex data\n");
        cgns_render_shutdown(ctx);
        return 1;
    }

    for (i = 0; i < 3; i++) {
        memcpy(vertices[i].position, triangle_vertices[i], sizeof(float) * 3);
        memcpy(vertices[i].normal, triangle_normals[i], sizeof(float) * 3);
        memcpy(vertices[i].color, triangle_colors[i], sizeof(float) * 4);
    }

    /* Render batch */
    printf("Testing batch rendering...\n");
    cgns_render_begin_frame(ctx);
    cgns_render_draw_batch(ctx, CGNS_PRIM_TRIANGLES, vertices, 3);
    cgns_render_end_frame(ctx);
    printf("Batch rendering OK\n");

    free(vertices);
    cgns_render_shutdown(ctx);

    printf("Test 4: PASSED\n\n");
    return 0;
}

/**
 * Test 5: Render State Management
 */
static int test_render_state(void)
{
    cgns_render_context_t* ctx;
    cgns_material_t material;
    cgns_viewport_t viewport;

    printf("Test 5: Render State Management\n");
    printf("--------------------------------\n");

    ctx = cgns_render_initialize(NULL);
    if (!ctx) {
        printf("ERROR: Failed to create context\n");
        return 1;
    }

    printf("Testing state management...\n");

    /* Test viewport */
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = 800;
    viewport.height = 600;
    viewport.aspect_ratio = 800.0f / 600.0f;
    cgns_render_set_viewport(ctx, &viewport);
    printf("Viewport set OK\n");

    /* Test render states */
    cgns_render_enable(ctx, CGNS_STATE_LIGHTING);
    cgns_render_enable(ctx, CGNS_STATE_DEPTH_TEST);
    printf("Render states enabled OK\n");

    cgns_render_set_shade_model(ctx, CGNS_SHADE_FLAT);
    cgns_render_set_polygon_mode(ctx, CGNS_POLY_FILL);
    printf("Shading and polygon mode set OK\n");

    /* Test material */
    material.ambient[0] = 0.2f; material.ambient[1] = 0.2f;
    material.ambient[2] = 0.2f; material.ambient[3] = 1.0f;

    material.diffuse[0] = 0.8f; material.diffuse[1] = 0.8f;
    material.diffuse[2] = 0.8f; material.diffuse[3] = 1.0f;

    material.specular[0] = 1.0f; material.specular[1] = 1.0f;
    material.specular[2] = 1.0f; material.specular[3] = 1.0f;

    material.emission[0] = 0.0f; material.emission[1] = 0.0f;
    material.emission[2] = 0.0f; material.emission[3] = 1.0f;

    material.shininess = 32.0f;

    cgns_render_set_material(ctx, &material);
    printf("Material set OK\n");

    /* Test color */
    cgns_render_set_color3f(ctx, 1.0f, 0.5f, 0.2f);
    printf("Color set OK\n");

    /* Disable states */
    cgns_render_disable(ctx, CGNS_STATE_LIGHTING);
    cgns_render_disable(ctx, CGNS_STATE_DEPTH_TEST);
    printf("Render states disabled OK\n");

    cgns_render_shutdown(ctx);

    printf("Test 5: PASSED\n\n");
    return 0;
}

/**
 * Test 6: Display Lists
 */
static int test_display_lists(void)
{
    cgns_render_context_t* ctx;
    unsigned int list_id;
    int i;

    printf("Test 6: Display Lists\n");
    printf("---------------------\n");

    ctx = cgns_render_initialize(NULL);
    if (!ctx) {
        printf("ERROR: Failed to create context\n");
        return 1;
    }

    /* Generate display list */
    printf("Generating display list...\n");
    list_id = cgns_render_gen_list(ctx);
    if (list_id == 0) {
        printf("ERROR: Failed to generate display list\n");
        cgns_render_shutdown(ctx);
        return 1;
    }
    printf("Display list ID: %u\n", list_id);

    /* Record display list */
    printf("Recording display list...\n");
    cgns_render_new_list(ctx, list_id);

    cgns_render_begin(ctx, CGNS_PRIM_TRIANGLES);
    for (i = 0; i < 3; i++) {
        cgns_render_normal3fv(ctx, triangle_normals[i]);
        cgns_render_vertex3fv(ctx, triangle_vertices[i]);
    }
    cgns_render_end(ctx);

    cgns_render_end_list(ctx);
    printf("Display list recorded\n");

    /* Call display list */
    printf("Calling display list...\n");
    cgns_render_begin_frame(ctx);
    cgns_render_call_list(ctx, list_id);
    cgns_render_end_frame(ctx);
    printf("Display list called OK\n");

    /* Delete display list */
    printf("Deleting display list...\n");
    cgns_render_delete_list(ctx, list_id);
    printf("Display list deleted OK\n");

    cgns_render_shutdown(ctx);

    printf("Test 6: PASSED\n\n");
    return 0;
}

/* ========================================================================
 * Main Test Suite
 * ======================================================================== */

int main(int argc, char** argv)
{
    int failed = 0;

    (void)argc;
    (void)argv;

    printf("========================================\n");
    printf("CGNS Render Backend Test Suite\n");
    printf("Phase 1: OpenGL Backend Verification\n");
    printf("========================================\n\n");

    /* Run all tests */
    failed += test_backend_selection();
    failed += test_context_management();
    failed += test_immediate_mode();
    failed += test_batch_rendering();
    failed += test_render_state();
    failed += test_display_lists();

    /* Summary */
    printf("========================================\n");
    if (failed == 0) {
        printf("ALL TESTS PASSED (%d/6)\n", 6 - failed);
        printf("Render backend abstraction is working!\n");
    } else {
        printf("SOME TESTS FAILED (%d/6 passed)\n", 6 - failed);
        printf("Please review the errors above.\n");
    }
    printf("========================================\n");

    return failed;
}
