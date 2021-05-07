#version 430 core

uniform mat4 mv;
uniform mat4 projection;
uniform vec3 light_pos;

in vec3 position;
in vec3 color;
in vec3 normal;

out vec3 vColor;

out VS_OUT
{
   vec3 N;
   vec3 L;
   vec3 V;
} vs_out;

void main()
{
    vec4 P = mv * vec4(position, 1.0);

    // Calculate normal in view-space
    vs_out.N = mat3(mv) * normal;

    // Calculate light vector
    vs_out.L = light_pos - P.xyz;

    // Calculate view vector;
    vs_out.V = -P.xyz;


    //gl_Position = (projection * mv) * vec4(position, 1.0);
    gl_Position = projection * P;

    vColor = color;
}
