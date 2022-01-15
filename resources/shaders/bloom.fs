#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;


 in vec3 FragPos;
 in vec3 Normal;
 in vec2 TexCoords;

 struct Material {
       sampler2D diffuse;
       sampler2D specular;
       float shininess;
   };

   struct DirLight {
       vec3 direction;

       vec3 ambient;
       vec3 diffuse;
       vec3 specular;
   };


    struct PointLight {
         vec3 position;

         float constant;
         float linear;
         float quadratic;

         vec3 ambient;
         vec3 diffuse;
         vec3 specular;
     };

     struct SpotLight {
         vec3 position;
         vec3 direction;
         float cutOff;
         float outerCutOff;

         float constant;
         float linear;
         float quadratic;

         vec3 ambient;
         vec3 diffuse;
         vec3 specular;
     };

     #define NR_POINT_LIGHTS 2

     uniform vec3 viewPos;
     uniform DirLight dirLight;
     uniform PointLight pointLight1;
     uniform PointLight pointLight2;
     uniform SpotLight spotLight;
     uniform Material material;
     uniform bool spot;

     // function prototypes
     vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
     vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
     vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

 void main()
 {

  vec3 norm = normalize(Normal);
     vec3 viewDir = normalize(viewPos - FragPos);

     // == =====================================================
     // Our lighting is set up in 3 phases: directional, point lights and an optional flashlight
     // For each phase, a calculate function is defined that calculates the corresponding color
     // per lamp. In the main() function we take all the calculated colors and sum them up for
     // this fragment's final color.
     // == =====================================================
     // phase 1: directional lighting
     vec3 result = CalcDirLight(dirLight, norm, viewDir);
     // phase 2: point lights
    // for(int i = 0; i < NR_POINT_LIGHTS; i++)
         result+= CalcPointLight(pointLight1, norm, FragPos, viewDir);
         result += CalcPointLight(pointLight2, norm, FragPos, viewDir);
     // phase 3: spot light
     if(spot)
     result += CalcSpotLight(spotLight, norm, FragPos, viewDir);

     FragColor = vec4(result, 1.0);

         float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
         if(brightness > 1.0)
             BrightColor = vec4(result, 1.0);
         else
             BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
         FragColor = vec4(result, 1.0);
 }

 vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
 {

     vec3 lightDir = normalize(-light.direction);
     // diffuse shading
     float diff = max(dot(normal, lightDir), 0.0);
     // specular shading
     vec3 reflectDir = reflect(-lightDir, normal);

     vec3 halfway=normalize(lightDir+viewDir);
     float spec = pow(max(dot(normal, halfway), 0.0), material.shininess+32.0f);


     vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords).rgb);
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords).rgb);
    vec3 specular = light.specular * spec*vec3(texture(material.specular, TexCoords).rgb);

    return diffuse+specular+ambient;

 }



vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{

   vec3 lightDir = normalize(light.position - FragPos);
    // diffuse shading

    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfway=normalize(lightDir+viewDir);
   float spec = pow(max(dot(normal, halfway), 0.0), material.shininess+32.0f);
    // attenuation
     float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results

     vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords).rgb);
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords).rgb);
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords).rgb);
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return ( diffuse + specular+ambient);

}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
     vec3 halfway=normalize(lightDir+viewDir);
    float spec = pow(max(dot(normal, halfway), 0.0),material.shininess+32.0f);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity =clamp((theta - light.outerCutOff) / epsilon,0,1);
    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords).rgb);
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords).rgb);
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords).rgb);
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return ( diffuse + specular+ambient);
}