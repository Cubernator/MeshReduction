#version 330

uniform mat4 mvp;

layout(location = 0) in vec3 aPosition;

void main(void)
{
    gl_Position = mvp * vec4(aPosition, 1.0);
}
