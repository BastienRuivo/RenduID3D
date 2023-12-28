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
    int drawCount;
};

uniform mat4 mvpMatrix;
uniform mat4 vpInverseMatrix;

layout(local_size_x = 256) in;

vec3[8] boxify(vec3 minvec, vec3 maxvec) {
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
void main( )
{
    if(gl_GlobalInvocationID.x == 0) drawCount = 0;
    barrier();
    if(gl_GlobalInvocationID.x >= object.length()) return;
    if(param[gl_GlobalInvocationID.x].index_count == 0) return;
    Object obj = object[gl_GlobalInvocationID.x];
    vec3[8] box = boxify(obj.pmin.xyz, obj.pmax.xyz);
    vec3[8] frustum = boxify(vec3(-1.0, -1.0, -1.0), vec3(1.0, 1.0, 1.0));

    uint PointBehindBox[6] = {
        0, 0, 0, 0, 0, 0
    };

    uint PointBehindFrustum[6] = {
        0, 0, 0, 0, 0, 0
    };

    for(int i = 0; i < 8; i++) {
        vec4 pos = mvpMatrix * vec4(box[i], 1.0);
        PointBehindBox[0] = PointBehindBox[0] + uint(pos.x < -pos.w);
        PointBehindBox[1] = PointBehindBox[1] + uint(pos.x >= pos.w);
        PointBehindBox[2] = PointBehindBox[2] + uint(pos.y < -pos.w);
        PointBehindBox[3] = PointBehindBox[3] + uint(pos.y >= pos.w);
        PointBehindBox[4] = PointBehindBox[4] + uint(pos.z < -pos.w);
        PointBehindBox[5] = PointBehindBox[5] + uint(pos.z >= pos.w);
    }

    for(int i = 0; i < 8; i++) {
        vec4 pos = vpInverseMatrix * vec4(frustum[i], 1.0);
        PointBehindFrustum[0] = PointBehindFrustum[0] + uint(frustum[i].x < -pos.w);
        PointBehindFrustum[1] = PointBehindFrustum[1] + uint(frustum[i].x >= pos.w);
        PointBehindFrustum[2] = PointBehindFrustum[2] + uint(frustum[i].y < -pos.w);
        PointBehindFrustum[3] = PointBehindFrustum[3] + uint(frustum[i].y >= pos.w);
        PointBehindFrustum[4] = PointBehindFrustum[4] + uint(frustum[i].z < -pos.w);
        PointBehindFrustum[5] = PointBehindFrustum[5] + uint(frustum[i].z >= pos.w);
    }

    bool isOutside = false;

    for(int i = 0; i < 6; i++) {
        isOutside = isOutside || (PointBehindBox[i] == 8) || (PointBehindFrustum[i] == 8);
    }

    param[gl_GlobalInvocationID.x].index_count = obj.index_count * uint(!isOutside);

    if(isOutside) return;
    atomicAdd(drawCount, 1);
}

#endif