# bgfx Architecture Fix - Build and Test Guide

## ✅ Pre-Flight Checklist

Run these commands to verify all files are in place:

```bash
cd /Users/brtnfld/packages/cgns.brtnfld/src/cgnstools

# Check implementation files
ls -l cgnsplot/bgfx_driver.h cgnsplot/bgfx_driver.c
ls -l BGFX_ARCHITECTURE_FIX.md

# Verify modifications
grep -q "cgns_render_update_camera" common/render_backend_bgfx.c && echo "✓ render_backend_bgfx.c updated"
grep -q "bgfx_driver.c" cgnsplot/CMakeLists.txt && echo "✓ CMakeLists.txt updated"
grep -q "Bgfx_Driver_Init" cgnsplot/plotwish.c && echo "✓ plotwish.c updated"
grep -q "frame.*-container" cgnsplot/cgnsplot.tcl && echo "✓ cgnsplot.tcl updated"
grep -q "render_cgns_scene" cgnsplot/cgnstcl.c && echo "✓ cgnstcl.c updated"
```

Expected output: All files found, all checks show "✓"

---

## 🔨 Step 1: Build bgfx Libraries (if needed)

```bash
cd /Users/brtnfld/packages/cgns.brtnfld

# Check if bgfx exists
if [ ! -d "external/bgfx" ]; then
    echo "bgfx not found - need to clone it"
    mkdir -p external
    cd external
    git clone --depth 1 https://github.com/bkaradzic/bx.git
    git clone --depth 1 https://github.com/bkaradzic/bimg.git
    git clone --depth 1 https://github.com/bkaradzic/bgfx.git
    cd bgfx
else
    cd external/bgfx
fi

# Build bgfx (this takes ~5 minutes)
make linux-release64

# Verify build succeeded
ls -lh .build/linux64_gcc/bin/*.a
```

Expected output: Should show several .a library files (~20-40 MB total)

---

## 🏗️ Step 2: Configure CGNS Build

```bash
cd /Users/brtnfld/packages/cgns.brtnfld
mkdir -p build
cd build

# Configure with bgfx enabled
cmake .. \
  -DCGNS_BUILD_CGNSTOOLS=ON \
  -DCGNS_ENABLE_BGFX=ON \
  -DCMAKE_BUILD_TYPE=Debug

# Verify configuration
grep "CGNS_ENABLE_BGFX" CMakeCache.txt
```

Expected output:
```
CGNS_ENABLE_BGFX:BOOL=ON
```

---

## ⚙️ Step 3: Build plotwish

```bash
cd /Users/brtnfld/packages/cgns.brtnfld/build

# Build (verbose to see if bgfx_driver.c compiles)
make plotwish VERBOSE=1 2>&1 | tee build.log

# Check for success
if [ -f src/cgnstools/cgnsplot/plotwish ]; then
    echo "✓ Build successful"
    ./src/cgnstools/cgnsplot/plotwish -h 2>&1 | head -5
else
    echo "✗ Build failed - check build.log"
    tail -50 build.log
fi
```

Expected output:
```
✓ Build successful
```

Common build errors and fixes:

**Error:** `bgfx_driver.h: No such file or directory`
**Fix:** Make sure bgfx_driver.h is in `src/cgnstools/cgnsplot/`

**Error:** `undefined reference to bgfx_init`
**Fix:** bgfx libraries not found - rebuild bgfx or check CMake paths

---

## 🧪 Step 4: Test Headless Mode (Quick Validation)

```bash
cd /Users/brtnfld/packages/cgns.brtnfld/src/cgnstools/common

# Run headless test
./test_bgfx_simple

# Expected output (truncated):
# ✓ bgfx backend is available
# ✓ Context created successfully
# ✓ Triangle submitted (3 vertices)
# ALL TESTS PASSED
```

If this fails, the basic bgfx backend has issues. Check for:
- Missing bgfx libraries
- Shader compilation errors
- View transform problems

---

## 🚀 Step 5: Run cgnsplot with bgfx

```bash
cd /Users/brtnfld/packages/cgns.brtnfld/src/cgnstools/cgnsplot

# Run cgnsplot
./plotwish cgnsplot.tcl 2>&1 | tee run.log &

# Monitor output
tail -f run.log
```

### What You Should See

**Console Output (first few seconds):**
```
bgfx_driver: Tcl commands registered
bgfx_driver: Initializing bgfx for window .main.plot.gl
  Window dimensions: 640x480
  Platform: Linux/X11
  Display: 0x7f...
  Window: 0x...
DEBUG: View transform initialized successfully
bgfx_driver: Initialization successful
  Viewport: 640x480 (aspect: 1.33)
bgfx initialized successfully
```

**Visual Output:**
- Window opens with dark gray background
- **RGB triangle in center**:
  - Red vertex at bottom-left
  - Green vertex at bottom-right
  - Blue vertex at top
- Smooth color gradient between vertices
- Triangle rotates in 3D space (if camera code active)

### Success Criteria

✅ Window opens without crashes
✅ Background color is dark gray (not black, not white)
✅ Triangle is visible and has RGB colors
✅ Window resizes smoothly
✅ No application hangs or freezes
✅ Console shows "bgfx initialized successfully"

---

## 🐛 Troubleshooting

### Issue 1: Window opens but is black (no triangle)

**Diagnosis:**
```bash
# Check if view transform was set
grep "View transform initialized" run.log
```

**Possible causes:**
- View transform not set → Should see "View transform initialized successfully"
- Geometry outside frustum → Triangle coords may be wrong
- Shader issue → Check for shader compilation errors in log

**Fix:**
```bash
# Edit render_backend_bgfx.c, change clear color to red for debugging:
# Line 668: bgfx_set_view_clear(0, BGFX_CLEAR_COLOR, 0xff0000ff, 1.0f, 0);
# If background turns red, rendering works but geometry is wrong
```

### Issue 2: "plot_init_bgfx: command not found"

**Diagnosis:**
```bash
# Check if bgfx_driver was initialized
grep "Bgfx_Driver_Init" run.log
```

**Cause:** bgfx_driver.c not compiled or not initialized

**Fix:**
```bash
# Verify it's in the build:
grep bgfx_driver build/src/cgnstools/cgnsplot/CMakeFiles/plotwish.dir/DependInfo.cmake

# Rebuild if missing:
cd build
make clean
cmake .. -DCGNS_ENABLE_BGFX=ON
make plotwish
```

### Issue 3: Crashes immediately

**Diagnosis:**
```bash
# Run under gdb
cd src/cgnstools/cgnsplot
gdb ./plotwish
> run cgnsplot.tcl
# When it crashes:
> bt
```

**Common causes:**
- NULL pointer in bgfx_driver.c → Check platform_data
- bgfx library mismatch → Rebuild bgfx and CGNS
- Missing X11 libraries → Install libx11-dev

### Issue 4: Window hangs on resize or interaction

**Cause:** View transform being updated from wrong thread

**Fix:** Make sure `cgns_render_update_camera()` is ONLY called from `render_cgns_scene()`, not from Tcl event callbacks

---

## 📊 Performance Validation

Once rendering works, measure performance:

```tcl
# Add to cgnsplot.tcl after successful init:
set ::frame_count 0
set ::last_time [clock milliseconds]

proc measure_fps {} {
    incr ::frame_count
    set now [clock milliseconds]
    set elapsed [expr {$now - $::last_time}]
    if {$elapsed >= 1000} {
        set fps [expr {$::frame_count * 1000.0 / $elapsed}]
        puts "FPS: [format %.1f $fps]"
        set ::frame_count 0
        set ::last_time $now
    }
    after 100 measure_fps
}
measure_fps
```

**Expected:** 55-60 FPS (limited by 16ms timer)

---

## 🎯 Next Steps After Successful Test

1. **Verify clean shutdown:**
   ```bash
   # Close window and check for:
   # - No segfaults
   # - "bgfx backend shutdown complete" message
   # - Clean Tcl exit
   ```

2. **Test with real CGNS file:**
   ```bash
   # Load a small CGNS file and verify mesh renders
   # (Will show test triangle until render_cgns_scene() is ported)
   ```

3. **Implement camera controls:**
   - Add mouse rotation bindings
   - Add zoom control
   - Update view matrix via `cgns_render_update_camera()`

4. **Port CGNS rendering code:**
   - Replace test triangle with actual mesh rendering
   - Adapt existing OpenGL code to use `cgns_render_*()` API

---

## 📞 Getting Help

If tests fail:

1. **Capture full output:**
   ```bash
   ./plotwish cgnsplot.tcl 2>&1 | tee full_output.log
   ```

2. **Check system info:**
   ```bash
   uname -a
   glxinfo | grep -i "opengl version"
   cat /etc/os-release
   ```

3. **Include in bug report:**
   - build.log (CMake + make output)
   - run.log (runtime console output)
   - Screenshot if window opens
   - System info

---

## ✨ Success!

If you see the RGB triangle rendering smoothly, **congratulations!**

The architecture is working correctly and you're ready to integrate actual CGNS mesh rendering.

The critical fixes that made this work:
1. ✅ View transform set during initialization (not in event loop)
2. ✅ Clean separation: Tcl → bgfx_driver → render_backend → bgfx
3. ✅ No hybrid code mixing bgfx and OpenGL in tkogl
4. ✅ Proper frame timing via Tcl `after` command

Happy rendering! 🎨
