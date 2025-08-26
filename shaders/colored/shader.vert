#version 460 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in mat4 model;

out VS_OUT { vec3 color; }
vs_out;

uniform mat4 projection;
uniform mat4 view;

void main() {
    vs_out.color = color;
    gl_Position = projection * view * model * vec4(pos, 1.0);
}
