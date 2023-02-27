
#define MaxLights 16

static const float PI = 3.14159265;

struct Light {
  float3 Color;
  float Intensity;
  float3 Position;
  float _Padding;
};

struct Material {
  float4 Albedo;
  float Roughness;
  float Metallic;
};

float3 CalcR0(float3 diffuseColor, float metallic) {
  float3 r0 = float3(0.04f, 0.04f, 0.04f);
  return lerp(r0, diffuseColor, metallic);
}

float CalcAttenuation(float d, float falloffStart, float falloffEnd) {
  return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

float3 SchlickFresnel(float3 R0, float3 normal, float3 lightDir) {
  float cosTheta = saturate(dot(normal, lightDir));
  float f0 = 1.0f - cosTheta;
  float3 reflect = R0 + (float3(1.0f, 1.0f, 1.0f) - R0) * pow(f0, 5.0f);

  return reflect;
}

float GGXDistribution(float3 normal, float3 halfDir, float roughness) {
  float roughness2 = pow(roughness, 2.0f);
  float NdotD = max(dot(normal, halfDir), 0.0f);
  float dotProduct2 = pow(NdotD, 2.0f);
  float num = roughness2;
  float denom = PI * pow(dotProduct2 * (roughness2 - 1.0f) + 1.0f, 2.0f);

  return num / denom;
}

float SchlickGGX(float dotProduct, float k) {
  float num = dotProduct;
  float denom = dotProduct * (1.0f - k) + k;

  return num / denom;
}

float SchlickGeometry(float3 normal, float3 eyeDir, float3 lightDir, float k) {
  float NdotE = max(dot(normal, eyeDir), 0.0f);
  float NdotL = max(dot(normal, lightDir), 0.0f);
  return SchlickGGX(NdotE, k) * SchlickGGX(NdotL, k);
}

float3 DiffuseReflectance(float3 diffuseColor, float3 specularRatio,
                          float3 metallic) {
  float3 diffuseRatio = float3(1.0f, 1.0f, 1.0f) - specularRatio;
  float3 diffuseMaterial = diffuseRatio * (1.0f - metallic);
  return diffuseMaterial * (diffuseColor / PI);
}

float3 BRDF(float3 normal, float3 halfDir, float3 lightDir, float3 eyeDir,
            Material mat) {
  float3 diffuseColor = mat.Albedo;
  float3 Fr =
      SchlickFresnel(CalcR0(diffuseColor, mat.Metallic), normal, lightDir);
  float D = GGXDistribution(normal, halfDir, mat.Roughness);

  float directK = pow(mat.Roughness + 1.0f, 2.0f) / 8.0f;

  float G = SchlickGeometry(normal, eyeDir, lightDir, directK);

  float NdotL = max(dot(normal, lightDir), 0.0f);
  float NdotE = max(dot(normal, eyeDir), 0.0f);

  float3 specular = (Fr * D * G) / max(4.0f * NdotL * NdotE, 0.001);
  float3 diffuse = DiffuseReflectance(diffuseColor, Fr, mat.Metallic);

  return (specular + diffuse) * NdotL;
}