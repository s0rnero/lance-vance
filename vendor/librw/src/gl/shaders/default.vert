VSIN(ATTRIB_POS)	vec3 in_pos;

// 85.1: instancing. La matriz del mundo llega como atributo por instancia
// (4 columnas en las locations 13-16) en vez de como uniforme; asi un mismo
// DFF se dibuja N veces con una sola orden. Sin OD_INSTANCED no cambia nada.
#ifdef OD_INSTANCED
VSIN(12)	vec4 in_inst0;
VSIN(13)	vec4 in_inst1;
VSIN(14)	vec4 in_inst2;
VSIN(15)	vec4 in_inst3;
#define OD_WORLD mat4(in_inst0, in_inst1, in_inst2, in_inst3)
#else
#define OD_WORLD u_world
#endif

VSOUT vec4 v_color;
VSOUT vec2 v_tex0;
VSOUT float v_fog;

void
main(void)
{
	vec4 Vertex = OD_WORLD * vec4(in_pos, 1.0);
	gl_Position = u_proj * u_view * Vertex;
	vec3 Normal = mat3(OD_WORLD) * in_normal;

	v_tex0 = in_tex0;

	v_color = in_color;
	v_color.rgb += u_ambLight.rgb*surfAmbient;
	v_color.rgb += DoDynamicLight(Vertex.xyz, Normal)*surfDiffuse;
	v_color = clamp(v_color, 0.0, 1.0);
	v_color *= u_matColor;

	v_fog = DoFog(gl_Position.w);
}
