#version 430

#ifdef COMPUTE_SHADER

uniform int w;
uniform int h;
uniform int lvl;

// depth texture float
uniform sampler2D input_img;
// depth texture mipmap
layout(binding = 1, r32f) writeonly uniform image2D output_img;




layout(local_size_x = 32, local_size_y = 32) in;
void main( )
{
    // get the index of the current thread
    int x = int(gl_GlobalInvocationID.x);
    int y = int(gl_GlobalInvocationID.y);

    // check if the thread is outside the image
    if (x >= w || y >= h) return;

    int fx = x * 2;
    int fy = y * 2;

    float d0, d1, d2, d3;
    d0 = texelFetch(input_img, ivec2(fx, fy), lvl).r;
    d1 = texelFetch(input_img, ivec2(fx + 1, fy), lvl).r;
    d2 = texelFetch(input_img, ivec2(fx, fy + 1), lvl).r;
    d3 = texelFetch(input_img, ivec2(fx + 1, fy + 1), lvl).r;

    // get the maximum value
    d0 = max(max(d0, d1), max(d2, d3));

    // write to output image
    imageStore(output_img, ivec2(x, y), vec4(d0, 0.0, 0.0, 0.0));
    
}

#endif