#version 460 core

out vec4 frag_color;

in VS_OUT { vec2 tex_coord; }
fs_in;

// Temporary removed until image texture loading is implemented
// uniform sampler2D texture;

void main() {
    frag_color = vec4(fs_in.tex_coord, 0.0, 1.0);
}
