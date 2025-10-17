/**
 * Performance benchmark for bgfx rendering backend
 *
 * Tests rendering performance for various scenarios:
 * - Immediate mode vs batch rendering
 * - Display lists vs direct rendering
 * - Different primitive types
 * - Various vertex counts
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "render_backend.h"

/* ========================================================================
 * Timing Utilities
 * ======================================================================== */

static double get_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0;
}

/* ========================================================================
 * Benchmark Utilities
 * ======================================================================== */

static void generate_cube_vertices(cgns_vertex_t* vertices, size_t* count,
                                   float x, float y, float z, float size)
{
    const float s = size * 0.5f;
    const float positions[8][3] = {
        {x-s, y-s, z-s}, {x+s, y-s, z-s}, {x+s, y+s, z-s}, {x-s, y+s, z-s},
        {x-s, y-s, z+s}, {x+s, y-s, z+s}, {x+s, y+s, z+s}, {x-s, y+s, z+s}
    };

    const int faces[6][4] = {
        {0,1,2,3}, {4,5,6,7}, {0,1,5,4}, {2,3,7,6}, {0,3,7,4}, {1,2,6,5}
    };

    const float normals[6][3] = {
        {0,0,-1}, {0,0,1}, {0,-1,0}, {0,1,0}, {-1,0,0}, {1,0,0}
    };

    size_t idx = 0;
    for (int face = 0; face < 6; face++) {
        for (int v = 0; v < 4; v++) {
            int pos_idx = faces[face][v];
            vertices[idx].position[0] = positions[pos_idx][0];
            vertices[idx].position[1] = positions[pos_idx][1];
            vertices[idx].position[2] = positions[pos_idx][2];
            vertices[idx].normal[0] = normals[face][0];
            vertices[idx].normal[1] = normals[face][1];
            vertices[idx].normal[2] = normals[face][2];
            vertices[idx].color[0] = 0.8f;
            vertices[idx].color[1] = 0.8f;
            vertices[idx].color[2] = 0.8f;
            vertices[idx].color[3] = 1.0f;
            idx++;
        }
    }
    *count = idx;
}

/* ========================================================================
 * Benchmark 1: Immediate Mode Rendering
 * ======================================================================== */

static double benchmark_immediate_mode(cgns_render_context_t* ctx, int cube_count)
{
    double start, end;
    cgns_vertex_t cube_verts[24];
    size_t vert_count;

    printf("  Testing %d cubes with immediate mode...\n", cube_count);

    start = get_time_ms();

    cgns_render_begin_frame(ctx);
    cgns_render_clear(ctx, 0.2f, 0.2f, 0.2f, 1.0f);

    for (int i = 0; i < cube_count; i++) {
        float x = (i % 10) * 2.0f;
        float y = ((i / 10) % 10) * 2.0f;
        float z = (i / 100) * 2.0f;

        generate_cube_vertices(cube_verts, &vert_count, x, y, z, 1.0f);

        /* Render as quads using immediate mode */
        for (size_t face = 0; face < 6; face++) {
            cgns_render_begin(ctx, CGNS_PRIM_QUADS);
            for (int v = 0; v < 4; v++) {
                size_t idx = face * 4 + v;
                cgns_render_normal3f(ctx, cube_verts[idx].normal[0],
                                         cube_verts[idx].normal[1],
                                         cube_verts[idx].normal[2]);
                cgns_render_vertex3f(ctx, cube_verts[idx].position[0],
                                         cube_verts[idx].position[1],
                                         cube_verts[idx].position[2]);
            }
            cgns_render_end(ctx);
        }
    }

    cgns_render_end_frame(ctx);

    end = get_time_ms();

    double elapsed = end - start;
    printf("  Completed in %.2f ms\n", elapsed);
    printf("  Vertices rendered: %d\n", cube_count * 24);
    printf("  Vertices/sec: %.0f\n", (cube_count * 24) / (elapsed / 1000.0));

    return elapsed;
}

/* ========================================================================
 * Benchmark 2: Batch Rendering
 * ======================================================================== */

static double benchmark_batch_rendering(cgns_render_context_t* ctx, int cube_count)
{
    double start, end;

    printf("  Testing %d cubes with batch rendering...\n", cube_count);

    /* Allocate batch vertex buffer */
    cgns_vertex_t* batch_verts = (cgns_vertex_t*)malloc(cube_count * 24 * sizeof(cgns_vertex_t));
    if (!batch_verts) {
        printf("  ERROR: Failed to allocate batch buffer\n");
        return 0.0;
    }

    /* Generate all vertices */
    size_t total_verts = 0;
    for (int i = 0; i < cube_count; i++) {
        float x = (i % 10) * 2.0f;
        float y = ((i / 10) % 10) * 2.0f;
        float z = (i / 100) * 2.0f;

        size_t vert_count;
        generate_cube_vertices(&batch_verts[total_verts], &vert_count, x, y, z, 1.0f);
        total_verts += vert_count;
    }

    start = get_time_ms();

    cgns_render_begin_frame(ctx);
    cgns_render_clear(ctx, 0.2f, 0.2f, 0.2f, 1.0f);

    /* Single batch draw call */
    cgns_render_draw_batch(ctx, CGNS_PRIM_QUADS, batch_verts, total_verts);

    cgns_render_end_frame(ctx);

    end = get_time_ms();

    free(batch_verts);

    double elapsed = end - start;
    printf("  Completed in %.2f ms\n", elapsed);
    printf("  Vertices rendered: %zu\n", total_verts);
    printf("  Vertices/sec: %.0f\n", total_verts / (elapsed / 1000.0));

    return elapsed;
}

/* ========================================================================
 * Benchmark 3: Display Lists
 * ======================================================================== */

static double benchmark_display_lists(cgns_render_context_t* ctx, int cube_count, int replay_count)
{
    double start, end;
    cgns_vertex_t cube_verts[24];
    size_t vert_count;
    unsigned int list_id;

    printf("  Testing %d cubes with display list (%d replays)...\n", cube_count, replay_count);

    /* Record display list */
    list_id = cgns_render_gen_list(ctx);
    cgns_render_new_list(ctx, list_id);

    for (int i = 0; i < cube_count; i++) {
        float x = (i % 10) * 2.0f;
        float y = ((i / 10) % 10) * 2.0f;
        float z = (i / 100) * 2.0f;

        generate_cube_vertices(cube_verts, &vert_count, x, y, z, 1.0f);

        for (size_t face = 0; face < 6; face++) {
            cgns_render_begin(ctx, CGNS_PRIM_QUADS);
            for (int v = 0; v < 4; v++) {
                size_t idx = face * 4 + v;
                cgns_render_normal3f(ctx, cube_verts[idx].normal[0],
                                         cube_verts[idx].normal[1],
                                         cube_verts[idx].normal[2]);
                cgns_render_vertex3f(ctx, cube_verts[idx].position[0],
                                         cube_verts[idx].position[1],
                                         cube_verts[idx].position[2]);
            }
            cgns_render_end(ctx);
        }
    }

    cgns_render_end_list(ctx);

    /* Replay display list multiple times */
    start = get_time_ms();

    for (int replay = 0; replay < replay_count; replay++) {
        cgns_render_begin_frame(ctx);
        cgns_render_clear(ctx, 0.2f, 0.2f, 0.2f, 1.0f);
        cgns_render_call_list(ctx, list_id);
        cgns_render_end_frame(ctx);
    }

    end = get_time_ms();

    cgns_render_delete_list(ctx, list_id);

    double elapsed = end - start;
    double avg_per_frame = elapsed / replay_count;
    printf("  Completed in %.2f ms (avg %.2f ms/frame)\n", elapsed, avg_per_frame);
    printf("  Vertices per frame: %d\n", cube_count * 24);
    printf("  Vertices/sec: %.0f\n", (cube_count * 24 * replay_count) / (elapsed / 1000.0));

    return avg_per_frame;
}

/* ========================================================================
 * Main Benchmark Suite
 * ======================================================================== */

int main(int argc, char** argv)
{
    cgns_render_context_t* ctx;
    double time_immediate, time_batch, time_display_list;

    (void)argc;
    (void)argv;

    printf("========================================\n");
    printf("bgfx Performance Benchmark Suite\n");
    printf("========================================\n\n");

    /* Check backend availability */
    if (!cgns_render_backend_available(CGNS_RENDER_BACKEND_BGFX)) {
        printf("ERROR: bgfx backend not available\n");
        return 1;
    }

    /* Set backend */
    if (cgns_render_set_backend(CGNS_RENDER_BACKEND_BGFX) != 0) {
        printf("ERROR: Failed to set bgfx backend\n");
        return 1;
    }

    /* Initialize context (headless mode) */
    ctx = cgns_render_initialize(NULL);
    if (!ctx) {
        printf("ERROR: Failed to initialize context\n");
        return 1;
    }

    printf("Backend: %s\n", cgns_render_backend_name(cgns_render_get_backend()));
    printf("Mode: Headless (NOOP renderer)\n");
    printf("\n");

    /* Benchmark 1: Small scene - 10 cubes */
    printf("Benchmark 1: Small Scene (10 cubes)\n");
    printf("----------------------------------------\n");
    time_immediate = benchmark_immediate_mode(ctx, 10);
    time_batch = benchmark_batch_rendering(ctx, 10);
    time_display_list = benchmark_display_lists(ctx, 10, 10);
    printf("  Speedup (batch vs immediate): %.2fx\n", time_immediate / time_batch);
    printf("  Speedup (display list vs immediate): %.2fx\n", time_immediate / time_display_list);
    printf("\n");

    /* Benchmark 2: Medium scene - 100 cubes */
    printf("Benchmark 2: Medium Scene (100 cubes)\n");
    printf("----------------------------------------\n");
    time_immediate = benchmark_immediate_mode(ctx, 100);
    time_batch = benchmark_batch_rendering(ctx, 100);
    time_display_list = benchmark_display_lists(ctx, 100, 10);
    printf("  Speedup (batch vs immediate): %.2fx\n", time_immediate / time_batch);
    printf("  Speedup (display list vs immediate): %.2fx\n", time_immediate / time_display_list);
    printf("\n");

    /* Benchmark 3: Large scene - 1000 cubes */
    printf("Benchmark 3: Large Scene (1000 cubes)\n");
    printf("----------------------------------------\n");
    time_immediate = benchmark_immediate_mode(ctx, 1000);
    time_batch = benchmark_batch_rendering(ctx, 1000);
    time_display_list = benchmark_display_lists(ctx, 1000, 10);
    printf("  Speedup (batch vs immediate): %.2fx\n", time_immediate / time_batch);
    printf("  Speedup (display list vs immediate): %.2fx\n", time_immediate / time_display_list);
    printf("\n");

    /* Cleanup */
    cgns_render_shutdown(ctx);

    printf("========================================\n");
    printf("Benchmark Complete\n");
    printf("========================================\n");

    return 0;
}
