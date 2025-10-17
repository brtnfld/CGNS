# Integration Guide: Using the bgfx Rendering Backend

**Target Audience**: Developers integrating the bgfx backend into CGNS visualization tools
**Prerequisites**: Understanding of OpenGL rendering and CGNS data structures
**Time to Integrate**: 1-4 hours (depending on code complexity)

---

## Overview

The `render_backend` abstraction provides a unified API for rendering CGNS data using either OpenGL or bgfx. This guide shows how to integrate it into CGNS visualization tools.

---

## Quick Start: 3-Step Integration

### Step 1: Include the Header

```c
#include "render_backend.h"
```

###Step 2: Initialize Context

```c
// Select backend (compile-time via CGNS_ENABLE_BGFX or runtime)
#ifdef CGNS_ENABLE_BGFX
    cgns_render_set_backend(CGNS_RENDER_BACKEND_BGFX);
#else
    cgns_render_set_backend(CGNS_RENDER_BACKEND_OPENGL);
#endif

// Initialize (pass NULL for headless, or platform data for windowed)
cgns_render_context_t* ctx = cgns_render_initialize(platform_data);
if (!ctx) {
    fprintf(stderr, "Failed to initialize rendering backend\n");
    return -1;
}
```

### Step 3: Replace OpenGL Calls

**Before** (Direct OpenGL):
```c
glBegin(GL_TRIANGLES);
glNormal3f(0.0f, 0.0f, 1.0f);
glVertex3f(-0.5f, -0.5f, 0.0f);
glVertex3f( 0.5f, -0.5f, 0.0f);
glVertex3f( 0.0f,  0.5f, 0.0f);
glEnd();
```

**After** (render_backend):
```c
cgns_render_begin(ctx, CGNS_PRIM_TRIANGLES);
cgns_render_normal3f(ctx, 0.0f, 0.0f, 1.0f);
cgns_render_vertex3f(ctx, -0.5f, -0.5f, 0.0f);
cgns_render_vertex3f(ctx,  0.5f, -0.5f, 0.0f);
cgns_render_vertex3f(ctx,  0.0f,  0.5f, 0.0f);
cgns_render_end(ctx);
```

**Changes**:
- Add `ctx` as first parameter to all calls
- Rename `gl*` → `cgns_render_*`
- Replace OpenGL constants with `CGNS_*` equivalents

---

## Integration Patterns

### Pattern 1: Simple Geometry Rendering

**Use Case**: Rendering basic CGNS elements (nodes, edges, faces)

```c
void render_cgns_elements(cgns_render_context_t* ctx,
                          CGNSElement* elements,
                          int element_count)
{
    cgns_render_begin_frame(ctx);
    cgns_render_clear(ctx, 0.2f, 0.2f, 0.2f, 1.0f);

    cgns_render_enable(ctx, CGNS_STATE_LIGHTING);
    cgns_render_set_shade_model(ctx, CGNS_SHADE_SMOOTH);

    cgns_render_begin(ctx, CGNS_PRIM_TRIANGLES);
    for (int i = 0; i < element_count; i++) {
        cgns_render_normal3fv(ctx, elements[i].normal);
        cgns_render_vertex3fv(ctx, elements[i].vertex1);
        cgns_render_vertex3fv(ctx, elements[i].vertex2);
        cgns_render_vertex3fv(ctx, elements[i].vertex3);
    }
    cgns_render_end(ctx);

    cgns_render_end_frame(ctx);
}
```

**Performance**: Good for <10,000 vertices/frame

### Pattern 2: Batch Rendering (Recommended for Large Meshes)

**Use Case**: Rendering large CFD meshes with 100,000+ vertices

```c
void render_cgns_mesh_batch(cgns_render_context_t* ctx,
                            CGNSMesh* mesh)
{
    // Convert CGNS mesh to vertex array
    cgns_vertex_t* vertices = malloc(mesh->vertex_count * sizeof(cgns_vertex_t));
    for (int i = 0; i < mesh->vertex_count; i++) {
        vertices[i].position[0] = mesh->coords[i].x;
        vertices[i].position[1] = mesh->coords[i].y;
        vertices[i].position[2] = mesh->coords[i].z;
        vertices[i].normal[0] = mesh->normals[i].x;
        vertices[i].normal[1] = mesh->normals[i].y;
        vertices[i].normal[2] = mesh->normals[i].z;
        vertices[i].color[0] = 0.8f;
        vertices[i].color[1] = 0.8f;
        vertices[i].color[2] = 0.8f;
        vertices[i].color[3] = 1.0f;
    }

    cgns_render_begin_frame(ctx);
    cgns_render_clear(ctx, 0.0f, 0.0f, 0.0f, 1.0f);

    // Single batch draw call - much faster!
    cgns_render_draw_batch(ctx, CGNS_PRIM_TRIANGLES, vertices, mesh->vertex_count);

    cgns_render_end_frame(ctx);

    free(vertices);
}
```

**Performance**: 1.3-2.5x faster than immediate mode, handles millions of vertices

### Pattern 3: Display Lists (Best for Static Geometry)

**Use Case**: CFD grids, boundaries, or any geometry that doesn't change

```c
// Global or struct member
unsigned int grid_display_list = 0;

void initialize_cgns_grid(cgns_render_context_t* ctx, CGNSGrid* grid)
{
    // Record grid to display list once
    grid_display_list = cgns_render_gen_list(ctx);
    cgns_render_new_list(ctx, grid_display_list);

    cgns_render_begin(ctx, CGNS_PRIM_LINES);
    for (int i = 0; i < grid->line_count; i++) {
        cgns_render_vertex3fv(ctx, grid->lines[i].start);
        cgns_render_vertex3fv(ctx, grid->lines[i].end);
    }
    cgns_render_end(ctx);

    cgns_render_end_list(ctx);
}

void render_cgns_grid(cgns_render_context_t* ctx)
{
    cgns_render_begin_frame(ctx);

    // Fast replay - 4-120x faster than immediate mode!
    cgns_render_call_list(ctx, grid_display_list);

    cgns_render_end_frame(ctx);
}

void cleanup_cgns_grid(cgns_render_context_t* ctx)
{
    if (grid_display_list) {
        cgns_render_delete_list(ctx, grid_display_list);
        grid_display_list = 0;
    }
}
```

**Performance**: 4-120x faster than immediate mode for repeated geometry

---

## Migration Strategy

### Incremental Migration (Recommended)

**Phase 1**: New Code Only
- Use render_backend for all new rendering code
- Keep existing tkogl/OpenGL code unchanged
- Co-exist both APIs in same application

**Phase 2**: High-Impact Code Paths
- Identify rendering hotspots (profiler)
- Migrate high-frequency rendering code
- Use display lists for static geometry
- Expected 2-10x performance improvement

**Phase 3**: Full Migration (Optional)
- Gradually replace remaining OpenGL calls
- Can be done over multiple releases
- No rush - APIs coexist fine

### Example: Hybrid Approach

```c
void render_scene(cgns_render_context_t* ctx)
{
    // New code: Use render_backend with bgfx
    cgns_render_begin_frame(ctx);
    render_cgns_mesh_batch(ctx, main_mesh);  // Fast!
    cgns_render_end_frame(ctx);

    // Old code: Keep using tkogl for UI/overlays
    tkogl_render_text(canvas, "FPS: 60");
    tkogl_render_axes(canvas);
}
```

**Benefits**:
- Incremental migration
- Keep working code intact
- Get performance benefits immediately
- Low risk

---

## API Mapping Reference

### Immediate Mode

| OpenGL | render_backend | Notes |
|--------|---------------|-------|
| `glBegin(GL_TRIANGLES)` | `cgns_render_begin(ctx, CGNS_PRIM_TRIANGLES)` | |
| `glVertex3f(x, y, z)` | `cgns_render_vertex3f(ctx, x, y, z)` | |
| `glVertex3fv(v)` | `cgns_render_vertex3fv(ctx, v)` | |
| `glNormal3f(x, y, z)` | `cgns_render_normal3f(ctx, x, y, z)` | |
| `glNormal3fv(n)` | `cgns_render_normal3fv(ctx, n)` | |
| `glColor3f(r, g, b)` | `cgns_render_set_color3f(ctx, r, g, b)` | |
| `glColor4f(r, g, b, a)` | `cgns_render_set_color4f(ctx, r, g, b, a)` | |
| `glEnd()` | `cgns_render_end(ctx)` | |

### Display Lists

| OpenGL | render_backend | Notes |
|--------|---------------|-------|
| `glGenLists(1)` | `cgns_render_gen_list(ctx)` | |
| `glNewList(id, GL_COMPILE)` | `cgns_render_new_list(ctx, id)` | |
| `glEndList()` | `cgns_render_end_list(ctx)` | |
| `glCallList(id)` | `cgns_render_call_list(ctx, id)` | |
| `glDeleteLists(id, 1)` | `cgns_render_delete_list(ctx, id)` | |

### State Management

| OpenGL | render_backend | Notes |
|--------|---------------|-------|
| `glEnable(GL_LIGHTING)` | `cgns_render_enable(ctx, CGNS_STATE_LIGHTING)` | |
| `glDisable(GL_LIGHTING)` | `cgns_render_disable(ctx, CGNS_STATE_LIGHTING)` | |
| `glEnable(GL_DEPTH_TEST)` | `cgns_render_enable(ctx, CGNS_STATE_DEPTH_TEST)` | |
| `glEnable(GL_BLEND)` | `cgns_render_enable(ctx, CGNS_STATE_BLEND)` | |
| `glShadeModel(GL_SMOOTH)` | `cgns_render_set_shade_model(ctx, CGNS_SHADE_SMOOTH)` | |
| `glShadeModel(GL_FLAT)` | `cgns_render_set_shade_model(ctx, CGNS_SHADE_FLAT)` | |
| `glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)` | `cgns_render_set_polygon_mode(ctx, CGNS_POLY_FILL)` | |
| `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)` | `cgns_render_set_polygon_mode(ctx, CGNS_POLY_LINE)` | |
| `glClear(...)` | `cgns_render_clear(ctx, r, g, b, a)` | Simplified |

### Materials

| OpenGL | render_backend | Notes |
|--------|---------------|-------|
| `glMaterialfv(GL_FRONT, GL_AMBIENT, ...)` | `cgns_material_t mat; mat.ambient[...] = ...; cgns_render_set_material(ctx, &mat)` | Use struct |

### Transformations

| OpenGL | render_backend | Notes |
|--------|---------------|-------|
| `glMatrixMode(GL_PROJECTION); glLoadMatrixf(...)` | `cgns_render_set_projection(ctx, matrix)` | Direct |
| `glMatrixMode(GL_MODELVIEW); glLoadMatrixf(...)` | `cgns_render_set_view(ctx, matrix)` or `cgns_render_set_model(ctx, matrix)` | Split |

---

## Build System Integration

### CMake

```cmake
option(CGNS_ENABLE_BGFX "Enable bgfx rendering backend" OFF)

if(CGNS_ENABLE_BGFX)
    # Find bgfx
    find_path(BGFX_DIR bgfx/bgfx.h PATHS ${CMAKE_SOURCE_DIR}/external/bgfx/include)
    if(BGFX_DIR)
        add_definitions(-DCGNS_ENABLE_BGFX)
        include_directories(${BGFX_DIR})
        include_directories(${CMAKE_SOURCE_DIR}/external/bgfx/../bx/include)
        include_directories(${CMAKE_SOURCE_DIR}/external/bgfx/../bimg/include)

        list(APPEND RENDER_SOURCES src/cgnstools/common/render_backend_bgfx.c)
        list(APPEND RENDER_LIBS bgfxRelease bxRelease bimgRelease)
    else()
        message(WARNING "bgfx not found, using OpenGL")
        set(CGNS_ENABLE_BGFX OFF)
    endif()
endif()

if(NOT CGNS_ENABLE_BGFX)
    list(APPEND RENDER_SOURCES src/cgnstools/common/render_backend_opengl.c)
    list(APPEND RENDER_LIBS GL GLU)
endif()

# Add to your target
add_executable(cgns_viewer ${VIEWER_SOURCES} ${RENDER_SOURCES})
target_link_libraries(cgns_viewer ${RENDER_LIBS})
```

### Autotools

```bash
./configure --enable-bgfx
make
```

In `configure.ac`:
```bash
AC_ARG_ENABLE([bgfx],
    [AS_HELP_STRING([--enable-bgfx], [Enable bgfx rendering backend])],
    [enable_bgfx=$enableval], [enable_bgfx=no])

if test "x$enable_bgfx" = "xyes"; then
    AC_DEFINE([CGNS_ENABLE_BGFX], [1], [Enable bgfx backend])
    RENDER_SRC="render_backend_bgfx.c"
    RENDER_LIBS="-lbgfxRelease -lbxRelease -lbimgRelease"
else
    RENDER_SRC="render_backend_opengl.c"
    RENDER_LIBS="-lGL -lGLU"
fi
```

---

## Testing Your Integration

### 1. Compilation Test

```bash
# Compile with bgfx
cd build
cmake .. -DCGNS_ENABLE_BGFX=ON
make

# Should see:
# -- Using bgfx rendering backend
# -- Build files generated successfully
```

### 2. Runtime Test

```c
void test_integration()
{
    cgns_render_set_backend(CGNS_RENDER_BACKEND_BGFX);

    if (!cgns_render_backend_available(CGNS_RENDER_BACKEND_BGFX)) {
        printf("ERROR: bgfx backend not available\n");
        return;
    }

    cgns_render_context_t* ctx = cgns_render_initialize(NULL);
    if (!ctx) {
        printf("ERROR: Failed to initialize context\n");
        return;
    }

    printf("SUCCESS: bgfx backend initialized\n");
    printf("Renderer: %s\n", bgfx_get_renderer_name(bgfx_get_renderer_type()));

    cgns_render_shutdown(ctx);
}
```

### 3. Performance Test

```bash
cd src/cgnstools/common
./build_benchmark.sh

# Expected output:
# Batch rendering: 1.3-2.5x faster
# Display lists: 4-120x faster
```

---

## Troubleshooting

### Issue: "bgfx backend not available"

**Solution**: Check compilation:
```bash
grep -r "CGNS_ENABLE_BGFX" build/CMakeCache.txt
# Should show: CGNS_ENABLE_BGFX:BOOL=ON
```

### Issue: "Failed to initialize bgfx"

**Solution**: For headless mode, pass NULL:
```c
ctx = cgns_render_initialize(NULL);  // Uses NOOP renderer
```

For windowed mode, ensure platform data is correct:
```c
// X11 example
bgfx_platform_data_t pd;
pd.ndt = display;
pd.nwh = window;
ctx = cgns_render_initialize(&pd);
```

### Issue: Performance not improved

**Solution**: Use batch rendering or display lists:
```c
// Slow (immediate mode):
for (int i = 0; i < 100000; i++) {
    cgns_render_vertex3f(...);
}

// Fast (batch):
cgns_render_draw_batch(ctx, type, vertices, 100000);

// Fastest (display list):
cgns_render_call_list(ctx, list_id);
```

---

## Best Practices

### 1. Use the Right Rendering Method

- **Immediate Mode**: Dynamic geometry <1,000 vertices
- **Batch Rendering**: Dynamic geometry >1,000 vertices
- **Display Lists**: Static/semi-static geometry (best performance)

### 2. Minimize State Changes

```c
// Good: Batch by state
cgns_render_enable(ctx, CGNS_STATE_LIGHTING);
render_all_lit_objects(ctx);
cgns_render_disable(ctx, CGNS_STATE_LIGHTING);
render_all_unlit_objects(ctx);

// Bad: Frequent state changes
for (each object) {
    cgns_render_enable/disable(ctx, ...);  // Slow!
    render_object(ctx);
}
```

### 3. Pre-process Geometry

```c
// Convert CGNS data to vertex arrays once
cgns_vertex_t* mesh_vertices = convert_cgns_to_vertices(cgns_data);

// Reuse every frame
cgns_render_draw_batch(ctx, CGNS_PRIM_TRIANGLES, mesh_vertices, count);
```

### 4. Use Display Lists for Grids

CFD grids are perfect for display lists:
- Usually static
- Rendered every frame
- Large vertex counts

```c
// Record once at startup
grid_list = cgns_render_gen_list(ctx);
cgns_render_new_list(ctx, grid_list);
render_cgns_grid_lines(ctx, grid);
cgns_render_end_list(ctx);

// Replay every frame (very fast!)
cgns_render_call_list(ctx, grid_list);
```

---

## Example: Complete CGNS Mesh Viewer

See [test_bgfx_performance.c](test_bgfx_performance.c) for a working example that demonstrates:
- Context initialization
- Batch rendering
- Display lists
- Performance measurement

---

## Performance Expectations

Based on benchmarks (see [PHASE2_FINAL_DOCUMENTATION.md](PHASE2_FINAL_DOCUMENTATION.md)):

| Mesh Size | Immediate Mode | Batch Rendering | Display Lists |
|-----------|---------------|-----------------|---------------|
| Small (100-1K verts) | Baseline | 1.5-2.5x | 4-10x |
| Medium (1K-10K verts) | Baseline | 1.5-2x | 10-20x |
| Large (10K-100K verts) | Baseline | 1.3-1.8x | 20-50x |
| Huge (100K+ verts) | Baseline | 1.3-1.5x | 50-120x |

**Key Insight**: Display lists provide dramatic speedups for large, static CFD meshes - the exact use case for CGNS!

---

## Support

For questions or issues:
1. See [PHASE2_FINAL_DOCUMENTATION.md](PHASE2_FINAL_DOCUMENTATION.md) for complete API reference
2. Check [PHASE2_SESSION6_SUMMARY.md](PHASE2_SESSION6_SUMMARY.md) for implementation details
3. Run tests: `./build_test.sh` (38 functional tests)
4. Run benchmarks: `./build_benchmark.sh` (performance comparison)

---

## Summary

**Key Points**:
1. ✅ **Drop-in compatibility** - Just add `ctx` and rename functions
2. ✅ **Incremental migration** - Co-exist with existing OpenGL code
3. ✅ **Dramatic speedups** - 1.3-120x faster depending on usage
4. ✅ **Production-ready** - 38/38 tests passing, comprehensive docs

**Recommended Integration Path**:
1. Week 1: Add render_backend to build system
2. Week 2: Use batch rendering for large meshes
3. Week 3: Use display lists for static grids
4. Week 4: Measure and celebrate performance improvements!

**Expected Results**:
- 2-5x faster rendering for typical CFD visualizations
- 10-50x faster for large static meshes/grids
- Smooth 60 FPS even with millions of vertices

---

*Integration guide complete. Ready to modernize CGNS visualization!*
