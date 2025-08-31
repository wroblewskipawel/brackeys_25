#version 460 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 tex_coord;
layout(location = 2) in vec4 joints;
layout(location = 3) in vec4 weights;
layout(location = 4) in mat4 model;

out VS_OUT { vec2 tex_coord; }
vs_out;

uniform mat4 projection;
uniform mat4 view;

void main() {
    vs_out.tex_coord = tex_coord;
    gl_Position = projection * view * model * vec4(pos, 1.0);
}
