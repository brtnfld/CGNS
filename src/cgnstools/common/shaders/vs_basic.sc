/*
 * vs_basic.sc
 * Basic vertex shader for CGNS visualization
 * Supports flat and smooth shading with basic lighting
 */

$input a_position, a_normal, a_color0
$output v_normal, v_position, v_color0

#include <bgfx_shader.sh>

void main()
{
	// Transform position using bgfx built-in uniforms
	// u_model is defined in bgfx_shader.sh (model[0])
	vec4 worldPos = mul(u_model[0], vec4(a_position, 1.0));
	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));

	// Pass through data for fragment shader
	v_position = worldPos.xyz;
	v_normal = mul(u_model[0], vec4(a_normal, 0.0)).xyz;
	v_color0 = a_color0;
}
