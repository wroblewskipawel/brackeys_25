#version 460 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 tex_coord;

out VS_OUT { vec2 tex_coord; }
vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
    vs_out.tex_coord = tex_coord;
    gl_Position = projection * view * model * vec4(pos, 1.0);
}
