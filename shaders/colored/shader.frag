#version 460 core

out vec4 frag_color;

in VS_OUT { vec3 color; }
fs_in;

void main() { frag_color = vec4(fs_in.color, 1.0); }
