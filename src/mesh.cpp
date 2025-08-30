#include "mesh.h"

void setInstanceAttributes(GLuint vao, GLuint nextLocation) {
    // Instance Attrib: glm::mat4
    constexpr size_t modelMatrixBufferIndex = 1;

    glVertexArrayAttribBinding(vao, nextLocation, modelMatrixBufferIndex);
    glVertexArrayAttribFormat(vao, nextLocation, 4, GL_FLOAT, GL_FALSE,
                              0 * sizeof(glm::vec4));
    glEnableVertexArrayAttrib(vao, nextLocation++);

    glVertexArrayAttribBinding(vao, nextLocation, modelMatrixBufferIndex);
    glVertexArrayAttribFormat(vao, nextLocation, 4, GL_FLOAT, GL_FALSE,
                              1 * sizeof(glm::vec4));
    glEnableVertexArrayAttrib(vao, nextLocation++);

    glVertexArrayAttribBinding(vao, nextLocation, modelMatrixBufferIndex);
    glVertexArrayAttribFormat(vao, nextLocation, 4, GL_FLOAT, GL_FALSE,
                              2 * sizeof(glm::vec4));
    glEnableVertexArrayAttrib(vao, nextLocation++);

    glVertexArrayAttribBinding(vao, nextLocation, modelMatrixBufferIndex);
    glVertexArrayAttribFormat(vao, nextLocation, 4, GL_FLOAT, GL_FALSE,
                              3 * sizeof(glm::vec4));
    glEnableVertexArrayAttrib(vao, nextLocation++);
    glVertexArrayBindingDivisor(vao, modelMatrixBufferIndex, 1);

    // Consider in future for glMultiDrawElements
    // // Instance Attrib: GLuint64 materialPackIndex
    // constexpr size_t materialPackIndexBufferIndex = 2;

    // glVertexArrayAttribBinding(vao, nextLocation,
    // materialPackIndexBufferIndex); glVertexArrayAttribIFormat(vao,
    // nextLocation, 1, GL_UNSIGNED_INT,
    //                            0 * sizeof(GLuint));
    // glEnableVertexArrayAttrib(vao, nextLocation++);
    // glVertexArrayBindingDivisor(vao, materialPackIndexBufferIndex, 1);
}

template <>
GLuint getVertexArray<UnlitVertex>() {
    static GLuint vao{0};
    if (vao == 0) {
        GLuint nextLocation{0};
        glCreateVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glVertexArrayAttribFormat(vao, nextLocation, 3, GL_FLOAT, GL_FALSE,
                                  offsetof(UnlitVertex, position));
        glVertexArrayAttribBinding(vao, nextLocation, 0);
        glEnableVertexArrayAttrib(vao, nextLocation++);
        glVertexArrayAttribFormat(vao, nextLocation, 2, GL_FLOAT, GL_FALSE,
                                  offsetof(UnlitVertex, texCoord));
        glVertexArrayAttribBinding(vao, nextLocation, 0);
        glEnableVertexArrayAttrib(vao, nextLocation++);

        setInstanceAttributes(vao, nextLocation);

        glBindVertexArray(0);
    }
    return vao;
}

template <>
GLuint getVertexArray<ColoredVertex>() {
    static GLuint vao{0};
    if (vao == 0) {
        GLuint nextLocation{0};
        glCreateVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glVertexArrayAttribFormat(vao, nextLocation, 3, GL_FLOAT, GL_FALSE,
                                  offsetof(ColoredVertex, position));
        glVertexArrayAttribBinding(vao, nextLocation, 0);
        glEnableVertexArrayAttrib(vao, nextLocation++);
        glVertexArrayAttribFormat(vao, nextLocation, 3, GL_FLOAT, GL_FALSE,
                                  offsetof(ColoredVertex, color));
        glVertexArrayAttribBinding(vao, nextLocation, 0);
        glEnableVertexArrayAttrib(vao, nextLocation++);

        setInstanceAttributes(vao, nextLocation);

        glBindVertexArray(0);
    }
    return vao;
}

void destroyVertexArrays() {
    destroyVertexArray<UnlitVertex>();
    destroyVertexArray<ColoredVertex>();
}
