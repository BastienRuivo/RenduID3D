#version 330

#ifdef VERTEX_SHADER

layout(location= 0) in vec3 position;
layout(location= 1) in vec2 texcoord;
layout(location= 2) in vec3 normal;

layout(location=5) in vec3 tangent;

uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;

uniform mat4 mvpLightMatrix;

out vec3 worldPos;
out vec3 worldNorm;
out vec2 uvs;
out vec4 shadowPos;
out mat3 TBN;
out vec3 tgn;


void main( )
{
    gl_Position = mvpMatrix * vec4(position, 1);

    worldPos = (modelMatrix * vec4(position, 1)).xyz;
    worldNorm = (modelMatrix * vec4(normal, 0)).xyz;
    shadowPos = mvpLightMatrix * vec4(worldPos, 1);

    vec3 T = normalize((modelMatrix * vec4(tangent, 0)).xyz);
    vec3 N = normalize(worldNorm);
    vec3 B = cross(T, N);

    tgn = T;

    TBN = mat3(T, B, N);

    uvs = texcoord;
}
#endif

#ifdef FRAGMENT_SHADER
in vec3 worldPos;
in vec3 worldNorm;
in vec2 uvs;
in vec4 shadowPos;
in mat3 TBN;
in vec3 tgn;

uniform vec3 source;
uniform vec3 lightDir;
uniform vec4 lightColor;
uniform vec3 camera;
uniform float lightPower;

uniform float shadowFactor;
uniform float bias;

const float PI= 3.14159265359; 

uniform sampler2D tm_diffuse;
uniform sampler2D tm_emission;
uniform sampler2D tm_normal;
uniform sampler2D tm_specular;

uniform int has_diffuse;
uniform int has_emission;
uniform int has_normal;
uniform int has_specular;

uniform sampler2D shadowMap;
uniform int useNormalMapping;

const float ps = 1.0 / 4096.0;

float computeShadow(float cos_theta)
{
    float b = bias * tan(acos(cos_theta));
    float shadow = 1.0;

    // sample 9 points around shadowPos
    float sum = 25.0;
    for(float i = -2; i <= 2; i++){
        for(float j = -2; j <= 2; j++)
        {
            vec2 uv = shadowPos.xy;
            uv.x += i * ps;
            uv.y += j * ps;
            float d = texture(shadowMap, uv).r;
            if(d <= shadowPos.z - b)
                sum -= shadowFactor;
        }
    }

    shadow = sum / 25.0;

    return shadow;
}

vec3 fresnelSchlick(float cos_theta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - clamp(cos_theta, 0.0, 1.0), 5.0);
}

float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = geometrySchlickGGX(NdotV, roughness);
    float ggx1  = geometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}


void main( )
{
    vec4 diffSample = vec4(1.0);
    if(has_diffuse == 1)
        diffSample = texture2D(tm_diffuse, uvs);

    if(diffSample.a <= 0.5)
        discard;

    vec3 normalSample;
    if(has_normal == 0 || useNormalMapping == 0)
        normalSample = normalize(worldNorm);
    else {
        vec2 uv = uvs;
        normalSample = texture2D(tm_normal, uv).xyz;
        normalSample = normalSample * 2.0 - 1.0;
        normalSample = normalize(TBN * normalSample);
    }

    vec3 emissionSample = vec3(0.0);
    if(has_emission == 1)
        emissionSample = texture2D(tm_emission, uvs).rgb;

    vec4 specSample = vec4(0.0);
    float metallic = 0.0;
    float roughness = 0.2;
    if(has_specular == 1) {
        specSample = texture2D(tm_specular, uvs);
        metallic = specSample.b;
        roughness = max(specSample.g, roughness);
    }

    vec3 cameraDir = normalize(camera - worldPos);
    vec3 lightDir = normalize(-lightDir);
    vec3 halfDir = normalize(lightDir + cameraDir);

    
    vec3 diffuse = (1.0 - metallic) * (diffSample.rgb / PI);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, diffSample.rgb, metallic);

    float NDF = distributionGGX(normalSample, halfDir, roughness);
    float G = geometrySmith(normalSample, cameraDir, lightDir, roughness * roughness);
    vec3 F = fresnelSchlick(max(dot(halfDir, cameraDir), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 nominator = NDF * G * F;
    float denominator = 4.0 * max(dot(normalSample, cameraDir), 0.0) * max(dot(normalSample, lightDir), 0.0);
    vec3 specular = nominator / max(denominator, 0.001);

    float cos_theta = max(dot(normalSample, lightDir), 0.0);
    float shadow = computeShadow(cos_theta);

    vec3 ambient = vec3(0.03) * diffSample.rgb;

    vec3 color = (diffuse + specular) * lightColor.rgb * lightPower * cos_theta;
    vec3 finalColor = (1.0 - shadow) * ambient + shadow * color;
    

    gl_FragColor = vec4(mix(finalColor, normalSample, 0.0), 1.0);

    
}
#endif
