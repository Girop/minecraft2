#version 450

layout(set = 0, binding = 0) uniform UniformBufferObj {
    mat4 model;
    mat4 view;
    mat4 projection;
} camera;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec2 textureCoord;

void main()
{
    gl_Position = camera.projection * camera.view * camera.model * vec4(position, 1.0);
    textureCoord = uv;
}
