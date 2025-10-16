/*
 * fs_smooth.sc
 * Smooth shading fragment shader for CGNS visualization
 * Per-vertex lighting calculation (interpolated)
 */

$input v_color0, v_normal, v_position

#include <bgfx_shader.sh>

uniform vec4 u_lightDir;       // Light direction
uniform vec4 u_ambientLight;   // Ambient light color
uniform vec4 u_diffuseLight;   // Diffuse light color
uniform vec4 u_specularLight;  // Specular light color
uniform vec4 u_materialAmbient; // Material ambient
uniform vec4 u_materialDiffuse; // Material diffuse
uniform vec4 u_materialSpecular; // Material specular
uniform vec4 u_materialShininess; // Material shininess (x component)
uniform vec4 u_enableLighting; // x: enable lighting
uniform vec4 u_cameraPos;      // Camera position

void main()
{
	vec3 normal = normalize(v_normal);
	vec4 color = v_color0;

	if (u_enableLighting.x > 0.5) {
		// Lighting enabled
		vec3 lightDir = normalize(u_lightDir.xyz);
		vec3 viewDir = normalize(u_cameraPos.xyz - v_position);

		// Ambient component
		vec3 ambient = u_ambientLight.rgb * u_materialAmbient.rgb;

		// Diffuse component (Lambert)
		float diff = max(dot(normal, lightDir), 0.0);
		vec3 diffuse = diff * u_diffuseLight.rgb * u_materialDiffuse.rgb;

		// Specular component (Blinn-Phong)
		vec3 halfDir = normalize(lightDir + viewDir);
		float spec = pow(max(dot(normal, halfDir), 0.0), u_materialShininess.x);
		vec3 specular = spec * u_specularLight.rgb * u_materialSpecular.rgb;

		// Combine lighting with vertex color
		color.rgb = color.rgb * (ambient + diffuse) + specular;
	}

	gl_FragColor = color;
}
