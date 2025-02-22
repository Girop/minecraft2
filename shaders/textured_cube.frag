#version 450

layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout (location = 0) in vec2 texCoord;

layout(location = 0) out vec4 fragColor;

void main()
{
    vec3 color = texture(textureSampler, texCoord).xyz;
    fragColor = vec4(color, 1.0);
}
