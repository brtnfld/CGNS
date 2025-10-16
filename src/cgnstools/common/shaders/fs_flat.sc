/*
 * fs_flat.sc
 * Flat shading fragment shader for CGNS visualization
 * Per-face lighting calculation
 */

$input v_color0, v_normal, v_position

#include <bgfx_shader.sh>

uniform vec4 u_lightDir;       // Light direction
uniform vec4 u_ambient Light;    // Ambient light color
uniform vec4 u_diffuseLight;   // Diffuse light color
uniform vec4 u_materialAmbient; // Material ambient
uniform vec4 u_materialDiffuse; // Material diffuse
uniform vec4 u_enableLighting; // x: enable lighting (0 or 1)

void main()
{
	vec3 normal = normalize(v_normal);
	vec4 color = v_color0;

	if (u_enableLighting.x > 0.5) {
		// Lighting enabled
		vec3 lightDir = normalize(u_lightDir.xyz);

		// Ambient component
		vec3 ambient = u_ambientLight.rgb * u_materialAmbient.rgb;

		// Diffuse component (Lambert)
		float diff = max(dot(normal, lightDir), 0.0);
		vec3 diffuse = diff * u_diffuseLight.rgb * u_materialDiffuse.rgb;

		// Combine lighting with vertex color
		color.rgb = color.rgb * (ambient + diffuse);
	}

	gl_FragColor = color;
}
