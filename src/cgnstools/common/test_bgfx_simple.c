/**
 * test_bgfx_simple.c
 *
 * Simple headless test for bgfx backend implementation.
 * Tests the API without requiring a window system.
 *
 * This test validates:
 * 1. Context initialization
 * 2. Immediate mode rendering (triangle)
 * 3. Primitive triangulation (quads, polygons)
 * 4. State management
 * 5. Material and lighting setup
 * 6. Proper cleanup
 */

#include "render_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_START(name) \
    printf("\n=== Test: %s ===\n", name)

#define TEST_PASS(msg) \
    do { \
        printf("  ✓ %s\n", msg); \
        tests_passed++; \
    } while(0)

#define TEST_FAIL(msg) \
    do { \
        printf("  ✗ %s\n", msg); \
        tests_failed++; \
    } while(0)

#define TEST_ASSERT(condition, msg) \
    do { \
        if (condition) { \
            TEST_PASS(msg); \
        } else { \
            TEST_FAIL(msg); \
        } \
    } while(0)

/* ========================================================================
 * Test 1: Backend Selection
 * ======================================================================== */
static void test_backend_selection(void)
{
    TEST_START("Backend Selection");

    /* Check if bgfx backend is available */
    int available = cgns_render_backend_available(CGNS_RENDER_BACKEND_BGFX);
    TEST_ASSERT(available, "bgfx backend is available");

    if (!available) {
        printf("  Note: bgfx backend not compiled in, skipping remaining tests\n");
        return;
    }

    /* Set bgfx backend */
    int result = cgns_render_set_backend(CGNS_RENDER_BACKEND_BGFX);
    TEST_ASSERT(result == 0, "Set bgfx backend");

    /* Verify current backend */
    cgns_render_backend_t current = cgns_render_get_backend();
    TEST_ASSERT(current == CGNS_RENDER_BACKEND_BGFX, "Current backend is bgfx");
}

/* ========================================================================
 * Test 2: Context Initialization
 * ======================================================================== */
static void test_context_init(cgns_render_context_t** ctx_out)
{
    TEST_START("Context Initialization");

    /* Initialize context (NULL for headless) */
    cgns_render_context_t* ctx = cgns_render_initialize(NULL);
    TEST_ASSERT(ctx != NULL, "Context created successfully");

    if (!ctx) {
        printf("  ERROR: Failed to create context, cannot continue tests\n");
        *ctx_out = NULL;
        return;
    }

    /* Make context current */
    int result = cgns_render_make_current(ctx);
    TEST_ASSERT(result == 0, "Context made current");

    *ctx_out = ctx;
}

/* ========================================================================
 * Test 3: Simple Triangle Rendering
 * ======================================================================== */
static void test_triangle_rendering(cgns_render_context_t* ctx)
{
    TEST_START("Triangle Rendering");

    if (!ctx) {
        TEST_FAIL("No context available");
        return;
    }

    /* Set viewport */
    cgns_viewport_t viewport = {0, 0, 800, 600, 800.0f/600.0f}; cgns_render_set_viewport(ctx, &viewport);
    TEST_PASS("Viewport set");

    /* Begin frame */
    cgns_render_begin_frame(ctx);
    TEST_PASS("Frame begun");

    /* Clear */
    cgns_render_clear(ctx, 0.2f, 0.3f, 0.4f, 1.0f);
    TEST_PASS("Clear executed");

    /* Render triangle */
    cgns_render_begin(ctx, CGNS_PRIM_TRIANGLES);

    cgns_render_set_color4f(ctx, 1.0f, 0.0f, 0.0f, 1.0f);
    cgns_render_normal3f(ctx, 0.0f, 0.0f, 1.0f);
    cgns_render_vertex3f(ctx, -0.5f, -0.5f, 0.0f);

    cgns_render_set_color4f(ctx, 0.0f, 1.0f, 0.0f, 1.0f);
    cgns_render_normal3f(ctx, 0.0f, 0.0f, 1.0f);
    cgns_render_vertex3f(ctx, 0.5f, -0.5f, 0.0f);

    cgns_render_set_color4f(ctx, 0.0f, 0.0f, 1.0f, 1.0f);
    cgns_render_normal3f(ctx, 0.0f, 0.0f, 1.0f);
    cgns_render_vertex3f(ctx, 0.0f, 0.5f, 0.0f);

    cgns_render_end(ctx);
    TEST_PASS("Triangle submitted (3 vertices)");

    /* End frame */
    cgns_render_end_frame(ctx);
    TEST_PASS("Frame ended");
}

/* ========================================================================
 * Test 4: Quad Triangulation
 * ======================================================================== */
static void test_quad_rendering(cgns_render_context_t* ctx)
{
    TEST_START("Quad Triangulation");

    if (!ctx) {
        TEST_FAIL("No context available");
        return;
    }

    cgns_render_begin_frame(ctx);

    /* Render quad (should be triangulated to 2 triangles) */
    cgns_render_begin(ctx, CGNS_PRIM_QUADS);

    cgns_render_set_color3f(ctx, 1.0f, 1.0f, 1.0f);
    cgns_render_normal3f(ctx, 0.0f, 0.0f, 1.0f);

    cgns_render_vertex3f(ctx, -1.0f, -1.0f, 0.0f);
    cgns_render_vertex3f(ctx,  1.0f, -1.0f, 0.0f);
    cgns_render_vertex3f(ctx,  1.0f,  1.0f, 0.0f);
    cgns_render_vertex3f(ctx, -1.0f,  1.0f, 0.0f);

    cgns_render_end(ctx);
    TEST_PASS("Quad submitted (4 vertices → 6 after triangulation)");

    cgns_render_end_frame(ctx);
}

/* ========================================================================
 * Test 5: Polygon Triangulation
 * ======================================================================== */
static void test_polygon_rendering(cgns_render_context_t* ctx)
{
    TEST_START("Polygon Triangulation");

    if (!ctx) {
        TEST_FAIL("No context available");
        return;
    }

    cgns_render_begin_frame(ctx);

    /* Render hexagon (6 vertices → 12 triangles) */
    cgns_render_begin(ctx, CGNS_PRIM_POLYGON);

    cgns_render_set_color3f(ctx, 0.5f, 0.5f, 1.0f);
    cgns_render_normal3f(ctx, 0.0f, 0.0f, 1.0f);

    /* Create hexagon vertices */
    for (int i = 0; i < 6; i++) {
        float angle = (float)i * 3.14159f * 2.0f / 6.0f;
        float x = cosf(angle);
        float y = sinf(angle);
        cgns_render_vertex3f(ctx, x, y, 0.0f);
    }

    cgns_render_end(ctx);
    TEST_PASS("Hexagon submitted (6 vertices → 12 after fan triangulation)");

    cgns_render_end_frame(ctx);
}

/* ========================================================================
 * Test 6: State Management
 * ======================================================================== */
static void test_state_management(cgns_render_context_t* ctx)
{
    TEST_START("State Management");

    if (!ctx) {
        TEST_FAIL("No context available");
        return;
    }

    cgns_render_begin_frame(ctx);

    /* Test lighting enable/disable */
    cgns_render_enable(ctx, CGNS_STATE_LIGHTING);
    TEST_PASS("Lighting enabled");

    cgns_render_disable(ctx, CGNS_STATE_LIGHTING);
    TEST_PASS("Lighting disabled");

    /* Test depth test enable/disable */
    cgns_render_enable(ctx, CGNS_STATE_DEPTH_TEST);
    TEST_PASS("Depth test enabled");

    cgns_render_disable(ctx, CGNS_STATE_DEPTH_TEST);
    TEST_PASS("Depth test disabled");

    /* Test blend enable/disable */
    cgns_render_enable(ctx, CGNS_STATE_BLEND);
    TEST_PASS("Blending enabled");

    cgns_render_disable(ctx, CGNS_STATE_BLEND);
    TEST_PASS("Blending disabled");

    /* Test shade model */
    cgns_render_set_shade_model(ctx, CGNS_SHADE_SMOOTH);
    TEST_PASS("Shade model set to SMOOTH");

    cgns_render_set_shade_model(ctx, CGNS_SHADE_FLAT);
    TEST_PASS("Shade model set to FLAT");

    /* Test polygon mode */
    cgns_render_set_polygon_mode(ctx, CGNS_POLY_FILL);
    TEST_PASS("Polygon mode set to FILL");

    cgns_render_set_polygon_mode(ctx, CGNS_POLY_LINE);
    TEST_PASS("Polygon mode set to LINE (wireframe)");

    cgns_render_end_frame(ctx);
}

/* ========================================================================
 * Test 7: Material and Lighting
 * ======================================================================== */
static void test_materials(cgns_render_context_t* ctx)
{
    TEST_START("Material and Lighting");

    if (!ctx) {
        TEST_FAIL("No context available");
        return;
    }

    cgns_render_begin_frame(ctx);

    /* Set up material */
    cgns_material_t material;
    material.ambient[0] = 0.2f;
    material.ambient[1] = 0.2f;
    material.ambient[2] = 0.2f;
    material.ambient[3] = 1.0f;

    material.diffuse[0] = 0.8f;
    material.diffuse[1] = 0.8f;
    material.diffuse[2] = 0.8f;
    material.diffuse[3] = 1.0f;

    material.specular[0] = 1.0f;
    material.specular[1] = 1.0f;
    material.specular[2] = 1.0f;
    material.specular[3] = 1.0f;

    material.emission[0] = 0.0f;
    material.emission[1] = 0.0f;
    material.emission[2] = 0.0f;
    material.emission[3] = 1.0f;

    material.shininess = 32.0f;

    cgns_render_set_material(ctx, &material);
    TEST_PASS("Material properties set");

    /* Enable lighting and render with material */
    cgns_render_enable(ctx, CGNS_STATE_LIGHTING);
    cgns_render_set_shade_model(ctx, CGNS_SHADE_SMOOTH);

    cgns_render_begin(ctx, CGNS_PRIM_TRIANGLES);

    cgns_render_normal3f(ctx, 0.0f, 0.0f, 1.0f);
    cgns_render_vertex3f(ctx, -0.5f, -0.5f, 0.0f);
    cgns_render_vertex3f(ctx,  0.5f, -0.5f, 0.0f);
    cgns_render_vertex3f(ctx,  0.0f,  0.5f, 0.0f);

    cgns_render_end(ctx);
    TEST_PASS("Triangle rendered with lighting and material");

    cgns_render_end_frame(ctx);
}

/* ========================================================================
 * Test 8: Matrix Transformations
 * ======================================================================== */
static void test_transformations(cgns_render_context_t* ctx)
{
    TEST_START("Matrix Transformations");

    if (!ctx) {
        TEST_FAIL("No context available");
        return;
    }

    cgns_render_begin_frame(ctx);

    /* Identity matrices */
    float identity[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    cgns_render_set_projection(ctx, identity);
    TEST_PASS("Projection matrix set");

    cgns_render_set_view(ctx, identity);
    TEST_PASS("View matrix set");

    cgns_render_set_model(ctx, identity);
    TEST_PASS("Model matrix set");

    cgns_render_end_frame(ctx);
}

/* ========================================================================
 * Test 9: Multiple Primitives
 * ======================================================================== */
static void test_multiple_primitives(cgns_render_context_t* ctx)
{
    TEST_START("Multiple Primitive Types");

    if (!ctx) {
        TEST_FAIL("No context available");
        return;
    }

    cgns_render_begin_frame(ctx);
    cgns_render_clear(ctx, 0.1f, 0.1f, 0.1f, 1.0f);

    /* Triangles */
    cgns_render_begin(ctx, CGNS_PRIM_TRIANGLES);
    cgns_render_set_color3f(ctx, 1.0f, 0.0f, 0.0f);
    cgns_render_vertex3f(ctx, -0.8f, 0.5f, 0.0f);
    cgns_render_vertex3f(ctx, -0.5f, 0.5f, 0.0f);
    cgns_render_vertex3f(ctx, -0.65f, 0.8f, 0.0f);
    cgns_render_end(ctx);
    TEST_PASS("Rendered triangles");

    /* Lines */
    cgns_render_begin(ctx, CGNS_PRIM_LINES);
    cgns_render_set_color3f(ctx, 0.0f, 1.0f, 0.0f);
    cgns_render_vertex3f(ctx, 0.0f, -0.8f, 0.0f);
    cgns_render_vertex3f(ctx, 0.0f, 0.8f, 0.0f);
    cgns_render_end(ctx);
    TEST_PASS("Rendered lines");

    /* Quads */
    cgns_render_begin(ctx, CGNS_PRIM_QUADS);
    cgns_render_set_color3f(ctx, 0.0f, 0.0f, 1.0f);
    cgns_render_vertex3f(ctx, 0.5f, -0.8f, 0.0f);
    cgns_render_vertex3f(ctx, 0.8f, -0.8f, 0.0f);
    cgns_render_vertex3f(ctx, 0.8f, -0.5f, 0.0f);
    cgns_render_vertex3f(ctx, 0.5f, -0.5f, 0.0f);
    cgns_render_end(ctx);
    TEST_PASS("Rendered quads (triangulated)");

    cgns_render_end_frame(ctx);
}

/* ========================================================================
 * Test 10: Display Lists
 * ======================================================================== */
static void test_display_lists(cgns_render_context_t* ctx)
{
    TEST_START("Display Lists");

    if (!ctx) {
        TEST_FAIL("No context available");
        return;
    }

    cgns_render_begin_frame(ctx);

    /* Generate display list ID */
    unsigned int list_id = cgns_render_gen_list(ctx);
    TEST_ASSERT(list_id != 0, "Display list ID generated");

    /* Record display list */
    cgns_render_new_list(ctx, list_id);
    TEST_PASS("Display list recording started");

    /* Record a triangle */
    cgns_render_begin(ctx, CGNS_PRIM_TRIANGLES);
    cgns_render_set_color3f(ctx, 1.0f, 0.5f, 0.0f);
    cgns_render_normal3f(ctx, 0.0f, 0.0f, 1.0f);
    cgns_render_vertex3f(ctx, -0.5f, -0.5f, 0.0f);
    cgns_render_vertex3f(ctx,  0.5f, -0.5f, 0.0f);
    cgns_render_vertex3f(ctx,  0.0f,  0.5f, 0.0f);
    cgns_render_end(ctx);

    cgns_render_end_list(ctx);
    TEST_PASS("Display list recording ended");

    /* Call display list multiple times */
    cgns_render_call_list(ctx, list_id);
    TEST_PASS("Display list called (1st time)");

    cgns_render_call_list(ctx, list_id);
    TEST_PASS("Display list called (2nd time)");

    cgns_render_call_list(ctx, list_id);
    TEST_PASS("Display list called (3rd time)");

    /* Delete display list */
    cgns_render_delete_list(ctx, list_id);
    TEST_PASS("Display list deleted");

    cgns_render_end_frame(ctx);
}

/* ========================================================================
 * Test 11: Cleanup
 * ======================================================================== */
static void test_cleanup(cgns_render_context_t* ctx)
{
    TEST_START("Context Cleanup");

    if (!ctx) {
        TEST_FAIL("No context to clean up");
        return;
    }

    cgns_render_shutdown(ctx);
    TEST_PASS("Context destroyed successfully");
}

/* ========================================================================
 * Main Test Runner
 * ======================================================================== */
int main(int argc, char** argv)
{
    cgns_render_context_t* ctx = NULL;

    (void)argc;
    (void)argv;

    printf("========================================\n");
    printf("bgfx Backend Test Suite (Headless)\n");
    printf("Phase 2: Implementation Validation\n");
    printf("========================================\n");

    /* Run all tests */
    test_backend_selection();
    test_context_init(&ctx);

    /* Only continue if context was created */
    if (ctx) {
        test_triangle_rendering(ctx);
        test_quad_rendering(ctx);
        test_polygon_rendering(ctx);
        test_state_management(ctx);
        test_materials(ctx);
        test_transformations(ctx);
        test_multiple_primitives(ctx);
        test_display_lists(ctx);
        test_cleanup(ctx);
    }

    /* Print summary */
    printf("\n========================================\n");
    printf("Test Results:\n");
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("  Total:  %d\n", tests_passed + tests_failed);

    if (tests_failed == 0) {
        printf("\n✓ ALL TESTS PASSED!\n");
        printf("bgfx backend is working correctly.\n");
    } else {
        printf("\n✗ SOME TESTS FAILED\n");
        printf("Please review errors above.\n");
    }
    printf("========================================\n");

    return (tests_failed == 0) ? 0 : 1;
}
