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

layout(std430, binding=2) buffer DrawResults {
    uint drawCount;
};

const float limit = 1e9;

uniform mat4 mvpvp;

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
uniform uint width;
uniform uint height;

layout(local_size_x = 256) in;
void main( )
{
    uint index = gl_GlobalInvocationID.x;

    if(index == 0) {
        drawCount = 0;
    }

    barrier();

    if (index >= param.length()) {
        return;
    }

    Object obj = object[index];
    vec3[8] box = boxify(obj.pmin.xyz, obj.pmax.xyz, mvpvp);

    vec3 minvec = vec3(limit, limit, limit);
    vec3 maxvec = vec3(-limit, -limit, -limit);

    float Z = limit;
    for (int i = 0; i < 8; i++) {
        vec4 p = mvpvp * vec4(box[i], 1.0);
        p /= p.w;
        minvec = min(minvec, p.xyz);
        maxvec = max(maxvec, p.xyz);
        Z = min(Z, p.z);
    }

    float depth = limit;

    float w = abs(maxvec.x - minvec.x);
    float h = abs(maxvec.y - minvec.y);

    float texelw = 1.0 / width;
    float texelh = 1.0 / height;

    // zmax doit etre inferieur a zmin de la voite

    for(float i = minvec.x; i < maxvec.x; i+=texelw) {
        for(float j = minvec.y; j < maxvec.y; j+=texelh) {
            vec2 uv = vec2(i, j);
            float d = texture(depthSampler, uv).r;
            depth = min(depth, d);
        }
    }

    param[index].index_count = uint(depth > Z) * obj.index_count;
    // Make object visible if it is in front of the near plane
    if (param[index].index_count > 0) {
        atomicAdd(drawCount, uint(depth  * 1000.0));
    }
}

#endif