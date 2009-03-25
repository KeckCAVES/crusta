uniform sampler2D geometryTex;
uniform sampler2D heightTex;
uniform sampler2D colorTex;

void main()
{
    vec3  toSurface = texture2D(geometryTex, gl_Vertex.xy).rgb;
    float height    = texture2D(heightTex, gl_Vertex.xy).r;
    vec4  color     = texture2D(colorTex, gl_Vertex.xy);

//    vec4 position = vec4(toSurface, 1.0);
    vec4 position = vec4(height * toSurface, 1.0);
    
    gl_Position    = gl_ModelViewProjectionMatrix * position;
    gl_FrontColor  = color;
}
