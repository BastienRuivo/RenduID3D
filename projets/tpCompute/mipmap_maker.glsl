#version 430

#ifdef COMPUTE_SHADER

uniform int w;
uniform int h;

// depth texture float
layout(binding = 0, r32f) readonly coherent uniform image2D input_img;
// depth texture mipmap
layout(binding = 1, r32f) writeonly coherent uniform image2D output_img;



layout(local_size_x = 32, local_size_y = 32) in;
void main( )
{
    // get the index of the current thread
    int x = int(gl_GlobalInvocationID.x);
    int y = int(gl_GlobalInvocationID.y);

    // check if the thread is inside the image
    if (x >= w || y >= h) return;

    int fx = x * 2;
    int fy = y * 2;

    float d0, d1, d2, d3;
    d0 = imageLoad(input_img, ivec2(fx, fy)).r;
    d1 = imageLoad(input_img, ivec2(fx + 1, fy)).r;
    d2 = imageLoad(input_img, ivec2(fx, fy + 1)).r;
    d3 = imageLoad(input_img, ivec2(fx + 1, fy + 1)).r;

    // get the maximum depth
    d0 = max(max(d0, d1), max(d2, d3));
    // mean

    // write the result to image
    imageStore(output_img, ivec2(x, y), vec4(d0, 0.0, 0.0, 0.0));
}

#endif