#version 430 core

in vec3 vColor;
in VS_OUT
{
    vec3 N;
    vec3 L;
    vec3 V;
} fs_in;

// Material properties
uniform vec3 mat_ambient;
uniform vec3 mat_diffuse;

void main()
{
    // Normalize the incoming N, L and V vectors
    vec3 N = normalize(fs_in.N);
    vec3 L = normalize(fs_in.L);
    vec3 V = normalize(fs_in.V);

    // Calculate R locally
    vec3 R = reflect(-L, N);

    // Compute the diffuse and specular components for each fragment
    //vec3 specular = pow(max(dot(R, V), 0.0), mat_power) * mat_specular;
    vec3 diffuse = max(dot(N, L), 0.0) * vColor;

    //gl_FragColor = vec4(vColor, 1.0);
    gl_FragColor = vec4(mat_ambient + diffuse, 1.0);
}
