#version 330 core

in vec2 UV;
out vec4 FragColor;

uniform samplerCube uSkybox;

// Inverse matrices
uniform mat4 uProjection;
uniform mat4 uView;

void main()
{
    mat4 invProj = inverse(uProjection);
    mat4 invView = inverse(uView);

    invView = mat4(mat3(invView));

    // Convert UV --> NDC (-1 to 1)
    vec2 ndc = UV * 2.0 - 1.0;

    // Clip space ray
    vec4 clip = vec4(ndc, 1.0, 1.0);

    // View space ray
    vec4 view = invProj * clip;
    view = vec4(view.xy, -1.0, 0.0);

    // World space direction
    vec3 dir = normalize((invView * view).xyz);

    // Sample cubemap
    FragColor = texture(uSkybox, dir);
}