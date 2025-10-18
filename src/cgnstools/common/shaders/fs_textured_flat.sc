/*
 * fs_textured_flat.sc
 * Textured flat shading fragment shader for CGNS visualization
 * Combines texture mapping with flat shading (no normal interpolation)
 */

$input v_color0, v_normal, v_position, v_texcoord0

#include <bgfx_shader.sh>

uniform vec4 u_lightDir;         // Light direction
uniform vec4 u_ambientLight;     // Ambient light color
uniform vec4 u_diffuseLight;     // Diffuse light color
uniform vec4 u_materialAmbient;  // Material ambient
uniform vec4 u_materialDiffuse;  // Material diffuse
uniform vec4 u_enableLighting;   // x: enable lighting
uniform vec4 u_enableTexture;    // x: enable texture, y: blend mode

SAMPLER2D(s_texture, 0);         // Texture sampler

void main()
{
	// Use non-normalized normal for flat shading effect
	vec3 normal = v_normal;
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

	// Apply flat shading if lighting enabled
	if (u_enableLighting.x > 0.5) {
		vec3 lightDir = normalize(u_lightDir.xyz);

		// Ambient component
		vec3 ambient = u_ambientLight.rgb * u_materialAmbient.rgb;

		// Diffuse component (Lambert) - flat shading
		float diff = max(dot(normalize(normal), lightDir), 0.0);
		vec3 diffuse = diff * u_diffuseLight.rgb * u_materialDiffuse.rgb;

		// Combine (no specular for flat shading)
		color.rgb = color.rgb * (ambient + diffuse);
	}

	gl_FragColor = color;
}
