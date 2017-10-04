#version 410

uniform mat4 normal_model_to_world;// Inverse transpose
// uniform sampler2D u_heightMap;


in VS_OUT {
	vec3 N; // Normal (from vertex shader)
    vec3 V; // View vector (from VS)
} fs_in;

out vec4 fColor;
void main()
{
    vec3 N = normalize(fs_in.N);
    vec3 V = normalize(fs_in.V);

    float facing = 1 - max(dot(V,N),0);
    vec4 c_deep = vec4(0.0, 0.0, 0.1, 1.0);
    vec4 c_shallow = vec4(0.0, 0.5, 0.5, 1.0);
    vec4 c_water = mix(c_deep, c_shallow, facing);
    fColor =  c_water;
}
