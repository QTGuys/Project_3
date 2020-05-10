#version 330 core

//Camera
uniform vec3 cameraPos;

// Matrices
uniform mat4 worldViewMatrix;
uniform mat3 normalMatrix;

// Material
uniform vec4 albedo;
uniform vec4 specular;
uniform vec4 emissive;
uniform float smoothness;
uniform float bumpiness;
uniform sampler2D albedoTexture;
uniform sampler2D specularTexture;
uniform sampler2D emissiveTexture;
uniform sampler2D normalTexture;
uniform sampler2D bumpTexture;

// Lights
#define MAX_LIGHTS 8
uniform int lightType[MAX_LIGHTS];
uniform vec3 lightPosition[MAX_LIGHTS];
uniform vec3 lightDirection[MAX_LIGHTS];
uniform vec3 lightColor[MAX_LIGHTS];
uniform float lightRange[MAX_LIGHTS];
uniform int lightCount;

in vec2 vTexCoords;
in vec3 aNormal;
in vec3 fragPos;

out vec4 outColor;

void main(void)
{
    vec3 res = vec3(0);
    vec3 directional_color = vec3(0);
    vec3 point_color = vec3(0);

    vec3 norm = normalize(aNormal);
    vec3 view_direction = normalize(cameraPos - fragPos);
    for(int i = 0; i<lightCount;++i)
    {


        if(lightType[i] == 0)
        {
            vec3 direction = normalize(lightPosition[i]- fragPos);

            //diffuse shading
            float diff = max(dot(norm, direction),0.0);

            //specularo shadoringoru
            //vec3 reflect_dir = reflect(-direction,norm);
            //float spec = pow(max(dot(view_direction, reflect_dir),0.0),32.0);
            vec3 half_way = normalize(direction+view_direction);
            float spec = pow(max(dot(half_way,norm),0.0),32.0);


            //attenuation (cutre, linear)
            float distance = length(lightPosition[i]-fragPos);
            float attenuation = max(1.0-(distance/lightRange[i]),0.0);
            attenuation = pow(attenuation,2.0);


            vec3 diffuse_res =lightColor[i] *diff * albedo.rgb * texture(albedoTexture,vTexCoords).rgb;
            diffuse_res *= attenuation;
           vec3 specular_res = spec * specular.rgb*lightColor[i]; //texture(specularTexture,vTexCoords).rgb;
           // vec3 specular_res = vec3(spec);
            specular_res *= attenuation;
            point_color +=   ( diffuse_res+ specular_res);
        }
        else if(lightType[i] == 1)
        {
            vec3 direction = normalize(-lightDirection[i]);

            //diffuse shading
            float diff = max(dot(norm, direction),0.0);

            //specular shading
            //vec3 reflect_direction = reflect(-direction,norm);
            //float spec = pow(max(dot(view_direction, reflect_direction),0.00001),256.0);
            vec3 half_way = normalize(direction+view_direction);
            float spec = pow(max(dot(half_way,norm),0.0),32.0);

            vec3 diffuse_res = lightColor[i]*diff * albedo.rgb * texture(albedoTexture,vTexCoords).rgb;
            vec3 specular_res = spec * specular.rgb * texture(albedoTexture,vTexCoords).rgb;
            directional_color = (diffuse_res + specular_res);
        }
    }
    vec3 ambient = 0.05 * albedo.rgb * texture(albedoTexture,vTexCoords).rgb;
    res = directional_color + point_color + ambient;
    outColor=vec4(res,1.0);
}
