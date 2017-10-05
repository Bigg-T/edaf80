#version 410
uniform float u_time;

uniform vec3 camera_position; // Defined in world space

uniform mat4 vertex_model_to_world; // Model -> World space
uniform mat4 normal_model_to_world; // Inverse transpose
uniform mat4 vertex_world_to_clip; // Model -> Clip space

layout (location = 0) in vec3 pos;// Defined in model space

out VS_OUT {
	vec3 N;
	vec3 V;
} fs_out;


void main()
{

    vec2 amplitude = vec2(1.0, 0.5);
    vec2 f = vec2(0.2, 0.4);
    vec2 phase = vec2(0.5, 1.3);
    vec2 sharpness = vec2(2.0, 2.0); // k
    vec2 dir1 = vec2(-1.0, 0.0);
    vec2 dir2 = vec2(-0.7, 0.7);
    mat2 dir = mat2(dir1, dir2);


    float theta1 = dot(dir1, vec2(pos.x, pos.z)) * f.x +  u_time*phase.x;
    float theta2 = dot(dir2, vec2(pos.x, pos.z)) * f.y +  u_time*phase.y;

    float G1 =  amplitude.x * pow((sin(theta1) * 0.5 + 0.5), sharpness.x);
    float G2 =  amplitude.y * pow((sin(theta2) * 0.5 + 0.5), sharpness.y);

    float dG1dx = 0.5*sharpness.x*f.x*amplitude.x * pow((sin(theta1) * 0.5 + 0.5), sharpness.x - 1)*cos(theta1)*dir1.x;
    float dG2dx = 0.5*sharpness.y*f.y*amplitude.y * pow((sin(theta2) * 0.5 + 0.5), sharpness.y - 1)*cos(theta2)*dir2.x;

    float dG1dz = 0.5*sharpness.x*f.x*amplitude.x * pow((sin(theta1) * 0.5 + 0.5), sharpness.x - 1)*cos(theta1)*dir2.y;
    float dG2dz = 0.5*sharpness.y*f.y*amplitude.y * pow((sin(theta2) * 0.5 + 0.5), sharpness.y - 1)*cos(theta2)*dir2.y;

    vec3 worldPos = (vertex_model_to_world*vec4(pos.x, pos.y + G1 + G2, pos.z,1)).xyz;
    fs_out.V = camera_position - worldPos;
    fs_out.N = vec3(-(dG1dx+dG2dx), 1, -(dG1dz+dG2dz));

    // vec3 pos = position;
    // vec2 offset1 = vec2(0.8, 0.4) * u_time * 0.1;
    // vec2 offset2 = vec2(0.6, 1.1) * u_time * 0.1;

   //    float hight1 = texture2D(u_heightMap, uv + offset1).r * 0.02;
    //   float hight2 = texture2D(u_heightMap, uv + offset2).r * 0.02;
   //  pos.z += hight1 + hight2;


    gl_Position = vertex_world_to_clip * vec4(worldPos,1);

}