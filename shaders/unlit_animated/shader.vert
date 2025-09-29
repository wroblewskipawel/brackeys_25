#version 460 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 tex_coord;
layout(location = 2) in uvec4 joints;
layout(location = 3) in vec4 weights;
layout(location = 4) in mat4 model;

out VS_OUT { vec2 tex_coord; }
vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform uint jointMatrixOffset;
uniform uint jointMatrixCount;

layout(std140, binding = 1) readonly buffer JointMatrices {
    mat4 joints[];
}
joint_matrices;

void main() {
    uint offset = jointMatrixOffset + jointMatrixCount * gl_InstanceID;
    mat4 joint_transform = mat4(0.0);
    for (int i = 0; i < 4; ++i) {
        joint_transform += joint_matrices.joints[offset + joints[i]] * weights[i];
    }
    vs_out.tex_coord = tex_coord;
    gl_Position = projection * view * model * joint_transform * vec4(pos, 1.0);
}
