/*
 * test_bgfx_texture.c
 * Comprehensive texture testing for CGNS bgfx rendering backend
 * Phase 3: Texture support validation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "render_backend.h"

#define TEST_WIDTH 64
#define TEST_HEIGHT 64

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Color definitions for testing */
static const float RED[4] = {1.0f, 0.0f, 0.0f, 1.0f};
static const float GREEN[4] = {0.0f, 1.0f, 0.0f, 1.0f};
static const float BLUE[4] = {0.0f, 0.0f, 1.0f, 1.0f};
static const float WHITE[4] = {1.0f, 1.0f, 1.0f, 1.0f};
static const float YELLOW[4] = {1.0f, 1.0f, 0.0f, 1.0f};

/* Test result macros */
#define TEST_START(name) \
    do { \
        printf("  [TEST] %s...", name); \
        fflush(stdout); \
        tests_run++; \
    } while(0)

#define TEST_PASS() \
    do { \
        printf(" PASS\n"); \
        tests_passed++; \
    } while(0)

#define TEST_FAIL(msg) \
    do { \
        printf(" FAIL: %s\n", msg); \
        tests_failed++; \
    } while(0)

#define ASSERT(condition, msg) \
    do { \
        if (!(condition)) { \
            TEST_FAIL(msg); \
            return 0; \
        } \
    } while(0)

/*
 * Generate a checkerboard pattern texture
 * Returns newly allocated texture data (caller must free)
 */
static unsigned char* generate_checkerboard_rgb(int width, int height, int square_size)
{
    unsigned char* data = (unsigned char*)malloc(width * height * 3);
    if (!data) return NULL;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int square_x = x / square_size;
            int square_y = y / square_size;
            int is_white = (square_x + square_y) % 2;

            int idx = (y * width + x) * 3;
            unsigned char value = is_white ? 255 : 0;
            data[idx + 0] = value;  /* R */
            data[idx + 1] = value;  /* G */
            data[idx + 2] = value;  /* B */
        }
    }

    return data;
}

/*
 * Generate a checkerboard pattern texture (RGBA)
 */
static unsigned char* generate_checkerboard_rgba(int width, int height, int square_size)
{
    unsigned char* data = (unsigned char*)malloc(width * height * 4);
    if (!data) return NULL;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int square_x = x / square_size;
            int square_y = y / square_size;
            int is_white = (square_x + square_y) % 2;

            int idx = (y * width + x) * 4;
            unsigned char value = is_white ? 255 : 0;
            data[idx + 0] = value;  /* R */
            data[idx + 1] = value;  /* G */
            data[idx + 2] = value;  /* B */
            data[idx + 3] = 255;     /* A */
        }
    }

    return data;
}

/*
 * Generate a gradient texture for testing
 */
static unsigned char* generate_gradient_rgb(int width, int height)
{
    unsigned char* data = (unsigned char*)malloc(width * height * 3);
    if (!data) return NULL;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 3;
            data[idx + 0] = (unsigned char)((x * 255) / width);   /* R gradient horizontal */
            data[idx + 1] = (unsigned char)((y * 255) / height);  /* G gradient vertical */
            data[idx + 2] = 128;                                   /* B constant */
        }
    }

    return data;
}

/*
 * Generate a luminance gradient texture
 */
static unsigned char* generate_luminance(int width, int height)
{
    unsigned char* data = (unsigned char*)malloc(width * height);
    if (!data) return NULL;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            data[idx] = (unsigned char)((x * 255) / width);
        }
    }

    return data;
}

/*
 * Test 1: Texture creation and deletion (RGB)
 */
static int test_texture_create_delete_rgb(cgns_render_context_t* ctx)
{
    TEST_START("Texture creation and deletion (RGB)");

    unsigned char* data = generate_checkerboard_rgb(TEST_WIDTH, TEST_HEIGHT, 8);
    ASSERT(data != NULL, "Failed to generate checkerboard texture");

    /* Create texture */
    unsigned int tex = cgns_render_create_texture(ctx, TEST_WIDTH, TEST_HEIGHT,
                                                    CGNS_TEX_FORMAT_RGB, data);
    free(data);

    ASSERT(tex != 0, "Failed to create RGB texture");

    /* Delete texture */
    cgns_render_delete_texture(ctx, tex);

    TEST_PASS();
    return 1;
}

/*
 * Test 2: Texture creation (RGBA)
 */
static int test_texture_create_delete_rgba(cgns_render_context_t* ctx)
{
    TEST_START("Texture creation and deletion (RGBA)");

    unsigned char* data = generate_checkerboard_rgba(TEST_WIDTH, TEST_HEIGHT, 8);
    ASSERT(data != NULL, "Failed to generate RGBA texture");

    unsigned int tex = cgns_render_create_texture(ctx, TEST_WIDTH, TEST_HEIGHT,
                                                    CGNS_TEX_FORMAT_RGBA, data);
    free(data);

    ASSERT(tex != 0, "Failed to create RGBA texture");

    cgns_render_delete_texture(ctx, tex);

    TEST_PASS();
    return 1;
}

/*
 * Test 3: Texture creation (Luminance)
 */
static int test_texture_create_delete_luminance(cgns_render_context_t* ctx)
{
    TEST_START("Texture creation and deletion (Luminance)");

    unsigned char* data = generate_luminance(TEST_WIDTH, TEST_HEIGHT);
    ASSERT(data != NULL, "Failed to generate luminance texture");

    unsigned int tex = cgns_render_create_texture(ctx, TEST_WIDTH, TEST_HEIGHT,
                                                    CGNS_TEX_FORMAT_LUMINANCE, data);
    free(data);

    ASSERT(tex != 0, "Failed to create luminance texture");

    cgns_render_delete_texture(ctx, tex);

    TEST_PASS();
    return 1;
}

/*
 * Test 4: Multiple texture creation
 */
static int test_multiple_textures(cgns_render_context_t* ctx)
{
    TEST_START("Multiple texture creation");

    unsigned int textures[4];

    /* Create 4 different textures */
    unsigned char* data1 = generate_checkerboard_rgb(32, 32, 4);
    unsigned char* data2 = generate_checkerboard_rgb(64, 64, 8);
    unsigned char* data3 = generate_gradient_rgb(128, 128);
    unsigned char* data4 = generate_luminance(64, 64);

    ASSERT(data1 && data2 && data3 && data4, "Failed to generate test textures");

    textures[0] = cgns_render_create_texture(ctx, 32, 32, CGNS_TEX_FORMAT_RGB, data1);
    textures[1] = cgns_render_create_texture(ctx, 64, 64, CGNS_TEX_FORMAT_RGB, data2);
    textures[2] = cgns_render_create_texture(ctx, 128, 128, CGNS_TEX_FORMAT_RGB, data3);
    textures[3] = cgns_render_create_texture(ctx, 64, 64, CGNS_TEX_FORMAT_LUMINANCE, data4);

    free(data1);
    free(data2);
    free(data3);
    free(data4);

    ASSERT(textures[0] != 0 && textures[1] != 0 && textures[2] != 0 && textures[3] != 0,
           "Failed to create multiple textures");

    /* Verify unique IDs */
    ASSERT(textures[0] != textures[1] && textures[0] != textures[2] && textures[0] != textures[3],
           "Texture IDs are not unique");

    /* Delete all */
    for (int i = 0; i < 4; i++) {
        cgns_render_delete_texture(ctx, textures[i]);
    }

    TEST_PASS();
    return 1;
}

/*
 * Test 5: Texture binding and unbinding
 */
static int test_texture_binding(cgns_render_context_t* ctx)
{
    TEST_START("Texture binding and unbinding");

    unsigned char* data = generate_checkerboard_rgb(TEST_WIDTH, TEST_HEIGHT, 8);
    ASSERT(data != NULL, "Failed to generate texture");

    unsigned int tex = cgns_render_create_texture(ctx, TEST_WIDTH, TEST_HEIGHT,
                                                    CGNS_TEX_FORMAT_RGB, data);
    free(data);
    ASSERT(tex != 0, "Failed to create texture");

    /* Bind texture to unit 0 */
    cgns_render_bind_texture(ctx, tex, 0);

    /* Unbind texture (bind 0) */
    cgns_render_bind_texture(ctx, 0, 0);

    /* Cleanup */
    cgns_render_delete_texture(ctx, tex);

    TEST_PASS();
    return 1;
}

/*
 * Test 6: Render textured quad (basic)
 */
static int test_render_textured_quad(cgns_render_context_t* ctx)
{
    TEST_START("Render textured quad");

    unsigned char* data = generate_checkerboard_rgb(TEST_WIDTH, TEST_HEIGHT, 8);
    ASSERT(data != NULL, "Failed to generate texture");

    unsigned int tex = cgns_render_create_texture(ctx, TEST_WIDTH, TEST_HEIGHT,
                                                    CGNS_TEX_FORMAT_RGB, data);
    free(data);
    ASSERT(tex != 0, "Failed to create texture");

    /* Setup projection */
    float proj[16], view[16];
    cgns_render_matrix_ortho(proj, -2.0f, 2.0f, -2.0f, 2.0f, -1.0f, 1.0f);
    cgns_render_matrix_identity(view);

    cgns_render_set_projection(ctx, proj);
    cgns_render_set_view(ctx, view);

    /* Bind texture */
    cgns_render_bind_texture(ctx, tex, 0);

    /* Draw textured quad */
    cgns_render_begin(ctx, CGNS_PRIM_QUADS);

    cgns_render_color4fv(ctx, WHITE);

    cgns_render_texcoord2f(ctx, 0.0f, 0.0f);
    cgns_render_vertex3f(ctx, -1.0f, -1.0f, 0.0f);

    cgns_render_texcoord2f(ctx, 1.0f, 0.0f);
    cgns_render_vertex3f(ctx, 1.0f, -1.0f, 0.0f);

    cgns_render_texcoord2f(ctx, 1.0f, 1.0f);
    cgns_render_vertex3f(ctx, 1.0f, 1.0f, 0.0f);

    cgns_render_texcoord2f(ctx, 0.0f, 1.0f);
    cgns_render_vertex3f(ctx, -1.0f, 1.0f, 0.0f);

    cgns_render_end(ctx);

    /* Frame should execute without errors */
    cgns_render_frame(ctx);

    /* Cleanup */
    cgns_render_bind_texture(ctx, 0, 0);
    cgns_render_delete_texture(ctx, tex);

    TEST_PASS();
    return 1;
}

/*
 * Test 7: Blend mode - Modulate
 */
static int test_blend_mode_modulate(cgns_render_context_t* ctx)
{
    TEST_START("Blend mode: Modulate");

    unsigned char* data = generate_checkerboard_rgb(TEST_WIDTH, TEST_HEIGHT, 8);
    ASSERT(data != NULL, "Failed to generate texture");

    unsigned int tex = cgns_render_create_texture(ctx, TEST_WIDTH, TEST_HEIGHT,
                                                    CGNS_TEX_FORMAT_RGB, data);
    free(data);
    ASSERT(tex != 0, "Failed to create texture");

    /* Setup projection */
    float proj[16], view[16];
    cgns_render_matrix_ortho(proj, -2.0f, 2.0f, -2.0f, 2.0f, -1.0f, 1.0f);
    cgns_render_matrix_identity(view);
    cgns_render_set_projection(ctx, proj);
    cgns_render_set_view(ctx, view);

    /* Set blend mode to modulate */
    cgns_render_set_texture_blend_mode(ctx, CGNS_TEX_BLEND_MODULATE);
    cgns_render_bind_texture(ctx, tex, 0);

    /* Draw quad with red color (should modulate with texture) */
    cgns_render_begin(ctx, CGNS_PRIM_QUADS);
    cgns_render_color4fv(ctx, RED);

    cgns_render_texcoord2f(ctx, 0.0f, 0.0f);
    cgns_render_vertex3f(ctx, -1.0f, -1.0f, 0.0f);

    cgns_render_texcoord2f(ctx, 1.0f, 0.0f);
    cgns_render_vertex3f(ctx, 1.0f, -1.0f, 0.0f);

    cgns_render_texcoord2f(ctx, 1.0f, 1.0f);
    cgns_render_vertex3f(ctx, 1.0f, 1.0f, 0.0f);

    cgns_render_texcoord2f(ctx, 0.0f, 1.0f);
    cgns_render_vertex3f(ctx, -1.0f, 1.0f, 0.0f);

    cgns_render_end(ctx);
    cgns_render_frame(ctx);

    /* Cleanup */
    cgns_render_bind_texture(ctx, 0, 0);
    cgns_render_delete_texture(ctx, tex);

    TEST_PASS();
    return 1;
}

/*
 * Test 8: Blend mode - Replace
 */
static int test_blend_mode_replace(cgns_render_context_t* ctx)
{
    TEST_START("Blend mode: Replace");

    unsigned char* data = generate_gradient_rgb(TEST_WIDTH, TEST_HEIGHT);
    ASSERT(data != NULL, "Failed to generate texture");

    unsigned int tex = cgns_render_create_texture(ctx, TEST_WIDTH, TEST_HEIGHT,
                                                    CGNS_TEX_FORMAT_RGB, data);
    free(data);
    ASSERT(tex != 0, "Failed to create texture");

    /* Setup projection */
    float proj[16], view[16];
    cgns_render_matrix_ortho(proj, -2.0f, 2.0f, -2.0f, 2.0f, -1.0f, 1.0f);
    cgns_render_matrix_identity(view);
    cgns_render_set_projection(ctx, proj);
    cgns_render_set_view(ctx, view);

    /* Set blend mode to replace */
    cgns_render_set_texture_blend_mode(ctx, CGNS_TEX_BLEND_REPLACE);
    cgns_render_bind_texture(ctx, tex, 0);

    /* Draw quad (vertex color should be ignored) */
    cgns_render_begin(ctx, CGNS_PRIM_QUADS);
    cgns_render_color4fv(ctx, GREEN);  /* This color should be replaced by texture */

    cgns_render_texcoord2f(ctx, 0.0f, 0.0f);
    cgns_render_vertex3f(ctx, -1.0f, -1.0f, 0.0f);

    cgns_render_texcoord2f(ctx, 1.0f, 0.0f);
    cgns_render_vertex3f(ctx, 1.0f, -1.0f, 0.0f);

    cgns_render_texcoord2f(ctx, 1.0f, 1.0f);
    cgns_render_vertex3f(ctx, 1.0f, 1.0f, 0.0f);

    cgns_render_texcoord2f(ctx, 0.0f, 1.0f);
    cgns_render_vertex3f(ctx, -1.0f, 1.0f, 0.0f);

    cgns_render_end(ctx);
    cgns_render_frame(ctx);

    /* Cleanup */
    cgns_render_bind_texture(ctx, 0, 0);
    cgns_render_delete_texture(ctx, tex);

    TEST_PASS();
    return 1;
}

/*
 * Test 9: Blend mode - Decal
 */
static int test_blend_mode_decal(cgns_render_context_t* ctx)
{
    TEST_START("Blend mode: Decal");

    unsigned char* data = generate_checkerboard_rgba(TEST_WIDTH, TEST_HEIGHT, 8);
    ASSERT(data != NULL, "Failed to generate texture");

    unsigned int tex = cgns_render_create_texture(ctx, TEST_WIDTH, TEST_HEIGHT,
                                                    CGNS_TEX_FORMAT_RGBA, data);
    free(data);
    ASSERT(tex != 0, "Failed to create texture");

    /* Setup projection */
    float proj[16], view[16];
    cgns_render_matrix_ortho(proj, -2.0f, 2.0f, -2.0f, 2.0f, -1.0f, 1.0f);
    cgns_render_matrix_identity(view);
    cgns_render_set_projection(ctx, proj);
    cgns_render_set_view(ctx, view);

    /* Set blend mode to decal */
    cgns_render_set_texture_blend_mode(ctx, CGNS_TEX_BLEND_DECAL);
    cgns_render_bind_texture(ctx, tex, 0);

    /* Draw quad (should blend based on texture alpha) */
    cgns_render_begin(ctx, CGNS_PRIM_QUADS);
    cgns_render_color4fv(ctx, YELLOW);

    cgns_render_texcoord2f(ctx, 0.0f, 0.0f);
    cgns_render_vertex3f(ctx, -1.0f, -1.0f, 0.0f);

    cgns_render_texcoord2f(ctx, 1.0f, 0.0f);
    cgns_render_vertex3f(ctx, 1.0f, -1.0f, 0.0f);

    cgns_render_texcoord2f(ctx, 1.0f, 1.0f);
    cgns_render_vertex3f(ctx, 1.0f, 1.0f, 0.0f);

    cgns_render_texcoord2f(ctx, 0.0f, 1.0f);
    cgns_render_vertex3f(ctx, -1.0f, 1.0f, 0.0f);

    cgns_render_end(ctx);
    cgns_render_frame(ctx);

    /* Cleanup */
    cgns_render_bind_texture(ctx, 0, 0);
    cgns_render_delete_texture(ctx, tex);

    TEST_PASS();
    return 1;
}

/*
 * Test 10: Textured and non-textured rendering mixed
 */
static int test_mixed_rendering(cgns_render_context_t* ctx)
{
    TEST_START("Mixed textured and non-textured rendering");

    unsigned char* data = generate_checkerboard_rgb(TEST_WIDTH, TEST_HEIGHT, 8);
    ASSERT(data != NULL, "Failed to generate texture");

    unsigned int tex = cgns_render_create_texture(ctx, TEST_WIDTH, TEST_HEIGHT,
                                                    CGNS_TEX_FORMAT_RGB, data);
    free(data);
    ASSERT(tex != 0, "Failed to create texture");

    /* Setup projection */
    float proj[16], view[16];
    cgns_render_matrix_ortho(proj, -3.0f, 3.0f, -2.0f, 2.0f, -1.0f, 1.0f);
    cgns_render_matrix_identity(view);
    cgns_render_set_projection(ctx, proj);
    cgns_render_set_view(ctx, view);

    /* Draw textured quad on left */
    cgns_render_bind_texture(ctx, tex, 0);
    cgns_render_begin(ctx, CGNS_PRIM_QUADS);
    cgns_render_color4fv(ctx, WHITE);

    cgns_render_texcoord2f(ctx, 0.0f, 0.0f);
    cgns_render_vertex3f(ctx, -2.5f, -1.0f, 0.0f);

    cgns_render_texcoord2f(ctx, 1.0f, 0.0f);
    cgns_render_vertex3f(ctx, -1.0f, -1.0f, 0.0f);

    cgns_render_texcoord2f(ctx, 1.0f, 1.0f);
    cgns_render_vertex3f(ctx, -1.0f, 1.0f, 0.0f);

    cgns_render_texcoord2f(ctx, 0.0f, 1.0f);
    cgns_render_vertex3f(ctx, -2.5f, 1.0f, 0.0f);

    cgns_render_end(ctx);

    /* Draw non-textured quad on right */
    cgns_render_bind_texture(ctx, 0, 0);  /* Unbind texture */
    cgns_render_begin(ctx, CGNS_PRIM_QUADS);
    cgns_render_color4fv(ctx, BLUE);

    cgns_render_vertex3f(ctx, 1.0f, -1.0f, 0.0f);
    cgns_render_vertex3f(ctx, 2.5f, -1.0f, 0.0f);
    cgns_render_vertex3f(ctx, 2.5f, 1.0f, 0.0f);
    cgns_render_vertex3f(ctx, 1.0f, 1.0f, 0.0f);

    cgns_render_end(ctx);

    cgns_render_frame(ctx);

    /* Cleanup */
    cgns_render_delete_texture(ctx, tex);

    TEST_PASS();
    return 1;
}

/*
 * Test 11: Batch rendering with textures
 */
static int test_batch_rendering_textured(cgns_render_context_t* ctx)
{
    TEST_START("Batch rendering with textures");

    unsigned char* data = generate_checkerboard_rgb(TEST_WIDTH, TEST_HEIGHT, 8);
    ASSERT(data != NULL, "Failed to generate texture");

    unsigned int tex = cgns_render_create_texture(ctx, TEST_WIDTH, TEST_HEIGHT,
                                                    CGNS_TEX_FORMAT_RGB, data);
    free(data);
    ASSERT(tex != 0, "Failed to create texture");

    /* Setup projection */
    float proj[16], view[16];
    cgns_render_matrix_ortho(proj, -2.0f, 2.0f, -2.0f, 2.0f, -1.0f, 1.0f);
    cgns_render_matrix_identity(view);
    cgns_render_set_projection(ctx, proj);
    cgns_render_set_view(ctx, view);

    /* Create vertex array for textured quad */
    cgns_vertex_t vertices[4];

    /* Bottom-left */
    vertices[0].position[0] = -1.0f;
    vertices[0].position[1] = -1.0f;
    vertices[0].position[2] = 0.0f;
    vertices[0].color[0] = 1.0f;
    vertices[0].color[1] = 1.0f;
    vertices[0].color[2] = 1.0f;
    vertices[0].color[3] = 1.0f;
    vertices[0].texcoord[0] = 0.0f;
    vertices[0].texcoord[1] = 0.0f;

    /* Bottom-right */
    vertices[1].position[0] = 1.0f;
    vertices[1].position[1] = -1.0f;
    vertices[1].position[2] = 0.0f;
    vertices[1].color[0] = 1.0f;
    vertices[1].color[1] = 1.0f;
    vertices[1].color[2] = 1.0f;
    vertices[1].color[3] = 1.0f;
    vertices[1].texcoord[0] = 1.0f;
    vertices[1].texcoord[1] = 0.0f;

    /* Top-right */
    vertices[2].position[0] = 1.0f;
    vertices[2].position[1] = 1.0f;
    vertices[2].position[2] = 0.0f;
    vertices[2].color[0] = 1.0f;
    vertices[2].color[1] = 1.0f;
    vertices[2].color[2] = 1.0f;
    vertices[2].color[3] = 1.0f;
    vertices[2].texcoord[0] = 1.0f;
    vertices[2].texcoord[1] = 1.0f;

    /* Top-left */
    vertices[3].position[0] = -1.0f;
    vertices[3].position[1] = 1.0f;
    vertices[3].position[2] = 0.0f;
    vertices[3].color[0] = 1.0f;
    vertices[3].color[1] = 1.0f;
    vertices[3].color[2] = 1.0f;
    vertices[3].color[3] = 1.0f;
    vertices[3].texcoord[0] = 0.0f;
    vertices[3].texcoord[1] = 1.0f;

    /* Set normal for all vertices */
    for (int i = 0; i < 4; i++) {
        vertices[i].normal[0] = 0.0f;
        vertices[i].normal[1] = 0.0f;
        vertices[i].normal[2] = 1.0f;
    }

    /* Bind texture and draw using batch rendering */
    cgns_render_bind_texture(ctx, tex, 0);
    cgns_render_draw_batch(ctx, CGNS_PRIM_QUADS, vertices, 4);
    cgns_render_frame(ctx);

    /* Cleanup */
    cgns_render_bind_texture(ctx, 0, 0);
    cgns_render_delete_texture(ctx, tex);

    TEST_PASS();
    return 1;
}

/*
 * Test 12: Display lists with textures
 */
static int test_display_list_textured(cgns_render_context_t* ctx)
{
    TEST_START("Display lists with textures");

    unsigned char* data = generate_gradient_rgb(TEST_WIDTH, TEST_HEIGHT);
    ASSERT(data != NULL, "Failed to generate texture");

    unsigned int tex = cgns_render_create_texture(ctx, TEST_WIDTH, TEST_HEIGHT,
                                                    CGNS_TEX_FORMAT_RGB, data);
    free(data);
    ASSERT(tex != 0, "Failed to create texture");

    /* Setup projection */
    float proj[16], view[16];
    cgns_render_matrix_ortho(proj, -2.0f, 2.0f, -2.0f, 2.0f, -1.0f, 1.0f);
    cgns_render_matrix_identity(view);
    cgns_render_set_projection(ctx, proj);
    cgns_render_set_view(ctx, view);

    /* Generate display list ID */
    unsigned int list = cgns_render_gen_list(ctx);

    /* Record textured quad to display list */
    cgns_render_bind_texture(ctx, tex, 0);
    cgns_render_set_texture_blend_mode(ctx, CGNS_TEX_BLEND_REPLACE);

    cgns_render_new_list(ctx, list);

    cgns_render_begin(ctx, CGNS_PRIM_QUADS);
    cgns_render_color4fv(ctx, WHITE);

    cgns_render_texcoord2f(ctx, 0.0f, 0.0f);
    cgns_render_vertex3f(ctx, -1.0f, -1.0f, 0.0f);

    cgns_render_texcoord2f(ctx, 1.0f, 0.0f);
    cgns_render_vertex3f(ctx, 1.0f, -1.0f, 0.0f);

    cgns_render_texcoord2f(ctx, 1.0f, 1.0f);
    cgns_render_vertex3f(ctx, 1.0f, 1.0f, 0.0f);

    cgns_render_texcoord2f(ctx, 0.0f, 1.0f);
    cgns_render_vertex3f(ctx, -1.0f, 1.0f, 0.0f);

    cgns_render_end(ctx);

    cgns_render_end_list(ctx);

    /* Unbind texture and change blend mode */
    cgns_render_bind_texture(ctx, 0, 0);
    cgns_render_set_texture_blend_mode(ctx, CGNS_TEX_BLEND_MODULATE);

    /* Call display list - should restore texture state */
    cgns_render_call_list(ctx, list);
    cgns_render_frame(ctx);

    /* Cleanup */
    cgns_render_delete_list(ctx, list);
    cgns_render_delete_texture(ctx, tex);

    TEST_PASS();
    return 1;
}

/*
 * Main test runner
 */
int main(int argc, char** argv)
{
    printf("\n");
    printf("========================================\n");
    printf("CGNS bgfx Texture Tests (Phase 3)\n");
    printf("========================================\n\n");

    /* Set backend to bgfx */
    if (cgns_render_set_backend(CGNS_RENDER_BACKEND_BGFX) != 0) {
        printf("FATAL: Failed to set bgfx backend\n");
        return 1;
    }

    /* Initialize render context (headless mode for testing) */
    cgns_render_context_t* ctx = cgns_render_initialize(NULL);
    if (!ctx) {
        printf("FATAL: Failed to initialize render context\n");
        return 1;
    }

    printf("Render context initialized successfully\n\n");
    printf("Running texture tests:\n");
    printf("----------------------------------------\n");

    /* Run all tests */
    test_texture_create_delete_rgb(ctx);
    test_texture_create_delete_rgba(ctx);
    test_texture_create_delete_luminance(ctx);
    test_multiple_textures(ctx);
    test_texture_binding(ctx);
    test_render_textured_quad(ctx);
    test_blend_mode_modulate(ctx);
    test_blend_mode_replace(ctx);
    test_blend_mode_decal(ctx);
    test_mixed_rendering(ctx);
    test_batch_rendering_textured(ctx);         /* Phase 3 Session 5 */
    test_display_list_textured(ctx);            /* Phase 3 Session 5 */

    /* Cleanup */
    cgns_render_shutdown(ctx);

    /* Print results */
    printf("----------------------------------------\n");
    printf("\nTest Results:\n");
    printf("  Total:  %d\n", tests_run);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("\n");

    if (tests_failed == 0) {
        printf("ALL TESTS PASSED!\n\n");
        return 0;
    } else {
        printf("SOME TESTS FAILED!\n\n");
        return 1;
    }
}
