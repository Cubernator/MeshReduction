#version 330

uniform mat4 mvp;
uniform mat4 world;
uniform vec3 lightPos;

layout(location = 0) in vec3 aVertex;
layout(location = 1) in vec3 aNormal;

out vec3 vNormal;
out vec3 vLightVec;

void main()
{
    vec4 v = vec4(aVertex, 1.0);
    vec4 pos = world * v;
    vec4 normal = world * vec4(aNormal, 0.0);

    gl_Position = mvp * v;

    vNormal = normal.xyz;
    vLightVec = lightPos - pos.xyz;
}
