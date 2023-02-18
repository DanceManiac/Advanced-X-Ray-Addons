#ifndef HMODEL_H
#define HMODEL_H

#include "common.h"
#include "cgim.h"

uniform samplerCUBE env_s0;
uniform samplerCUBE env_s1;

uniform half4 env_color; // color.w  = lerp factor
uniform half3x4 m_v2w;

float3 SmallSkyCompute(float3 uv)
{
	float3 color = float3(0.0, 0.0, 0.0);

	static const int3 o[8] =
	{
		{-1, 0, 0},
		{ 1, 0, 0},
		{ 0,-1, 0},
		{ 0, 1, 0},
		{ 0, 0,-1},
		{ 0, 0, 1},
		{ 1, 1, 1},
		{ 0, 0, 0}
	};

	static const int num = 8;

	for (int i = 0; i < num; i++)
	{
		float3 tap = normalize(uv + o[i] * SMALLSKY_BLUR_INTENSITY);
		float3 env0 = texCUBE(env_s0, tap);
		float3 env1 = texCUBE(env_s1, tap);
		color += lerp(env0, env1, env_color.w) / num;
	}

	float top_to_down_vec = saturate(uv.y);
	top_to_down_vec *= top_to_down_vec;
	
	static const float factor = SMALLSKY_TOP_VECTOR_POWER;
	color *= saturate(factor + (1.0 - factor) * top_to_down_vec) + (1.0 - factor) / 2;

	return color;
}

void hmodel
(
    out half3 hdiffuse, out half3 hspecular,
    half m, half h, half s, float3 point, half3 normal)
{
    // hscale - something like diffuse reflection
    half3 nw = mul(m_v2w, normal);
    half hscale = h; //. *        (.5h + .5h*nw.y);

    // reflection vector
    float3 v2pointL = normalize(point);
    half3 v2point = mul(m_v2w, v2pointL);
    half3 vreflect = reflect(v2point, nw);
    half hspec = .5h + .5h * dot(vreflect, v2point);

    // material
    half4 light = tex3D(s_material, half3(hscale, hspec, m)); // sample material

	// diffuse color
	float3 env_d = SmallSkyCompute(nw) * env_color.xyz;
	env_d *= env_d; // contrast
	
    hdiffuse = env_d * light.xyz + L_ambient.rgb;

    // specular color
    vreflect.y = vreflect.y * 2 - 1; // fake remapping

	float3 env_s = SmallSkyCompute(vreflect) * env_color.xyz;
	env_s *= env_s; // contrast

    hspecular = env_s * light.w * s;
}
#endif