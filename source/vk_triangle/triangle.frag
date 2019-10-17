#version 450 core
#extension GL_ARB_separate_shader_objects: enable

layout (location = 0) in vec3 vertColour;
layout (location = 0) out vec4 fragColour;

void main()
{
    fragColour = vec4(vertColour, 1.0);
}
