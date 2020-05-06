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

void main()
{
    // retrieve data from G-buffer
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;

    vec3 lighting = vec3(0.0);
    vec3 viewDir = normalize(viewPos - FragPos);


    for(int i = 0; i < lightCount; ++i)
    {
        // then calculate lighting as usual
        lighting = Albedo * 0.1; // hard-coded ambient component

        // diffuse
        vec3 lightDir = normalize(lightPosition[i] - FragPos);
        vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Albedo * lightColor[i];

        lighting += diffuse;

        //specularo shadoringoru
        vec3 reflect_dir = reflect(-lightDir,Normal);
        float spec = pow(max(dot(viewDir, reflect_dir),0.0),256.0);

        lighting += vec3(spec);
    }

    FragColor = vec4(lighting,1.0);
}
