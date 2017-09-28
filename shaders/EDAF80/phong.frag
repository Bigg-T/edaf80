#version 410

uniform vec3 ambient; // Material ambient
uniform vec3 diffuse; // Material diffuse
uniform vec3 specular; // Material specular
uniform float shininess;

in VS_OUT {
	vec3 Nf; // Normal (from vertex shader)
    vec3 Lf; // Light vector (from VS)
    vec3 Vf; // View vector (from VS)
} fs_in;

out vec4 fColor;
void main()
{
    vec3 N = normalize(fs_in.Nf);
    vec3 L = normalize(fs_in.Lf);
    vec3 V = normalize(fs_in.Vf);
    vec3 R = normalize(reflect(-L,N));
    vec3 diff = diffuse*max(dot(L,N),0.0);
    vec3 specular2 = specular*pow(max(dot(V,R),0.0), shininess);
    fColor.xyz = ambient + diff + specular2;
    fColor.w = 1.0;
}
