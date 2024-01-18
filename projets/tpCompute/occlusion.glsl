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
    //float drawCount;
};

const float limit = 1e9;

uniform mat4 mvp;

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

uniform sampler2D depthSampler;
uniform sampler2D depthMipmap;
uniform int width;
uniform int height;
uniform int nbLvls;

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
    vec3[8] box = boxify(obj.pmin.xyz, obj.pmax.xyz);

    vec4 p0 = mvp * vec4(box[0], 1.0);
    p0 /= p0.w;

    vec3 minvec = p0.xyz;
    vec3 maxvec = p0.xyz;

    for (int i = 1; i < 8; i++) {
        vec4 p = mvp * vec4(box[i], 1.0);
        p /= p.w;
        minvec.x = min(minvec.x, p.x);
        minvec.y = min(minvec.y, p.y);
        minvec.z = min(minvec.z, p.z);

        maxvec.x = max(maxvec.x, p.x);
        maxvec.y = max(maxvec.y, p.y);
        maxvec.z = max(maxvec.z, p.z);
    }

    if(maxvec.z > 1.0) {
        param[index].index_count = obj.index_count;
        return;
    }

    float depth = 0.0;

    float w = (maxvec.x - minvec.x);
    float h = (maxvec.y - minvec.y);

    float texelw = 1.0 / width;
    float texelh = 1.0 / height;

    // normalized coordinate -> pixel coordinate
    int x = max(int(minvec.x * float(width)), 0);
    int y = max(int(minvec.y * float(height)), 0);

    int maxx = min(int(maxvec.x * float(width)), width);
    int maxy = min(int(maxvec.y * float(height)), height);

    // zmax doit etre inferieur a zmin de la voite
    float nbpxscreen = float(width * height);
    float nbpxbox = min(float((maxx - x) * (maxy - y)), nbpxscreen);
    float pixelRatio = nbpxbox / nbpxscreen;
    int lvl = int(floor(pixelRatio * float(nbLvls)));

    int div = int(pow(2.0, lvl));
    if(lvl == 0) {
        for(int i = x; i < maxx; i++) {
            for(int j = y; j < maxy; j++) {
                float d = texelFetch(depthSampler, ivec2(i, j), 0).r;
                depth = max(depth, d);
            }
        }
    } else {
        x = x / div;
        y = y / div;

        maxx = max(min(maxx / div, width), 4);
        maxy = max(min(maxy / div, height), 4);

        

        for(float i = 0; i < maxx; i++) {
            for(float j = 0; j < maxy; j++) {
                vec2 uv = vec2(i * 2 * lvl / float(width), j * 2 * lvl / float(height));
                float d = textureLod(depthMipmap, uv, lvl - 1).r;
                depth = max(depth, d);
            }
        }
    }

    param[index].index_count = uint(minvec.z <= depth) * obj.index_count;
    //param[index].index_count = obj.index_count;
    if(param[index].index_count > 0) {
        atomicAdd(drawCount, 1);
    }
}

#endif