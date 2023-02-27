
#include "LightUtil.hlsl"

cbuffer cbPerObject : register(b0) { float4x4 gWorld; };

cbuffer cbMaterial : register(b1) {
  float4 gDiffuseAlbedo;
  float gRoughness;
  float gMetallic;
  float4x4 gMatTransform;
};

cbuffer cbPass : register(b2) {
  float4x4 gView;
  float4x4 gInvView;
  float4x4 gProj;
  float4x4 gInvProj;
  float4x4 gViewProj;
  float4x4 gInvViewProj;
  float3 gEyePosW;
  float cbPerObjectPad1;
  float2 gRenderTargetSize;
  float2 gInvRenderTargetSize;
  float gNearZ;
  float gFarZ;
  float gTotalTime;
  float gDeltaTime;

  AreaLight gLights[MaxLights];
};

struct VertexIn {
  float3 PosL : POSITION;
  float3 NormalL : NORMAL;
};

struct VertexOut {
  float4 PosH : SV_POSITION;
  float3 PosW : POSITION;
  float3 NormalW : NORMAL;
};

VertexOut VS(VertexIn vin) {
  VertexOut vout = (VertexOut)0.0f;

  float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
  vout.PosW = posW.xyz;

  vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);
  vout.PosH = mul(posW, gViewProj);

  return vout;
}

float4 PS(VertexOut pin) : SV_Target {
  float3 eyeDir = normalize(gEyePosW - pin.PosW);

  Material mat = {gDiffuseAlbedo, gRoughness, gMetallic};

  float3 Lo = float3(0.0f, 0.0f, 0.0f);

  for(int i = 0; i < 4; i++) {
    float lightPos = gLights[i].Position;
    float3 lightDir = normalize(lightPos - pin.PosW);
    float3 halfDir = normalize(eyeDir + lightDir);
    float distance = length(lightPos - pin.PosW);
    float attenuation = 1.0f / (distance * distance);
    float3 radiance = gLights[i].Intensity * attenuation * gLights[i].Color;

    Lo += BRDF(pin.NormalW, halfDir, lightDir, eyeDir, mat);
  }
  return Lo;
}
