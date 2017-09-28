#version 410


uniform samplerCube my_cube_map;

in VS_OUT {
	vec3 N; // Normal (from vertex shader)
    vec3 V; // Rv vector (from VS)
} fs_in;

out vec4 fColor;
void main()
{
    vec3 N = normalize(fs_in.N);
    vec3 V = normalize(fs_in.V);
    vec3 R = normalize(reflect(-V,N));
    vec4 reflection = texture(my_cube_map, R);
    fColor = reflection;
}
