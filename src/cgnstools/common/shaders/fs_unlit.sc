/*
 * fs_unlit.sc
 * Unlit fragment shader for CGNS visualization
 * For lines, wireframes, and other unlit geometry
 */

$input v_color0

#include <bgfx_shader.sh>

void main()
{
	gl_FragColor = v_color0;
}
