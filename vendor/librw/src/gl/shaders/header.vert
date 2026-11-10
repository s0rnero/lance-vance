
//#define DIRECTIONALS
//#define POINTLIGHTS
//#define SPOTLIGHTS

#define ATTRIB_POS	0
#define ATTRIB_NORMAL	1
#define ATTRIB_COLOR	2
#define ATTRIB_WEIGHTS	3
#define ATTRIB_INDICES	4
#define ATTRIB_TEXCOORDS0	5
#define ATTRIB_TEXCOORDS1	6


VSIN(ATTRIB_NORMAL)	vec3 in_normal;
VSIN(ATTRIB_COLOR)	vec4 in_color;
VSIN(ATTRIB_WEIGHTS)	vec4 in_weights;
VSIN(ATTRIB_INDICES)	vec4 in_indices;
VSIN(ATTRIB_TEXCOORDS0)	vec2 in_tex0;
VSIN(ATTRIB_TEXCOORDS1)	vec2 in_tex1;


#ifdef USE_UBOS
layout(std140) uniform State
{
	vec2 u_alphaRef;
	vec4  u_fogData;
	vec4  u_fogColor;
};
#else
uniform vec4 u_alphaRef;
uniform vec4  u_fogData;
uniform vec4  u_fogColor;
#endif

#define u_fogStart (u_fogData.x)
#define u_fogEnd (u_fogData.y)
#define u_fogRange (u_fogData.z)
#define u_fogDisable (u_fogData.w)

#ifdef USE_UBOS
layout(std140) uniform Scene
{
	mat4 u_proj;
	mat4 u_view;
};
#else
uniform mat4 u_proj;
uniform mat4 u_view;
#endif

#define MAX_LIGHTS 8

#ifdef USE_UBOS
layout(std140) uniform Object
{
	mat4  u_world;
	vec4  u_ambLight;
	vec4 u_lightParams[MAX_LIGHTS];	// type, radius, minusCosAngle, hardSpot
	vec4 u_lightPosition[MAX_LIGHTS];
	vec4 u_lightDirection[MAX_LIGHTS];
	vec4 u_lightColor[MAX_LIGHTS];
};
#else
uniform mat4 u_world;
uniform vec4 u_ambLight;
uniform vec4 u_lightParams[MAX_LIGHTS];	// type, radius, minusCosAngle, hardSpot
uniform vec4 u_lightPosition[MAX_LIGHTS];
uniform vec4 u_lightDirection[MAX_LIGHTS];
uniform vec4 u_lightColor[MAX_LIGHTS];
#endif

uniform vec4 u_matColor;
uniform vec4 u_surfProps;	// amb, spec, diff, extra
uniform vec4 u_odSpecularPower;	// .x Fresnel, .y exponente del especular Blinn (0 = sin especular)
uniform vec4 u_odSpecColor;		// color del especular (el del SP)

#define surfAmbient (u_surfProps.x)
#define surfSpecular (u_surfProps.y)
#define surfDiffuse (u_surfProps.z)

vec3 DoDynamicLight(vec3 V, vec3 N)
{
	vec3 color = vec3(0.0, 0.0, 0.0);
	for(int i = 0; i < MAX_LIGHTS; i++){
		if(u_lightParams[i].x == 0.0)
			break;
#ifdef DIRECTIONALS
		if(u_lightParams[i].x == 1.0){
			// direct
			float l = max(0.0, dot(N, -u_lightDirection[i].xyz));
			color += l*u_lightColor[i].rgb;
		}else
#endif
#ifdef POINTLIGHTS
		if(u_lightParams[i].x == 2.0){
			// point
			vec3 dir = V - u_lightPosition[i].xyz;
			float dist = length(dir);
			float atten = max(0.0, (1.0 - dist/u_lightParams[i].y));
			float l = max(0.0, dot(N, -normalize(dir)));
			color += l*u_lightColor[i].rgb*atten;
		}else
#endif
#ifdef SPOTLIGHTS
		if(u_lightParams[i].x == 3.0){
			// spot
			vec3 dir = V - u_lightPosition[i].xyz;
			float dist = length(dir);
			float atten = max(0.0, (1.0 - dist/u_lightParams[i].y));
			dir /= dist;
			float l = max(0.0, dot(N, -dir));
			float pcos = dot(dir, u_lightDirection[i].xyz);	// cos to point
			float ccos = -u_lightParams[i].z;
			float falloff = (pcos-ccos)/(1.0-ccos);
			if(falloff < 0.0)	// outside of cone
				l = 0.0;
			l *= max(falloff, u_lightParams[i].w);
			return l*u_lightColor[i].rgb*atten;
		}else
#endif
			;
	}
	return color;
}

float DoFog(float w)
{
	return clamp((w - u_fogEnd)*u_fogRange, u_fogDisable, 1.0);
}

// PORTADO -- GTA VC original de PC (gta-vc.exe, pipeline D3D8)
//   Que se toma: el retail hacia un ESPECULAR BLINN POR LUZ, coloreado con el color
//   de la luz que ilumina (el brillo del semaforo en la chapa del coche). librw no lo
//   implementaba: solo subida el "specular" del material (u_surfProps.y) y nunca lo
//   usaba, asi que las superficies brillantes no brillaban.
//   Adaptacion: el exponente llega en u_odSpecularPower (0 = apagado) y el color de la
//   luz es el que ya viaja en u_lightColor[].
vec3 DoSpecular(vec3 worldPos, vec3 N, vec3 eye)
{
	vec3 color = vec3(0.0, 0.0, 0.0);
	if(u_odSpecularPower.y <= 0.0)
		return color;
	vec3 V = normalize(eye - worldPos);
	// El termino de Fresnel del SP/neo (mix(b^5, 1, fresnel)): el brillo se
	// refuerza en los angulos de vision rasantes.
	float odB = 1.0 - clamp(dot(V, N), 0.0, 1.0);
	float odFresnel = mix(odB*odB*odB*odB*odB, 1.0, u_odSpecularPower.x);
	for(int i = 0; i < MAX_LIGHTS; i++){
		if(u_lightParams[i].x == 0.0)
			break;
		vec3 L;
		float atten = 1.0;
#ifdef DIRECTIONALS
		if(u_lightParams[i].x == 1.0){
			L = -u_lightDirection[i].xyz;
		}else
#endif
#ifdef POINTLIGHTS
		if(u_lightParams[i].x == 2.0){
			vec3 dir = u_lightPosition[i].xyz - worldPos;
			float dist = length(dir);
			atten = max(0.0, (1.0 - dist/u_lightParams[i].y));
			L = dir / max(dist, 0.0001);
		}else
#endif
#ifdef SPOTLIGHTS
		if(u_lightParams[i].x == 3.0){
			vec3 dir = u_lightPosition[i].xyz - worldPos;
			float dist = length(dir);
			atten = max(0.0, (1.0 - dist/u_lightParams[i].y));
			L = dir / max(dist, 0.0001);
			float pcos = dot(-L, u_lightDirection[i].xyz);
			float ccos = -u_lightParams[i].z;
			float falloff = (pcos-ccos)/(1.0-ccos);
			if(falloff < 0.0)
				continue;
			atten *= max(falloff, u_lightParams[i].w);
		}else
#endif
			continue;
		vec3 H = normalize(L + V);
		float s = max(0.0, dot(N, H));
		color += pow(s, u_odSpecularPower.y) * u_lightColor[i].rgb * atten;
	}
	return color;
}
