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
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

in vec2 texCoord;
out vec4 outColor;


vec4 Background()
{
    vec4 ret;

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
    ret.rgb = groundColor + skyColor+ horizonColor;
    ret.a = 1.0;

    return ret;
}

vec4 Grid(vec3 hitPoint, vec3 gridSteps)
{
    vec4 ret;

    vec2 grid_1 = fwidth(hitPoint.xz)/mod(hitPoint.xz,gridSteps.x);
    float line_1 = step(1.0,max(grid_1.x,grid_1.y));
    vec2 grid_2 = fwidth(hitPoint.xz)/mod(hitPoint.xz,gridSteps.y);
    float line_2 = step(1.0,max(grid_2.x,grid_2.y));
    vec2 grid_3 = fwidth(hitPoint.xz)/mod(hitPoint.xz,gridSteps.z);
    float line_3 = step(1.0,max(grid_3.x,grid_3.y));

    if(line_1==1.0)
    {
        ret=vec4(1.0,0.0,1.0,1.0);

        if(line_2==1.0)
        {
            ret=vec4(1.0,0.0,0.0,1.0);

            if(line_3==1.0)
            {
                ret=vec4(1.0,1.0,1.0,1.0);
            }
        }

    }
    else
    {
        ret=Background();
    }

    return ret;
}


void main()
{
    //Eye direction using camera parameters
    vec3 eyeDirEyespace;
    eyeDirEyespace.x = left + texCoord.x *(right-left);
    eyeDirEyespace.y = bottom + texCoord.y * (top-bottom);
    eyeDirEyespace.z = -znear;

    //Tranform to camera position
    vec3 eyeDirWorldspace = normalize(mat3(worldMatrix)*eyeDirEyespace); //like this because its a vector and we don't need the transform

    //Eye Position (center)
    vec3 eyePosEyespace=vec3(0.0,0.0,0.0);
    vec3 eyePosWorldspace = vec3(worldMatrix*vec4(eyePosEyespace,1.0)); //like this because we need the transformation

    //Plane parameters
    vec3 planeNormal = vec3(0.0,1.0,0.0);
    vec3 planePosition = vec3(0.0,0.0,0.0);

    //Ray-Plane Intersection
    float numerator = dot(planePosition-eyePosWorldspace, planeNormal);
    float denominator = dot(eyeDirWorldspace,planeNormal);
    float t= numerator/denominator;

    if(t>0.0)
    {
        vec3 hitWorldspace=eyePosWorldspace+eyeDirWorldspace*t;

        //Small bias to avoid z-fighting
        vec3 bias = (eyePosWorldspace-hitWorldspace)*0.005;

        //fragment depth
        vec4 hitClip = projectionMatrix*viewMatrix*vec4(hitWorldspace+bias,1.0);
        float ndcDepth = hitClip.z/hitClip.w;
        gl_FragDepth=((gl_DepthRange.diff*ndcDepth)+gl_DepthRange.near + gl_DepthRange.far)/2.0;

        outColor = Grid(hitWorldspace,vec3(1.0,10.0,100.0));
    }
    else
    {
        outColor=Background();
    }
}
