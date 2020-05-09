#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

#define MAX_LIGHTS 8
uniform int lightType[MAX_LIGHTS];
uniform vec3 lightPosition[MAX_LIGHTS];
uniform vec3 lightDirection[MAX_LIGHTS];
uniform vec3 lightColor[MAX_LIGHTS];
uniform float lightRange[MAX_LIGHTS];
uniform int lightCount;

uniform vec3 viewPos;

//void main()
//{
//    // retrieve data from G-buffer
//    vec3 FragPos = texture(gPosition, TexCoords).rgb;
//    vec3 Normal = texture(gNormal, TexCoords).rgb;
//    vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
//    float Specular = texture(gAlbedoSpec, TexCoords).a;

//    vec3 lighting = vec3(0.0);
//    vec3 viewDir = normalize(viewPos - FragPos);


//    for(int i = 0; i < lightCount; ++i)
//    {
//        // then calculate lighting as usual
//        lighting = Albedo * 0.1; // hard-coded ambient component

//        // diffuse
//        vec3 lightDir = normalize(lightPosition[i] - FragPos);
//        vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Albedo * lightColor[i];

//        lighting += diffuse;

//        //specularo shadoringoru
//        vec3 reflect_dir = reflect(-lightDir,Normal);
//        float spec = pow(max(dot(viewDir, reflect_dir),0.0),256.0);

//        lighting += vec3(spec);
//    }

//    FragColor = vec4(lighting,1.0);
//}
void main(void)
{
    vec3 res = vec3(0);
    vec3 directional_color = vec3(0);
    vec3 point_color = vec3(0);

    // retrieve data from G-buffer
    vec3 fragPos = texture(gPosition, TexCoords).rgb;
    vec3 norm = texture(gNormal, TexCoords).rgb;
    vec3 albedo = texture(gAlbedoSpec, TexCoords).rgb;

    vec3 lighting = vec3(0.0);
    vec3 view_direction = normalize(viewPos - fragPos);

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


            vec3 diffuse_res =lightColor[i] *diff * albedo;
            diffuse_res *= attenuation;
           vec3 specular_res = spec * lightColor[i]; //texture(specularTexture,vTexCoords).rgb;
           // vec3 specular_res = vec3(spec);
            //specular_res *= attenuation;
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


            vec3 diffuse_res = lightColor[i]*diff * albedo;
            vec3 specular_res = spec * lightColor[i];
            directional_color = (diffuse_res + specular_res);
        }
    }
   vec3 ambient = 0.05 * albedo;
    res = directional_color + point_color + ambient;
    FragColor=vec4(res,1.0);
}
