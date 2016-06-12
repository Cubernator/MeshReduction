#version 330

uniform vec4 color;

in vec3 vNormal;
in vec3 vLightVec;

void main(void)
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(vLightVec);

    float NdotL = clamp(dot(N, L), 0.0, 1.0);

    gl_FragColor = color * NdotL;
}
