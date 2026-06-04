#version 450

layout(set=0,binding=0) uniform sampler2D gPosition;
layout(set=0,binding=1) uniform sampler2D gNormal;
layout(set=0,binding=2) uniform sampler2D gAlbedo;
layout(set=0,binding=3) uniform sampler2D gMaterial;
layout(set=0,binding=4) uniform sampler2D gDepth;

layout(location=0) in vec2 fragUV;
layout(location=0) out vec4 outColor;

layout(push_constant) uniform PC {
    layout(offset=0)  vec4 camPos;
    layout(offset=16) vec4 lightDir;
    layout(offset=32) vec4 lightColor;
    layout(offset=48) mat4 shadowVP;
};

const float PI = 3.14159265359;

float DistributionGGX(vec3 N,vec3 H,float r){float a=r*r,a2=a*a,NdH=max(dot(N,H),0.0);float d=NdH*NdH*(a2-1.0)+1.0;return a2/(PI*d*d);}
float GeometrySmith(vec3 N,vec3 V,vec3 L,float r){float k=(r+1.0)*(r+1.0)/8.0;return(max(dot(N,V),0.0)/(max(dot(N,V),0.0)*(1.0-k)+k))*(max(dot(N,L),0.0)/(max(dot(N,L),0.0)*(1.0-k)+k));}
vec3 FresnelSchlick(float c,vec3 F0){return F0+(1.0-F0)*pow(clamp(1.0-c,0.0,1.0),5.0);}

void main(){
    vec3 worldPos=texture(gPosition,fragUV).xyz;
    vec3 N=normalize(texture(gNormal,fragUV).xyz);
    vec3 baseColor=texture(gAlbedo,fragUV).rgb;
    vec2 mr=texture(gMaterial,fragUV).rg;
    float metallic=mr.x,roughness=clamp(mr.y,0.04,1.0);

    vec3 V=normalize(camPos.xyz-worldPos);
    vec3 L=normalize(-lightDir.xyz);
    vec3 H=normalize(V+L);
    vec3 F0=mix(vec3(0.04),baseColor,metallic);

    float NDF=DistributionGGX(N,H,roughness),G=GeometrySmith(N,V,L,roughness);
    vec3 F=FresnelSchlick(max(dot(H,V),0.0),F0);
    vec3 kS=F,kD=(vec3(1.0)-kS)*(1.0-metallic);
    vec3 spec=(NDF*G*F)/max(4.0*max(dot(N,V),0.0)*max(dot(N,L),0.0),0.001);
    vec3 diff=kD*baseColor/PI;
    float NdotL=max(dot(N,L),0.0);
    vec3 Lo=(diff+spec)*lightColor.rgb*NdotL*lightColor.w;
    vec3 ambient=vec3(0.03)*baseColor;
    vec3 color=ambient+Lo;
    color=color/(color+vec3(1.0));
    color=pow(color,vec3(1.0/2.2));
    outColor=vec4(color,1.0);
}
