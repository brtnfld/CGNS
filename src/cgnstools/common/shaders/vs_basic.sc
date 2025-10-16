/*
 * vs_basic.sc
 * Basic vertex shader for CGNS visualization
 * Supports flat and smooth shading with basic lighting
 */

$input a_position, a_normal, a_color0
$output v_color0, v_normal, v_position

#include <bgfx_shader.sh>

uniform mat4 u_modelViewProj;
uniform mat4 u_model;
uniform vec4 u_lightDir;  // Light direction in world space

void main()
{
	// Transform position
	vec4 worldPos = mul(u_model, vec4(a_position, 1.0));
	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));

	// Pass through data for fragment shader
	v_position = worldPos.xyz;
	v_normal = mul(u_model, vec4(a_normal, 0.0)).xyz;
	v_color0 = a_color0;
}
