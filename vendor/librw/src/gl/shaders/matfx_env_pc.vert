uniform mat4 u_texMatrix;
uniform vec4 u_colorClamp;
uniform vec4 u_envColor;

VSIN(ATTRIB_POS)	vec3 in_pos;

VSOUT vec4 v_color;
VSOUT vec4 v_envColor;
VSOUT vec2 v_tex0;
VSOUT vec2 v_tex1;
VSOUT float v_fog;

void
main(void)
{
	vec4 Vertex = u_world * vec4(in_pos, 1.0);
	gl_Position = u_proj * u_view * Vertex;
	vec3 Normal = mat3(u_world) * in_normal;

	v_tex0 = in_tex0;
	v_tex1 = (u_texMatrix * vec4(Normal, 1.0)).xy;

	v_color = in_color;
	v_color.rgb += u_ambLight.rgb*surfAmbient;
	v_color.rgb += DoDynamicLight(Vertex.xyz, Normal)*surfDiffuse;
	vec3 odEye = (inverse(u_view) * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
	v_color.rgb += DoSpecular(Vertex.xyz, Normal, odEye)*surfSpecular;
	v_color.rgb *= u_matColor.rgb;
	v_color = vec4(clamp(v_color.rgb, 0.0, 1.0), u_matColor.a);

	v_envColor = max(v_color, u_colorClamp) * u_envColor;

	v_fog = DoFog(gl_Position.w);
}
