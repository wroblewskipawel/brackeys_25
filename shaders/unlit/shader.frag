#version 460 core

out vec4 frag_color;

in VS_OUT { vec2 tex_coord; }
fs_in;

uniform sampler2D texture;

void main() { frag_color = texture(texture, fs_in.tex_coord); }
