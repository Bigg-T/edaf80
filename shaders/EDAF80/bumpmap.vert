#version 410

uniform mat4 vertex_model_to_world;// Model -> World space
uniform mat4 normal_model_to_world;// Inverse transpose
uniform mat4 vertex_world_to_clip;// Model -> Clip space
uniform vec3 light_position; // Defined in world space
uniform vec3 camera_position; // Defined in world space

layout (location = 0) in vec3 Pos;// Defined in model space
layout (location = 1) in vec3 Normal;// Defined in model space
layout (location = 3) in vec3 tan;// Defined in model space
layout (location = 4) in vec3 binorm;// Defined in model space

layout (location = 2) in vec3 textcoord;// Defined in model space

out VS_OUT {
	vec3 N; // Normal (from vertex shader)
    vec3 T; // Light vector (from VS)
    vec3 B; // View vector (from VS)
    vec3 tc;
    vec3 Lf; // Light vector (from VS)

} fs_out;

void main()
{
    vec3 worldPos = (vertex_model_to_world*vec4(Pos,1)).xyz;
    fs_out.N = (normal_model_to_world*vec4(Normal,0)).xyz;
    fs_out.B = (normal_model_to_world*vec4(binorm,0)).xyz;
    fs_out.T = (normal_model_to_world*vec4(tan,0)).xyz;;
    fs_out.tc = textcoord;
    fs_out.Lf = light_position - worldPos;
    gl_Position = vertex_world_to_clip*vec4(worldPos,1);
}