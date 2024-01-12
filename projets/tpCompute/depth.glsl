#version 330

#ifdef VERTEX_SHADER

layout(location= 0) in vec3 position;

uniform mat4 mvpMatrix;

void main( )
{
    gl_Position = mvpMatrix * vec4(position, 1);
}
#endif

#ifdef FRAGMENT_SHADER

uniform float znear;
uniform float zfar;

out vec4 fragColor;

void main( )
{
    // float depth = -gl_FragCoord.z;
    // depth = (2.0 * znear) / (zfar + znear - depth * (zfar - znear));
    
    // fragColor = vec4(depth, 0, 0, 1);
}
#endif
