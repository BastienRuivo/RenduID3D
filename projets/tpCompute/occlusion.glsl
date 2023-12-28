#version 430

#ifdef COMPUTE_SHADER

struct Object
{
    vec4 pmin;
    vec4 pmax;
    uint index_count;
};

struct Draw
{
    uint index_count;
    uint instance_count;
    uint first_index;
    uint vertex_base;
    uint instance_base;
};

layout(std430, binding=0) buffer Objects {
    Object object[];
};

layout(std430, binding=1) buffer DrawParameters {
    Draw param[];
};

uniform mat4 mvpMatrix;

layout(local_size_x = 256) in;

vec3[8] boxify(vec3 minvec, vec3 maxvec, mat4 mvpMatrix) {
    vec3[8] result;
    result[0] = vec3(minvec.x, minvec.y, minvec.z);
    result[1] = vec3(minvec.x, minvec.y, maxvec.z);
    result[2] = vec3(minvec.x, maxvec.y, minvec.z);
    result[3] = vec3(minvec.x, maxvec.y, maxvec.z);
    result[4] = vec3(maxvec.x, minvec.y, minvec.z);
    result[5] = vec3(maxvec.x, minvec.y, maxvec.z);
    result[6] = vec3(maxvec.x, maxvec.y, minvec.z);
    result[7] = vec3(maxvec.x, maxvec.y, maxvec.z);
    return result;
}

uniform sampler2D depthSampler;
uniform uint depthSamplerWidth;
uniform uint depthSamplerHeight;

void main( )
{
    uint index = gl_GlobalInvocationID.x;
    if (index >= param.length()) {
        return;
    }

    Object obj = object[index];
    vec3[8] box = boxify(obj.pmin.xyz, obj.pmax.xyz, mvpMatrix);

    vec3 minvec = vec3(1e10, 1e10, 1e10);
    vec3 maxvec = vec3(-1e10, -1e10, -1e10);

    float Z = 1e10;
    for (int i = 0; i < 8; i++) {
        vec4 p = mvpMatrix * vec4(box[i], 1.0);
        p /= p.w;
        minvec = min(minvec, p.xyz);
        maxvec = max(maxvec, p.xyz);
        Z = min(Z, p.z);
    }

    vec2 uv = (gl_GlobalInvocationID.xy + vec2(0.5)) / vec2(float(depthSamplerWidth), float(depthSamplerHeight));
    float depth = texture(depthSampler, uv).x;

    param[index].index_count = uint(depth > Z) * obj.index_count;
}

#endif