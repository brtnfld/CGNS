# Phase 4 Plan: cgnsplot Tool Integration

**Created**: 2025-10-17
**Phase**: 4 (Tool Integration - cgnsplot)
**Goal**: Migrate cgnsplot from OpenGL to bgfx rendering backend
**Priority**: HIGH - Production Value
**Estimated Duration**: 5-7 days

---

## Executive Summary

Phase 4 will migrate the cgnsplot visualization tool from direct OpenGL calls to the bgfx rendering backend implemented in Phases 2-3. This enables cgnsplot to work on platforms where OpenGL is deprecated or unavailable (macOS Metal, modern Linux Vulkan, Windows DirectX 12).

### Success Criteria

- [ ] cgnsplot compiles with bgfx backend
- [ ] All OpenGL calls replaced with render backend API
- [ ] cgnsplot renders simple CGNS files correctly
- [ ] cgnsplot renders complex CFD datasets correctly
- [ ] Performance is equal or better than OpenGL version
- [ ] All existing cgnsplot features work
- [ ] Backward compatibility maintained (can still use OpenGL)
- [ ] Tested with real CFD datasets

---

## Background

### Current State (Phase 3 Complete)

✅ **Render Backend API**: Complete with 50+ functions
✅ **bgfx Implementation**: Fully functional with textures
✅ **OpenGL Implementation**: Reference implementation exists
✅ **Testing**: 50/50 tests passing (100%)
✅ **Performance**: 1.3-120x speedup, validated
✅ **Documentation**: Complete API reference

### Target Application: cgnsplot

**File**: `src/cgnstools/cgnsplot/cgnstcl.c` (5,455 lines)
**OpenGL Calls**: ~92 immediate mode calls
**Complexity**: Medium
**Dependencies**: Tcl/Tk, tkogl widget

**Key Characteristics**:
- Visualization tool for CGNS CFD datasets
- Renders structured/unstructured grids
- Displays zones, boundaries, elements
- Supports materials, colors, lighting
- Uses display lists for performance
- Tcl/Tk GUI with OpenGL rendering widget

---

## Analysis

### OpenGL Usage in cgnstcl.c

Based on grep analysis: **92 OpenGL calls**

**Immediate Mode Calls** (~70 calls):
- `glBegin()`/`glEnd()` - Primitive submission
- `glVertex3f()`/`glVertex3fv()` - Vertex positions
- `glNormal3f()`/`glNormal3fv()` - Normal vectors
- `glColor3f()`/`glColor3fv()` - Vertex colors

**State Management** (~15 calls):
- `glMaterialfv()` - Material properties
- `glEnable()`/`glDisable()` - Feature toggles
- `glShadeModel()` - Smooth/flat shading
- `glLineWidth()` - Line rendering
- `glPointSize()` - Point rendering

**Display Lists** (~7 calls):
- `glNewList()` - Start recording
- `glEndList()` - End recording
- `glCallList()` - Playback
- `glDeleteLists()` - Cleanup

### Data Structures

From PHASE3_CODE_ANALYSIS.md:

```c
typedef float Node[3];                // 3D vertex position

typedef struct {
    cgsize_t id;
    cgsize_t nodes[2];                // Line segment
} Edge;

typedef struct {
    cgsize_t id;
    int flags;
    int nnodes;
    cgsize_t *nodes;                  // Variable vertex count
    float normal[3];                  // Face normal
} Face;

typedef struct {
    char name[33];
    int type;
    float color[4];                   // RGBA color
    float bbox[3][2];                 // Bounding box
    cgsize_t nedges;
    Edge *edges;
    cgsize_t nfaces;
    Face *faces;
    Node *nodes;                      // Vertex array
} Zone;
```

**Excellent Match**: These structures align perfectly with `cgns_vertex_t` for batch rendering!

---

## Migration Strategy

### Approach 1: Direct Replacement (Recommended)

Replace OpenGL calls with render backend API calls **without** changing program structure.

**Advantages**:
- Minimal code changes
- Easy to verify correctness
- Lower risk of regressions
- Can be done incrementally

**Disadvantages**:
- May not fully utilize batch rendering initially
- Display list performance similar to OpenGL

**Estimated Duration**: 3-4 days

### Approach 2: Optimization + Replacement

Replace OpenGL calls **and** convert to batch rendering for better performance.

**Advantages**:
- Maximum performance (1.5-3x faster)
- Better utilization of bgfx backend
- Production-quality result

**Disadvantages**:
- More code changes
- Higher risk of bugs
- Longer development time
- Requires careful testing

**Estimated Duration**: 5-7 days

### Recommended: Hybrid Approach

**Phase 4.1** (Days 1-3): Direct replacement
- Replace all OpenGL calls 1:1
- Verify functional correctness
- Get cgnsplot working with bgfx

**Phase 4.2** (Days 4-5): Optimization (optional)
- Convert hot paths to batch rendering
- Measure performance improvements
- Validate with complex datasets

**Phase 4.3** (Days 6-7): Polish + Testing
- Test with real CFD datasets
- Fix any edge cases
- Documentation updates

---

## Implementation Plan

### Session 1: Environment Setup & Analysis (4-5 hours)

**Goals**:
1. Build cgnsplot with current OpenGL backend
2. Run cgnsplot with sample CGNS files
3. Catalog all OpenGL function calls
4. Identify rendering patterns
5. Create detailed migration map

**Deliverables**:
- cgnsplot builds successfully
- Catalog of all 92 OpenGL calls with locations
- Migration mapping document
- Test CGNS files identified

**Tasks**:
- [ ] Build cgnsplot: `cd src/cgnstools/cgnsplot && make`
- [ ] Test with sample files in `src/tests/`
- [ ] Grep all OpenGL calls: `grep -n "gl[A-Z]" cgnstcl.c > opengl_calls.txt`
- [ ] Analyze call patterns and group by function
- [ ] Identify display list usage
- [ ] Create PHASE4_SESSION1_SUMMARY.md

### Session 2: Include Headers & Context Setup (3-4 hours)

**Goals**:
1. Add render_backend.h include
2. Initialize render context
3. Replace context creation code
4. Handle platform data (X11, Windows)
5. Compile successfully (no runtime changes yet)

**Deliverables**:
- Modified cgnstcl.c includes render_backend.h
- Render context created at startup
- Compiles without errors
- No functional changes (still using OpenGL)

**Tasks**:
- [ ] Add `#include "render_backend.h"` to cgnstcl.c
- [ ] Find initialization function (likely in tkogl widget)
- [ ] Add `cgns_render_context_t* ctx` field to main structure
- [ ] Initialize context with `cgns_render_initialize()`
- [ ] Test compilation
- [ ] Create PHASE4_SESSION2_SUMMARY.md

### Session 3: Replace Immediate Mode Calls (5-6 hours)

**Goals**:
1. Replace glBegin/glEnd with cgns_render_begin/end
2. Replace glVertex3f/v with cgns_render_vertex3f/v
3. Replace glNormal3f/v with cgns_render_normal3f/v
4. Replace glColor3f/v with cgns_render_set_color4f/v
5. Test basic rendering

**Deliverables**:
- All ~70 immediate mode calls replaced
- cgnsplot compiles successfully
- Basic geometry renders correctly
- Create test cases for verification

**Tasks**:
- [ ] Find all `glBegin()` calls (estimate: ~15 locations)
- [ ] Replace with `cgns_render_begin(ctx, primitive_type)`
- [ ] Map GL_TRIANGLES → CGNS_PRIM_TRIANGLES, etc.
- [ ] Replace all `glVertex3*()` calls (~30 locations)
- [ ] Replace all `glNormal3*()` calls (~15 locations)
- [ ] Replace all `glColor3*()` calls (~10 locations)
- [ ] Handle color3f → color4f conversion (alpha=1.0)
- [ ] Test with simple CGNS file
- [ ] Create PHASE4_SESSION3_SUMMARY.md

### Session 4: Replace State Management (4-5 hours)

**Goals**:
1. Replace glMaterialfv with cgns_render_set_material
2. Replace glEnable/Disable with backend equivalents
3. Replace glShadeModel with cgns_render_set_shade_model
4. Replace glLineWidth, glPointSize (may be unsupported)
5. Test material and lighting

**Deliverables**:
- All ~15 state management calls replaced
- Materials render correctly
- Lighting works as expected
- Document any unsupported features

**Tasks**:
- [ ] Find all `glMaterialfv()` calls
- [ ] Replace with `cgns_render_set_material*()`
- [ ] Find all `glEnable(GL_LIGHTING)` → `cgns_render_enable_lighting()`
- [ ] Find all `glEnable(GL_DEPTH_TEST)` → `cgns_render_enable_depth_test()`
- [ ] Replace `glShadeModel()` with `cgns_render_set_shade_model()`
- [ ] Check for glLineWidth/glPointSize (may need to document as unsupported)
- [ ] Test with lit/shaded CGNS files
- [ ] Create PHASE4_SESSION4_SUMMARY.md

### Session 5: Replace Display Lists (3-4 hours)

**Goals**:
1. Replace glNewList with cgns_render_new_list
2. Replace glEndList with cgns_render_end_list
3. Replace glCallList with cgns_render_call_list
4. Replace glDeleteLists with cgns_render_delete_list
5. Verify performance improvement

**Deliverables**:
- All ~7 display list calls replaced
- Display lists work correctly
- Performance validated (should be faster)
- Rendering correctness verified

**Tasks**:
- [ ] Find all `glNewList()` calls
- [ ] Replace with `cgns_render_new_list(ctx, list_id)`
- [ ] Replace `glEndList()` with `cgns_render_end_list(ctx)`
- [ ] Replace `glCallList()` with `cgns_render_call_list(ctx, list_id)`
- [ ] Replace `glDeleteLists()` with `cgns_render_delete_list(ctx, list_id)`
- [ ] Test display list performance
- [ ] Compare with OpenGL performance (should be faster)
- [ ] Create PHASE4_SESSION5_SUMMARY.md

### Session 6: Matrix and Viewport Management (3-4 hours)

**Goals**:
1. Replace glViewport with cgns_render_set_viewport
2. Replace matrix operations (glMatrixMode, glLoadMatrix, etc.)
3. Handle projection and view matrices
4. Test camera transformations

**Deliverables**:
- Viewport management working
- Camera transformations correct
- Zoom/pan/rotate working
- All matrix operations replaced

**Tasks**:
- [ ] Find `glViewport()` calls
- [ ] Replace with `cgns_render_set_viewport()`
- [ ] Find matrix stack operations
- [ ] Replace with `cgns_render_set_projection()` and `cgns_render_set_view()`
- [ ] May need to compute matrices (OpenGL uses stack, bgfx doesn't)
- [ ] Test camera controls (zoom, pan, rotate)
- [ ] Create PHASE4_SESSION6_SUMMARY.md

### Session 7: Testing with Real Datasets (4-5 hours)

**Goals**:
1. Test with simple structured grid
2. Test with complex unstructured grid
3. Test with multiple zones
4. Test with boundary conditions
5. Verify all features work

**Deliverables**:
- cgnsplot works with simple files
- cgnsplot works with complex files
- All features functional
- Performance meets/exceeds OpenGL
- List of any issues found

**Tasks**:
- [ ] Test with `src/tests/*.cgns` files
- [ ] Test with real CFD datasets (if available)
- [ ] Test all rendering modes (wireframe, shaded, lit)
- [ ] Test all view controls (zoom, pan, rotate, reset)
- [ ] Test zone visibility toggles
- [ ] Test boundary condition display
- [ ] Measure rendering performance
- [ ] Document any issues or limitations
- [ ] Create PHASE4_SESSION7_SUMMARY.md

### Session 8: Backward Compatibility & Build Integration (3-4 hours)

**Goals**:
1. Maintain OpenGL backend option
2. Add compile-time flag (CGNS_USE_BGFX)
3. Update build system (Makefile/CMake)
4. Create example programs
5. Update documentation

**Deliverables**:
- Can build with either OpenGL or bgfx
- Build system updated
- Example programs created
- Documentation complete

**Tasks**:
- [ ] Add `#ifdef CGNS_USE_BGFX` guards
- [ ] Keep OpenGL code path available
- [ ] Update Makefile.in to support both backends
- [ ] Update CMakeLists.txt (if exists)
- [ ] Create example: simple CGNS viewer
- [ ] Update cgnsplot documentation
- [ ] Create PHASE4_SESSION8_SUMMARY.md

### Session 9: Performance Optimization (Optional, 4-5 hours)

**Goals**:
1. Identify rendering hot paths
2. Convert to batch rendering where beneficial
3. Optimize display list usage
4. Measure performance improvements

**Deliverables**:
- Performance profiling results
- Batch rendering for hot paths
- Documented performance improvements
- Comparison with OpenGL

**Tasks**:
- [ ] Profile cgnsplot with large datasets
- [ ] Identify functions called most frequently
- [ ] Convert Zone/Face rendering to batch mode
- [ ] Use pre-computed vertex arrays
- [ ] Measure before/after performance
- [ ] Document optimization techniques
- [ ] Create PHASE4_SESSION9_SUMMARY.md

---

## OpenGL to Render Backend Mapping

### Immediate Mode

| OpenGL Call | Render Backend Call | Notes |
|-------------|---------------------|-------|
| `glBegin(GL_TRIANGLES)` | `cgns_render_begin(ctx, CGNS_PRIM_TRIANGLES)` | 1:1 mapping |
| `glBegin(GL_QUADS)` | `cgns_render_begin(ctx, CGNS_PRIM_QUADS)` | 1:1 mapping |
| `glBegin(GL_LINES)` | `cgns_render_begin(ctx, CGNS_PRIM_LINES)` | 1:1 mapping |
| `glBegin(GL_POLYGON)` | `cgns_render_begin(ctx, CGNS_PRIM_POLYGON)` | May need tesselation |
| `glEnd()` | `cgns_render_end(ctx)` | 1:1 mapping |
| `glVertex3f(x, y, z)` | `cgns_render_vertex3f(ctx, x, y, z)` | 1:1 mapping |
| `glVertex3fv(v)` | `cgns_render_vertex3fv(ctx, v)` | 1:1 mapping |
| `glNormal3f(x, y, z)` | `cgns_render_normal3f(ctx, x, y, z)` | 1:1 mapping |
| `glNormal3fv(n)` | `cgns_render_normal3fv(ctx, n)` | 1:1 mapping |
| `glColor3f(r, g, b)` | `cgns_render_set_color4f(ctx, r, g, b, 1.0)` | Add alpha |
| `glColor3fv(c)` | `cgns_render_set_color4f(ctx, c[0], c[1], c[2], 1.0)` | Add alpha |
| `glColor4f(r, g, b, a)` | `cgns_render_set_color4f(ctx, r, g, b, a)` | 1:1 mapping |

### Materials and Lighting

| OpenGL Call | Render Backend Call | Notes |
|-------------|---------------------|-------|
| `glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, c)` | `cgns_render_set_material_ambient(ctx, c)` | Simplified |
| `glMaterialfv(..., GL_DIFFUSE, c)` | `cgns_render_set_material_diffuse(ctx, c)` | Simplified |
| `glMaterialfv(..., GL_SPECULAR, c)` | `cgns_render_set_material_specular(ctx, c)` | Simplified |
| `glMaterialfv(..., GL_EMISSION, c)` | `cgns_render_set_material_emission(ctx, c)` | Simplified |
| `glMaterialf(..., GL_SHININESS, s)` | `cgns_render_set_material_shininess(ctx, s)` | Simplified |
| `glEnable(GL_LIGHTING)` | `cgns_render_enable_lighting(ctx, 1)` | 1:1 mapping |
| `glDisable(GL_LIGHTING)` | `cgns_render_enable_lighting(ctx, 0)` | 1:1 mapping |
| `glShadeModel(GL_SMOOTH)` | `cgns_render_set_shade_model(ctx, CGNS_SHADE_SMOOTH)` | 1:1 mapping |
| `glShadeModel(GL_FLAT)` | `cgns_render_set_shade_model(ctx, CGNS_SHADE_FLAT)` | 1:1 mapping |

### Display Lists

| OpenGL Call | Render Backend Call | Notes |
|-------------|---------------------|-------|
| `glNewList(id, GL_COMPILE)` | `cgns_render_new_list(ctx, id)` | Simplified |
| `glEndList()` | `cgns_render_end_list(ctx)` | 1:1 mapping |
| `glCallList(id)` | `cgns_render_call_list(ctx, id)` | 1:1 mapping |
| `glDeleteLists(id, 1)` | `cgns_render_delete_list(ctx, id)` | Simplified |

### State Management

| OpenGL Call | Render Backend Call | Notes |
|-------------|---------------------|-------|
| `glViewport(x, y, w, h)` | `cgns_render_set_viewport(ctx, x, y, w, h)` | 1:1 mapping |
| `glEnable(GL_DEPTH_TEST)` | `cgns_render_enable_depth_test(ctx, 1)` | 1:1 mapping |
| `glDisable(GL_DEPTH_TEST)` | `cgns_render_enable_depth_test(ctx, 0)` | 1:1 mapping |
| `glEnable(GL_BLEND)` | `cgns_render_enable_blending(ctx, 1)` | 1:1 mapping |
| `glClear(GL_COLOR_BUFFER_BIT)` | `cgns_render_clear(ctx, ...)` | May need wrapper |
| `glLineWidth(w)` | N/A | **Unsupported** (document) |
| `glPointSize(s)` | N/A | **Unsupported** (document) |

### Matrix Operations

| OpenGL Call | Render Backend Call | Notes |
|-------------|---------------------|-------|
| `glMatrixMode(GL_PROJECTION)` | N/A | Compute matrix, then set |
| `glLoadMatrixf(m)` | `cgns_render_set_projection(ctx, m)` or `_set_view()` | Depends on mode |
| `glViewport(x, y, w, h)` | `cgns_render_set_viewport(ctx, x, y, w, h)` | 1:1 mapping |

**Note**: OpenGL uses matrix stacks, bgfx does not. May need to compute matrices outside and pass in.

---

## Risk Assessment

### High Risks

1. **Tcl/Tk Integration Complexity**
   - tkogl widget is tightly coupled to OpenGL
   - May need to modify tkogl.c significantly
   - **Mitigation**: Start with cgnstcl.c only, keep tkogl for later

2. **Matrix Stack Differences**
   - OpenGL has matrix stack (push/pop)
   - bgfx requires explicit matrix computation
   - **Mitigation**: Compute matrices externally, existing code may already do this

3. **Unsupported Features**
   - glLineWidth, glPointSize not in render backend
   - **Mitigation**: Document as limitations, consider adding if critical

### Medium Risks

1. **Display List Semantics**
   - OpenGL display lists may have different behavior
   - **Mitigation**: Test thoroughly, already validated in Phase 2

2. **Performance Regression**
   - cgnsplot may be slower initially
   - **Mitigation**: Use display lists, optimize later

3. **Real Dataset Edge Cases**
   - May encounter unsupported geometry types
   - **Mitigation**: Test with diverse datasets

### Low Risks

1. **Compilation Issues**
   - Include path problems
   - **Mitigation**: Straightforward build system fixes

2. **Color Format Differences**
   - RGB vs RGBA
   - **Mitigation**: Simple conversion (alpha=1.0)

---

## Success Metrics

### Functional Metrics

- [ ] cgnsplot compiles without errors
- [ ] cgnsplot launches without crashes
- [ ] Simple CGNS files render correctly
- [ ] Complex CGNS files render correctly
- [ ] All geometry types supported (triangles, quads, polygons)
- [ ] Materials and colors correct
- [ ] Lighting works as expected
- [ ] Display lists functional
- [ ] Camera controls work (zoom, pan, rotate)
- [ ] All Tcl/Tk GUI features work

### Performance Metrics

- [ ] Rendering speed ≥ OpenGL version
- [ ] Display lists provide speedup (4-100x expected)
- [ ] No visible lag with large datasets
- [ ] Frame rate ≥ 30 FPS for interactive use

### Quality Metrics

- [ ] No visual artifacts
- [ ] No rendering errors
- [ ] No memory leaks
- [ ] Clean compilation (no warnings)
- [ ] Backward compatible with OpenGL build

---

## Testing Strategy

### Unit Testing

**Not applicable** - cgnsplot is an application, not a library

### Integration Testing

1. **Simple CGNS Files** (src/tests/):
   - Single zone structured grid
   - Multiple zones
   - Boundary conditions
   - Different element types

2. **Complex CFD Datasets**:
   - Unstructured grids
   - Hybrid grids (hex + tet + prism)
   - Large datasets (>1M elements)
   - Time-dependent data (if supported)

3. **Rendering Modes**:
   - Wireframe
   - Shaded (flat/smooth)
   - With lighting
   - With materials
   - With transparency (if used)

4. **Interactive Controls**:
   - Zoom in/out
   - Pan (translate)
   - Rotate (all axes)
   - Reset view
   - Toggle zones on/off

### Platform Testing

- [ ] Linux (primary development platform)
- [ ] Windows (if accessible)
- [ ] macOS (if accessible)

---

## Dependencies

### Build Dependencies

- ✅ render_backend.h (Phase 2)
- ✅ render_backend_bgfx.c (Phase 2-3)
- ✅ bgfx library (external)
- ✅ Tcl/Tk (existing)

### Runtime Dependencies

- ✅ CGNS library (existing)
- ✅ bgfx shaders (Phase 3)
- ⚠️ X11 (Linux) or equivalent platform window system

---

## Documentation Plan

### Documents to Create

1. **PHASE4_OPENGL_CATALOG.md** - Complete list of all OpenGL calls
2. **PHASE4_MIGRATION_MAP.md** - Detailed mapping of each call
3. **PHASE4_SESSION[1-9]_SUMMARY.md** - Session summaries
4. **PHASE4_PROGRESS.md** - Progress tracking
5. **PHASE4_STATUS.md** - Current status
6. **PHASE4_FINAL_SUMMARY.md** - Completion summary

### Documents to Update

1. **PHASE2_FINAL_DOCUMENTATION.md** - Add cgnsplot integration section
2. **WHERE_WE_LEFT_OFF.md** - Update for Phase 4 completion
3. **README.md** (if exists) - Update build instructions

---

## Timeline

### Optimistic (Minimal Approach)

- Session 1: Environment & Analysis (4 hours)
- Session 2: Headers & Context (3 hours)
- Session 3: Immediate Mode (5 hours)
- Session 4: State Management (4 hours)
- Session 5: Display Lists (3 hours)
- Session 6: Matrices & Viewport (3 hours)
- Session 7: Testing (4 hours)
- Session 8: Build Integration (3 hours)

**Total**: ~29 hours (~4 days)

### Realistic (With Optimization)

- Add Session 9: Optimization (4 hours)
- Add debugging time (20% overhead)
- Add documentation time (4 hours)

**Total**: ~40 hours (~5-6 days)

### Pessimistic (With Issues)

- Unforeseen tkogl modifications (8 hours)
- Matrix computation issues (4 hours)
- Platform-specific bugs (4 hours)
- Dataset compatibility issues (4 hours)

**Total**: ~60 hours (~7-8 days)

---

## Next Steps

### Immediate Actions

1. **Review and approve this plan**
2. **Start Session 1**: Environment setup and analysis
3. **Build cgnsplot** with current OpenGL backend
4. **Test with sample files** to understand current behavior
5. **Catalog all OpenGL calls** comprehensively

### Questions to Answer

1. Do we have access to real CFD datasets for testing?
2. Do we need Windows/macOS support immediately, or Linux-first?
3. Is backward compatibility with OpenGL required, or full migration?
4. What is the acceptable timeline for completion?

---

## Conclusion

Phase 4 is well-positioned for success:

✅ **Strong Foundation**: Phases 2-3 provide robust rendering backend
✅ **Clear Scope**: 92 OpenGL calls in well-defined codebase
✅ **1:1 Mapping**: Most calls have direct equivalents
✅ **Low Risk**: Immediate mode replacement is straightforward
✅ **High Value**: Enables cgnsplot on modern platforms

**Recommendation**: Proceed with hybrid approach (direct replacement first, then optimization).

---

**Document Version**: 1.0
**Status**: Planning Complete - Ready for Session 1
**Next**: Environment Setup & Analysis
