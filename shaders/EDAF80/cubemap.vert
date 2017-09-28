#version 410

uniform vec3 camera_position; // Defined in world space

uniform mat4 vertex_model_to_world;// Model -> World space
uniform mat4 normal_model_to_world;// Inverse transpose
uniform mat4 vertex_world_to_clip;// Model -> Clip space


layout (location = 0) in vec3 Pos;// Defined in model space
layout (location = 1) in vec3 Normal;// Defined in model space

out VS_OUT {
	vec3 N; // Normal (from vertex shader)
    vec3 V; // Light vector (from VS)
} fs_out;

void main()
{
    vec3 worldPos = (vertex_model_to_world*vec4(Pos,1)).xyz;
    fs_out.N = (normal_model_to_world*vec4(Normal,0)).xyz;
    fs_out.V = camera_position - worldPos;
    gl_Position = vertex_world_to_clip*vec4(Pos,1);
}