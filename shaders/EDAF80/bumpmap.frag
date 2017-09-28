#version 410

in VS_OUT {
	vec3 N; // Normal (from vertex shader)
    vec3 T; // Light vector (from VS)
    vec3 B; // View vector (from VS)
} fs_in;

out vec4 fColor;
void main()
{
    vec3 N = normalize(fs_in.N)*2 - 1;
    vec3 T = normalize(fs_in.T);
    vec3 B = normalize(fs_in.B);
    mat3 TBN = glm::mat3(T,B,N);
    fcolor = wit * (glm::mat4(TBN,0)) * (glm::vec4(N, 0))

}
