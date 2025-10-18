/*
 * fs_textured_unlit.sc
 * Textured unlit fragment shader for CGNS visualization
 * Texture mapping without any lighting calculations
 * Useful for wireframes, overlays, and UI elements
 */

$input v_color0, v_texcoord0

#include <bgfx_shader.sh>

uniform vec4 u_enableTexture;    // x: enable texture, y: blend mode

SAMPLER2D(s_texture, 0);         // Texture sampler

void main()
{
	vec4 color = v_color0;

	// Sample texture if enabled
	if (u_enableTexture.x > 0.5) {
		vec4 texColor = texture2D(s_texture, v_texcoord0);
		// Blend modes: 0=modulate, 1=replace, 2=decal
		if (u_enableTexture.y < 0.5) {
			// Modulate (multiply with vertex color)
			color = color * texColor;
		} else if (u_enableTexture.y < 1.5) {
			// Replace (use texture color, keep vertex alpha)
			color = vec4(texColor.rgb, color.a);
		} else {
			// Decal (blend based on texture alpha)
			color.rgb = mix(color.rgb, texColor.rgb, texColor.a);
		}
	}

	gl_FragColor = color;
}
