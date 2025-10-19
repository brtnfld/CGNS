#!/usr/bin/env tclsh
# Test native OpenGL rendering after disabling bgfx wrappers in cgnstcl.c
# This test should show rendering working even though bgfx init fails

package require Tk
package require Tkogl

puts "=== Native OpenGL Rendering Test ==="
puts ""
puts "This test verifies that rendering works with native OpenGL"
puts "even though bgfx initialization fails."
puts ""

# Create main window
wm title . "Native OpenGL Test - After CGNSTCL_BGFX_DISABLED"
wm geometry . 800x600

# Create tkogl widget
puts "Creating tkogl widget..."
if {[catch {
    tkogl .gl -width 800 -height 600 -double 1 -depth 1
    pack .gl -fill both -expand 1
} err]} {
    puts "ERROR: Failed to create tkogl widget: $err"
    exit 1
}
puts "✓ tkogl widget created"
puts ""

# Wait for window to be realized
update
after 500

# Test basic OpenGL rendering
puts "Testing basic OpenGL rendering..."
.gl render {
    # Clear to dark blue
    glClearColor 0.1 0.1 0.3 1.0
    glClear 16640  ;# GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT

    glEnable 2929  ;# GL_DEPTH_TEST

    # Draw a test triangle using native OpenGL (not bgfx)
    glBegin 4  ;# GL_TRIANGLES
    glColor3f 1.0 0.0 0.0
    glVertex3f 0.0 0.5 0.0
    glColor3f 0.0 1.0 0.0
    glVertex3f -0.5 -0.5 0.0
    glColor3f 0.0 0.0 1.0
    glVertex3f 0.5 -0.5 0.0
    glEnd

    glFlush
}

puts "✓ OpenGL rendering commands executed"
puts ""
puts "====================================================================="
puts "EXPECTED RESULT:"
puts "  - You should see a colored triangle on dark blue background"
puts "  - bgfx initialization may fail (that's OK)"
puts "  - Rendering should work via native OpenGL fallback"
puts ""
puts "If you see the triangle, native OpenGL rendering is working!"
puts "====================================================================="
puts ""
puts "Window will auto-close in 10 seconds..."
puts ""

# Auto-close after 10 seconds
after 10000 {
    destroy .
}

# Wait for window events
tkwait window .

puts ""
puts "=== Test Complete ==="
exit 0
