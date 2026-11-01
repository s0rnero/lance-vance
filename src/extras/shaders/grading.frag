uniform sampler2D tex0;
uniform vec4 u_gradeR;
uniform vec4 u_gradeG;
uniform vec4 u_gradeB;

FSIN vec4 v_color;
FSIN vec2 v_tex0;
FSIN float v_fog;

void
main(void)
{
	vec4 c = texture(tex0, vec2(v_tex0.x, 1.0-v_tex0.y));
	c.a = 1.0;

	vec4 color;
	color.r = dot(u_gradeR, c);
	color.g = dot(u_gradeG, c);
	color.b = dot(u_gradeB, c);
	color.a = 1.0;

	FRAGCOLOR(color);
}
