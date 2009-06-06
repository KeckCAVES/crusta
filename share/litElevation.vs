uniform sampler2D geometryTex;
uniform sampler2D heightTex;
uniform sampler2D colorTex;

uniform float texStep;
uniform float verticalScale;
uniform vec3  centroid;

vec3 surfacePoint(in vec2 coords)
{
    vec3 res      = texture2D(geometryTex, coords).rgb;
    vec3 dir      = normalize(centroid + res);
    float height  = texture2D(heightTex, coords).r;
    height       *= verticalScale;
    res          += height * dir;
    return res;
}

void main()
{
    vec2 coord    = gl_Vertex.xy;
    vec3 me       = surfacePoint(coord);
    vec4 meInEye  = gl_ModelViewMatrix * vec4(me, 1.0);
    gl_Position   = gl_ModelViewProjectionMatrix * vec4(me, 1.0);

    vec3 up    = surfacePoint(vec2(coord.x, coord.y + texStep));
    vec3 down  = surfacePoint(vec2(coord.x, coord.y - texStep));
    vec3 left  = surfacePoint(vec2(coord.x - texStep, coord.y));
    vec3 right = surfacePoint(vec2(coord.x + texStep, coord.y));

    vec3 normal   = cross((right - left), (up - down));
    normal        = normalize(gl_NormalMatrix * normal);

    vec3 lightDir = (gl_LightSource[0].position.xyz * meInEye.w) -
                    (meInEye.xyz * gl_LightSource[0].position.w);
    lightDir      = normalize(lightDir);

    float intensity = max(dot(normal, lightDir), 0.2);

    vec4  color     = texture2D(colorTex, coord);
#if 1
	gl_FrontColor =  color * intensity;
#else
    float single;
    single = texture2D(heightTex, coord).r / 1000.0;
//    single = abs(verticalScale);// / 1.0;
//    single = intensity;
//    single = texture2D(heightTex, gl_Vertex.xy).r / 1000.0;

	gl_FrontColor =  vec4(single, single, single, 1.0);

//    gl_FrontColor = vec4(texture2D(geometryTex, coord).rgb, 1.0);
#endif
}
