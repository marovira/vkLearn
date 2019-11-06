#version 450 core
#extension GL_ARB_separate_shader_objects: enable

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 colour;
layout (location = 2) in vec2 texCoord;

layout (location = 0) out vec3 vertColour;
layout (location = 1) out vec2 vertTexCoord;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main()
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(position, 1.0);
    vertColour = colour;
    vertTexCoord = texCoord;
}


