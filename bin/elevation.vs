
const float textureStep = 1.0 / 4.0;
const float normalZ     = 6.0 * textureStep;
const vec3  sobelCoeff  = vec3(1.0, 2.0, 1.0);

//can optimize out the normalize
//vpNormalZ = 6.0f/(float)(quadRes - 1);


uniform sampler2D heights;

void main()
{
    vec3 heightPalette[3];
    heightPalette[0] = vec3(0.85, 0.83, 0.66);
    heightPalette[1] = vec3(0.80, 0.21, 0.28);
    heightPalette[2] = vec3(0.29, 0.37, 0.50);

    float samples[9];
    samples[0] = texture2D(heights, vec2(gl_MultiTexCoord0.x - textureStep,
                                         gl_MultiTexCoord0.y + textureStep)).r;
    samples[1] = texture2D(heights, vec2(gl_MultiTexCoord0.x,
                                         gl_MultiTexCoord0.y + textureStep)).r;
    samples[2] = texture2D(heights, vec2(gl_MultiTexCoord0.x + textureStep,
                                         gl_MultiTexCoord0.y + textureStep)).r;
    samples[3] = texture2D(heights, vec2(gl_MultiTexCoord0.x - textureStep,
                                         gl_MultiTexCoord0.y)).r;
    samples[4] = texture2D(heights, vec2(gl_MultiTexCoord0.x,
                                         gl_MultiTexCoord0.y)).r;
    samples[5] = texture2D(heights, vec2(gl_MultiTexCoord0.x + textureStep,
                                         gl_MultiTexCoord0.y)).r;
    samples[6] = texture2D(heights, vec2(gl_MultiTexCoord0.x - textureStep,
                                         gl_MultiTexCoord0.y - textureStep)).r;
    samples[7] = texture2D(heights, vec2(gl_MultiTexCoord0.x,
                                         gl_MultiTexCoord0.y - textureStep)).r;
    samples[8] = texture2D(heights, vec2(gl_MultiTexCoord0.x + textureStep,
                                         gl_MultiTexCoord0.y - textureStep)).r;

    vec3 tmp;
    float dx;
    tmp = vec3(samples[0], samples[3], samples[6]);
    dx  = dot(tmp, -sobelCoeff);
    tmp = vec3(samples[2], samples[5], samples[8]);
    dx += dot(tmp, sobelCoeff);
    
    float dy;
    tmp = vec3(samples[6], samples[7], samples[8]);
    dy  = dot(tmp, -sobelCoeff);
    tmp = vec3(samples[0], samples[1], samples[2]);
    dy += dot(tmp, sobelCoeff);
    
    vec3 normal = vec3(dx, dy, normalZ);
    normalize(normal);

    vec4 position  = vec4(samples[4] * gl_Vertex.xyz, 1.0);
    
    gl_Position    = gl_ModelViewProjectionMatrix * position;
//    gl_FrontColor  = gl_Color;
    
    float alpha = (samples[4] - 1.0) * 4.0;
    int low  = 0;
    int high = 0;
    if (alpha < 0.5)
    {
        low = 0; high = 1;
        alpha *= 2.0;
    }
    else
    {
        low = 1; high = 2;
        alpha = (alpha - 0.5) * 2.0;
    }
//    gl_FrontColor = vec4(alpha, alpha, alpha, 1.0);

    gl_FrontColor  = vec4(((1.0-alpha) * heightPalette[low]) +
                          (alpha       * heightPalette[high]), 1.0);
}
