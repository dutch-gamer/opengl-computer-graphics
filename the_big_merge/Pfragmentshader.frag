#version 430 core

in vec3 vColor;

// Material properties
uniform vec3 mat_ambient;
uniform vec3 mat_diffuse;

void main()
{
    gl_FragColor = vec4(vColor, 1.0);
}
