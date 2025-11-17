#version 460 core

out vec4 frag_color;

in VS_OUT { vec2 tex_coord; }
fs_in;

struct ArrayUnlit {
    sampler2DArray albedo;
};

uniform ArrayUnlit arrayUnlit;

struct UnlitMaterial {
    float x_scale;
    float y_scale;
};

layout(std140, binding = 0) readonly buffer MaterialPack {
    UnlitMaterial materials[];
}
material_pack;

uniform uint material;

void main() {
    vec2 tex_coord = fs_in.tex_coord * vec2(material_pack.materials[material].x_scale,
                                            material_pack.materials[material].y_scale);
    vec4 albedo = texture(arrayUnlit.albedo, vec3(tex_coord, material));
    frag_color = albedo;
}
