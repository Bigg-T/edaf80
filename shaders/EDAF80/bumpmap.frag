#version 410

uniform mat4 normal_model_to_world;// Inverse transpose
uniform sampler2D my_normal_map;
uniform sampler2D my_diffuse;

uniform vec3 ambient; // Material ambient
uniform vec3 diffuse; // Material diffuse
uniform vec3 specular; // Material specular
uniform float shininess;

in VS_OUT {
	vec3 N; // Normal (from vertex shader)
    vec3 T; // Light vector (from VS)
    vec3 B; // View vector (from VS)
    vec3 tc;
    vec3 Lf; // Light vector (from VS)
} fs_in;

out vec4 fColor;
void main()
{
    vec3 N = normalize(fs_in.N);
    vec3 T = normalize(fs_in.T);
    vec3 B = normalize(fs_in.B);
    vec3 L = normalize(fs_in.Lf);
    vec3 TC = fs_in.tc;
    mat3 TBN = mat3(T,B,N);

    vec3 ntext= (texture(my_normal_map, TC.xy).xyz)*2 - 1;
    vec3 diffuse_text= (texture(my_diffuse, TC.xy).xyz);

    vec3 bump = (normalize(normal_model_to_world * (mat4(vec4(T,0),vec4(B,0), vec4(N,0), vec4(0,0,0,1))) * (vec4(ntext, 0)))).xyz;


    //vec3 R = normalize(reflect(-L,bump));
    vec3 diff = diffuse_text*max(dot(L,bump),0.0);
    fColor.xyz =  diff;
    fColor.w = 1.0;

}
