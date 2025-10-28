#version 430 core
out vec4 fragColor;

in vec2 texCoords;

uniform sampler2D texA;
uniform sampler2D texB;
uniform int blendMode;

vec4 blendAdd(vec4 a, vec4 b) {
    return a + b;
}

vec4 blendMultiply(vec4 a, vec4 b) {
    return a * b;
}

vec4 blendScreen(vec4 a, vec4 b) {
    return 1.0 - (1.0 - a) * (1.0 - b);
}

vec4 blendNormal(vec4 a, vec4 b) {
    return mix(a, b, b.a);
}

void main()
{
    vec4 colorA = texture(texA, texCoords);
    vec4 colorB = texture(texB, texCoords);

    vec4 result;

    switch (blendMode) {
        case 0:
            result = blendAdd(colorA, colorB);
            break;
        case 1:
            result = blendMultiply(colorA, colorB);
            break;
        case 2:
            result = blendScreen(colorA, colorB);
            break;
        case 3:
        default:
            result = blendNormal(colorA, colorB);
            break;
    }

    fragColor = result;
}