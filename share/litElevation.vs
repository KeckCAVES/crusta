uniform sampler2D geometryTex;
uniform sampler2D heightTex;
uniform sampler2D colorTex;

uniform float texStep;
uniform vec3 centroid;

vec3 surfacePoint(in vec2 coords)
{
    vec3 res     = texture2D(geometryTex, coords).rgb;
    vec3 dir     = normalize(centroid + res);
    float height = texture2D(heightTex, coords).r;
///\todo removeZ once the preprocessor is cleaned-up then this can eliminated
//height       = height>9000.0 ? 0.0 : height;
height *= 2000.0;
    res         += height * dir;
    return res;
}

void main()
{
    vec3 me     = surfacePoint(gl_Vertex.xy);
    gl_Position = gl_ModelViewProjectionMatrix * vec4(me, 1.0);

    vec3 up    = surfacePoint(vec2(gl_Vertex.x, gl_Vertex.y + texStep));
    vec3 down  = surfacePoint(vec2(gl_Vertex.x, gl_Vertex.y - texStep));
    vec3 left  = surfacePoint(vec2(gl_Vertex.x - texStep, gl_Vertex.y));
    vec3 right = surfacePoint(vec2(gl_Vertex.x + texStep, gl_Vertex.y));

    vec3 normal   = cross((right - left), (up - down));
    normal        = normalize(gl_NormalMatrix * -normal);

    vec3 lightDir = (gl_LightSource[0].position.xyz * gl_Position.w) -
                    (gl_Position.xyz * gl_LightSource[0].position.w);
    lightDir      = normalize(lightDir);

    float intensity = max(dot(normal, lightDir), 0.0);

    vec4  color   = texture2D(colorTex, gl_Vertex.xy);
#if 1
	gl_FrontColor =  color * intensity;
#else
    float single;
    single = intensity;
//    single = texture2D(heightTex, gl_Vertex.xy).r / 1000.0;

	gl_FrontColor =  vec4(single, single, single, 1.0);
#endif
}
