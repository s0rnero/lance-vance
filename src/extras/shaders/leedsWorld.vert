uniform vec4 u_prelightTweak;

#define tweakMult (u_prelightTweak.x)
#define tweakAdd (u_prelightTweak.y)
#define litFactor (u_prelightTweak.z)

VSIN(ATTRIB_POS)	vec3 in_pos;

VSOUT vec4 v_color;
VSOUT vec2 v_tex0;
VSOUT float v_fog;

void
main(void)
{
	vec4 Vertex = u_world * vec4(in_pos, 1.0);
	gl_Position = u_proj * u_view * Vertex;

	v_tex0 = in_tex0;

	vec4 prelight = in_color;
	prelight.rgb = clamp(prelight.rgb*tweakMult + vec3(tweakAdd), 0.0, 1.0);

	vec3 ambient = mix(vec3(1.0), u_ambLight.rgb, litFactor);
	vec3 emissive = u_ambLight.rgb*0.5;

	v_color = clamp(prelight*vec4(ambient, 1.0) + vec4(emissive*u_surfProps.w, 0.0), 0.0, 1.0);
	v_color.a *= u_matColor.a;

	v_fog = DoFog(gl_Position.w);
}
