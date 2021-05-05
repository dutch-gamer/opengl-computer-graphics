#version 430 core

uniform mat4 mv;
uniform mat4 projection;
uniform vec3 light_pos;

in vec3 position;
in vec3 color;
in vec3 normal;

out vec3 vColor;

void main()
{
    gl_Position = (projection * mv) * vec4(position, 1.0);

    vColor = color;
}
