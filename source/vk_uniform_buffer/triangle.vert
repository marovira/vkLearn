#version 450 core
#extension GL_ARB_separate_shader_objects: enable

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 colour;

layout(location = 0) out vec3 vertColour;

layout(binding = 0) uniform MatricesBuffer
{
    mat4 model;
    mat4 view;
    mat4 proj;
} matrices;

void main()
{
    gl_Position = matrices.proj * matrices.view * matrices.model *
        vec4(position, 0.0, 1.0);
    vertColour = colour;
}


