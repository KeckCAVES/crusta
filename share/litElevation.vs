uniform sampler2D geometryTex;
uniform sampler2D heightTex;
uniform sampler2D colorTex;

uniform float texStep;

const float sphereRadius = 6371000.0;

void main()
{
    vec3  toSurface = texture2D(geometryTex, gl_Vertex.xy).rgb;
    float height    = texture2D(heightTex, gl_Vertex.xy).r;
    vec4  color     = texture2D(colorTex, gl_Vertex.xy);

//    vec4 position = vec4((sphereRadius+height) * toSurface, 1.0);
    vec4 position = vec4(sphereRadius * toSurface, 1.0);
//    vec4 position = vec4(toSurface, 1.0);
    gl_Position    = gl_ModelViewProjectionMatrix * position;

    vec2 coord;
    coord      = vec2(gl_Vertex.x, gl_Vertex.y + texStep);
    vec3 up    = texture2D(geometryTex, coord).rgb;
    up        *= sphereRadius;//texture2D(heightTex, coord).r;
    coord      = vec2(gl_Vertex.x, gl_Vertex.y - texStep);
    vec3 down  = texture2D(geometryTex, coord).rgb;
    down      *= sphereRadius;//texture2D(heightTex, coord).r;
    coord      = vec2(gl_Vertex.x - texStep, gl_Vertex.y);
    vec3 left  = texture2D(geometryTex, coord).rgb;
    left      *= sphereRadius;//texture2D(heightTex, coord).r;
    coord      = vec2(gl_Vertex.x + texStep, gl_Vertex.y);
    vec3 right = texture2D(geometryTex, coord).rgb;
    right     *= sphereRadius;//texture2D(heightTex, coord).r;

    vec3 normal   = cross((right - left), (up - down));
    normal        = normalize(gl_NormalMatrix * -normal);
    normal        = normalize(normal);
    vec3 lightDir = (gl_LightSource[0].position.xyz * gl_Position.w) -
                    (gl_Position.xyz * gl_LightSource[0].position.w);
    lightDir      = normalize(lightDir);

    float intensity = max(dot(normal, lightDir), 0.0);
//	gl_FrontColor =  color * intensity;

	gl_FrontColor =  vec4(intensity, intensity, intensity, 1.0);
//	gl_FrontColor =  vec4(height, height, height, 1.0);
//	gl_FrontColor =  vec4(gl_Vertex.x, gl_Vertex.y, 0.0, 1.0);
}
