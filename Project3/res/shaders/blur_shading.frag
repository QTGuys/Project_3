#version 330 core

uniform sampler2D colorMap;
uniform vec2 direction;
uniform int inputLod;

in vec2 texCoord;

out vec4 outColor;

void main()
{
    vec2 texSize = textureSize(colorMap, inputLod);
    vec2 texelSize = 1.0/texSize;
    vec2 margin1 = texelSize *0.5;
    vec2 margin2 = vec2(1.0)-margin1;

    outColor = vec4(0.0);

    vec2 directionFragCoord = gl_FragCoord.xy * direction;
    int coord= int(directionFragCoord.x + directionFragCoord.y);
    vec2 directionTexSize = texSize * direction;
    int size = int(directionTexSize.x + directionTexSize.y);
    int kernelRadius = 5;
    int kernelBegin = -min(kernelRadius, coord);
    int kernelEnd = min(kernelRadius, size-coord);
    float weight = 0.0;
    for(int i = kernelBegin ; i <= kernelEnd; ++i)
    {
        float currentWeight = smoothstep(float(kernelRadius),0.0,float(abs(i)));
        vec2 finalTexCoord = texCoord+i*direction*texelSize;
        finalTexCoord = clamp(finalTexCoord, margin1,margin2);
        outColor += textureLod(colorMap, finalTexCoord, inputLod)*currentWeight;
        weight+= currentWeight;
    }
    outColor = outColor/weight;
    //outColor = vec4(0.0,1.0,0.0,1.0);
    //outColor =vec4(texCoord,0.0,1.0);
}

//#version 330 core
//uniform sampler2D colorMap;
//uniform vec2 direction;
//uniform int inputLod;

//in vec2 texCoords;

//out vec4 outColor;
//uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

//void main()
//{
//    vec2 tex_offset = 1.0 / textureSize(colorMap, inputLod); // gets size of single texel
//    vec3 result = textureLod(colorMap, texCoords,inputLod).rgb * weight[0]; // current fragment's contribution
//    if(direction.x != 0.0)
//    {
//        for(int i = 1; i < 5; ++i)
//        {
//            result += textureLod(colorMap, texCoords + vec2(tex_offset.x * i, 0.0),inputLod).rgb * weight[i];

//            result += textureLod(colorMap, texCoords - vec2(tex_offset.x * i, 0.0),inputLod).rgb * weight[i];
//        }
//    }
//    else
//    {
//        for(int i = 1; i < 5; ++i)
//        {
//            result += textureLod(colorMap, texCoords + vec2(0.0, tex_offset.y * i),inputLod).rgb * weight[i];
//            result += textureLod(colorMap, texCoords - vec2(0.0, tex_offset.y * i),inputLod).rgb * weight[i];
//        }
//    }
//    outColor = vec4(result, 1.0);
//}


