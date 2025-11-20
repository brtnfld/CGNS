/*
 * fs_unlit.sc
 * Unlit fragment shader for CGNS visualization
 * For lines, wireframes, and other unlit geometry
 */

$input v_color0, v_normal, v_position

#include <bgfx_shader.sh>

void main()
{
	// Unlit - just output vertex color (ignore normal and position)
	gl_FragColor = v_color0;
}
