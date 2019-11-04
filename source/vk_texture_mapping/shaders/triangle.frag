#version 450 core
#extension GL_ARB_separate_shader_objects: enable

layout (location = 0) in vec3 vertColour;
layout (location = 1) in vec2 vertTexCoord;

layout (location = 0) out vec4 fragColour;

layout (binding = 1) uniform sampler2D texSampler;

void main()
{
    fragColour = texture(texSampler, vertTexCoord);
}
