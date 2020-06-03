#version 330 core
out vec4 FragColor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

uniform int lightType;
uniform vec3 lightPosition;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform float lightRange;

uniform vec2 camViewPort;

uniform vec3 viewPos;

void main(void)
{
    vec3 res = vec3(0.0);
    vec3 final_color = vec3(0.0);

    vec2 TexCoords = gl_FragCoord.xy/camViewPort;

    // retrieve data from G-buffer
    vec3 fragPos = texture(gPosition, TexCoords).rgb;
    vec3 norm = texture(gNormal, TexCoords).rgb;//*2-vec3(1.0);
    vec3 albedo = texture(gAlbedoSpec, TexCoords).rgb;

    vec3 view_direction = normalize(viewPos - fragPos);

    if(lightType == 0)
    {
        vec3 direction = normalize(lightPosition - fragPos);

        //diffuse shading
        float diff = max(dot(norm, direction),0.0);

        //specularo shadoringoru
        vec3 half_way = normalize(direction+view_direction);
        float spec = pow(max(dot(half_way,norm),0.0),32.0);

        //attenuation (cutre, linear)
        float distance = length(lightPosition-fragPos);
//      float attenuation = max(1.0-(distance/lightRange),0.0);
//      attenuation = pow(attenuation,2.0);
        float attenuation = (1.0-smoothstep(0,lightRange,distance));

        vec3 diffuse_res =lightColor *diff * albedo;
        diffuse_res *= attenuation;
        vec3 specular_res = spec * lightColor;
        specular_res *= attenuation;
        final_color =  ( diffuse_res + specular_res)+(0.05 * albedo);
    }
    else
    {
        vec3 direction = normalize(-lightDirection);

        //diffuse shading
        float diff = max(dot(norm, direction),0.0);

        //specular shading
        vec3 half_way = normalize(direction+view_direction);
        float spec = pow(max(dot(half_way,norm),0.0),32.0);

        vec3 diffuse_res = lightColor*diff * albedo;
        vec3 specular_res = spec * lightColor;
        final_color = (diffuse_res + specular_res)+(0.05 * albedo);
    }

    res = final_color;
    FragColor=vec4(res,1.0);
}
