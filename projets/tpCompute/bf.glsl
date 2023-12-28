#version 430

#ifdef VERTEX_SHADER

layout(location= 0) in vec3 position;
layout(location= 1) in vec2 texcoord;
layout(location= 2) in vec3 normal;
layout(location= 4) in uint materialId;


uniform mat4 modelMatrix;
uniform mat4 mvpMatrix;

uniform mat4 mvpLightMatrix;

out vec3 worldPos;
out vec3 worldNorm;
out vec2 uvs;
out flat uint matId;
//out vec4 shadowPos;

void main( )
{
    gl_Position = mvpMatrix * vec4(position, 1);

    worldPos = (modelMatrix * vec4(position, 1)).xyz;
    worldNorm = (modelMatrix * vec4(normal, 0)).xyz;
    //shadowPos = mvpLightMatrix * vec4(worldPos, 1);
    
    uvs = texcoord;
    matId = materialId;
}
#endif

#ifdef FRAGMENT_SHADER
in vec3 worldPos;
in vec3 worldNorm;
in vec2 uvs;
in flat uint matId;
in vec4 shadowPos;


struct Material
{
    vec4 diffuse;              //!< couleur diffuse / de base.
    float ns;                   //!< concentration des reflets, exposant pour les reflets blinn-phong.
    // vec4 specular;             //!< couleur du reflet.
    // vec4 emission;             //!< pour une source de lumiere.
    int diffuse_texture;        //!< indice de la texture, ou -1.
    // int specular_texture;        //!< indice de la texture, ou -1.
    // int emission_texture;        //!< indice de la texture, ou -1.
    int ns_texture;             //!< indice de la texture de reflet, ou -1.
};


// bind texture array
layout(binding= 0) uniform sampler2DArray diffuseTextures;
layout(std140, binding= 1) buffer Mats
{
    Material materials[];
};
uniform sampler2D shadowMap;

uniform vec3 lightDir;
uniform vec4 lightColor;
uniform vec3 camera;
uniform float lightPower;

uniform float bias;
uniform float shadowFactor;

const float ps = 1.0 / 4096.0;
const float PI= 3.14159265359; 

const vec4 ambient = vec4(0.0, 0.0, 0.0, 1.0);
out vec4 fragment_color;

void main( )
{

    vec3 ld = normalize(lightDir);
    vec3 camDir = normalize(camera - worldPos);
    vec3 halfDir = normalize(camDir + ld);
    vec3 nn = normalize(worldNorm);
    

    float cos_theta = max(0.0, dot(nn, ld));
    float cos_theta_h = dot(nn, halfDir);

    Material material = materials[matId];
    vec4 diffuse = material.diffuse;
    float ns = material.ns;

    

    if(material.diffuse_texture != -1)
    {
        diffuse = texture(diffuseTextures, vec3(uvs, material.diffuse_texture));
    } 
    
    if(material.ns_texture != -1)
    {
        ns = texture(diffuseTextures, vec3(uvs, material.ns_texture)).r;
    } 


    if(diffuse.a <= 0.001)
        discard;

    // float b = bias * tan(acos(cos_theta));
    // float shadow = 1.0;

    // float sum = 25.0;
    // int k = 0;
    // for(float i = -2; i <= 2; i++){
    //     for(float j = -2; j <= 2; j++)
    //     {
    //         k++;
    //         vec2 uv = shadowPos.xy;
    //         uv.x += i * ps;
    //         uv.y += j * ps;
    //         float d = texture(shadowMap, uv).r;
    //         if(d <= shadowPos.z - b)
    //             sum -= shadowFactor;
    //     }
    // }

    // shadow = sum / 25.0;

    float fr = (ns + 8.0) / (8.0 * PI) * pow(cos_theta_h, ns);

    fragment_color = ambient * diffuse + lightColor * cos_theta * diffuse * lightPower;// * shadow;
}
#endif
