uniform sampler2D tex0;
uniform sampler2D tex1;

uniform vec4 u_leedsEnv;

#define envCoeff (u_leedsEnv.x)

FSIN vec4 v_color;
FSIN vec2 v_tex0;
FSIN vec2 v_tex1;
FSIN float v_fog;

void
main(void)
{
	vec4 pass1 = v_color*texture(tex0, vec2(v_tex0.x, 1.0-v_tex0.y));
	vec3 envmap = texture(tex1, vec2(v_tex1.x, 1.0-v_tex1.y)).rgb;

	pass1.rgb = mix(u_fogColor.rgb, pass1.rgb, v_fog);
	pass1.rgb += envmap*envCoeff;

	DoAlphaTest(pass1.a);

	FRAGCOLOR(pass1);
}
