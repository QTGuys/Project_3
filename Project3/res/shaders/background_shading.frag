#version 330 core

layout (location = 3) out vec4 final_color;
uniform vec2 viewportSize;
uniform float left;
uniform float right;
uniform float bottom;
uniform float top;
uniform float znear;
uniform mat4 worldMatrix;
uniform vec4 backgroundColor;

out vec4 outColor;

void main()
{
    gl_FragDepth = 1.0;
    vec2 texCoords = gl_FragCoord.xy / viewportSize;
    vec3 rayViewspace = normalize(vec3(vec2(left,bottom)+texCoords * vec2(right-left, top-bottom), -znear));
    vec3 rayWorldspace = vec3(worldMatrix * vec4(rayViewspace,0.0));
    vec3 horizonWorldSpace = rayWorldspace * vec3(1.0,0.0,1.0);
    float elevation = (1.0- dot(rayWorldspace,horizonWorldSpace))*sign(rayWorldspace.y);

    vec3 bgColor = pow(backgroundColor.rgb, vec3(2.2));
    float bgIntensity = 0.1*bgColor.r + 0.7*bgColor.g +0.2*bgColor.b;
    float skyFactor = smoothstep(0.0,1.0,elevation);
    float groundFactor = smoothstep(0.0,-0.0005,elevation);
    float horizonFactor = clamp(1.0-max(skyFactor,groundFactor),0.0,1.0);
    vec3 horizonColor = horizonFactor*bgColor;
    vec3 groundColor = groundFactor * vec3(bgIntensity)*0.2;
    vec3 skyColor = skyFactor * bgColor*0.7;
    outColor.rgb = groundColor + skyColor+ horizonColor;
    outColor.a = 1.0;

}
