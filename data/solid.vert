#version 450

layout (constant_id = 0) const bool alpha_mode = false;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tc;
layout (location = 2) in vec4 color;
 
layout (location = 0) out vec2 tc_vert;
layout (location = 1) out vec4 color_vert;
 
layout (set = 0, binding = 0, std140) uniform ModelData {
    mat4 world;
};

layout (set = 2, binding = 0, std140) uniform SceneData {
    mat4 view;
    mat4 proj;
};

void main() {
    gl_Position = proj * view * world * vec4(pos, 1);

    tc_vert = tc;
    color_vert = color;
}
