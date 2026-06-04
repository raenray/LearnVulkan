#version 450

layout(location = 0) in  vec3 fragWorldPos;
layout(location = 1) in  vec3 fragNormal;
layout(location = 0) out vec4 outColor;

// ── PBR params (push constants, offset 128) ──────────────────────────────

layout(push_constant) uniform PC {
    layout(offset = 128) vec4  albedo;     // rgb=baseColor, a=unused
    layout(offset = 144) vec4  params;     // x=metallic, y=roughness
    layout(offset = 160) vec4  lightDir;   // xyz=direction to light, w=intensity
    layout(offset = 176) vec4  camPos;     // xyz=camera world position
};

const float PI = 3.14159265359;

// ── Cook-Torrance microfacet functions ────────────────────────────────────

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float denom  = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = NdotV / (NdotV * (1.0 - k) + k);
    float ggx2 = NdotL / (NdotL * (1.0 - k) + k);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(camPos.xyz - fragWorldPos);
    vec3 L = normalize(-lightDir.xyz);   // lightDir points TOWARD light

    // ── material ────────────────────────────────────────────────────────
    vec3  baseColor = albedo.rgb;
    float metallic  = params.x;
    float roughness = clamp(params.y, 0.04, 1.0);

    // F0: dielectric ~0.04, metal = baseColor
    vec3 F0 = mix(vec3(0.04), baseColor, metallic);

    vec3 H = normalize(V + L);

    // ── Cook-Torrance BRDF ──────────────────────────────────────────────
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    vec3 specular = (NDF * G * F) / max(4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0), 0.001);
    vec3 diffuse  = kD * baseColor / PI;

    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (diffuse + specular) * vec3(1.0) * NdotL * lightDir.w;

    // ── ambient ─────────────────────────────────────────────────────────
    vec3 ambient = vec3(0.03) * baseColor;

    vec3 color = ambient + Lo;

    // ── tonemap (simple Reinhard) ───────────────────────────────────────
    color = color / (color + vec3(1.0));

    // ── gamma ───────────────────────────────────────────────────────────
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, 1.0);
}
