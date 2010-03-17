uniform float lineStartCoord;
uniform float lineCoordStep;
uniform float lineWidth;

uniform vec3 center;

uniform sampler1D lineDataTex;
uniform sampler2D symbolTex;

varying vec3 position;
varying vec3 normal;

vec4 read(inout float coord)
{
    vec4 r = texture1D(lineDataTex, coord);
    coord += lineCoordStep;
    return r;
}

void main()
{
    //setup the default color to whatever the vertex program computed
    gl_FragColor = gl_Color;

    float coord = lineStartCoord;

    if (coord == 0.0)
        return;

    //walk all the line bits for the node of the fragment
    vec4 color      = vec4(0.0, 0.0, 0.0, 0.0);
    int lineBitsMax = int(read(coord).r) - 1;

    for (int i=0; i<128; ++i)
    {
        vec4 symbolOS = read(coord);

        int segmentsMax = int(read(coord).r) - 1;
        for (int j=0; j<128; ++j)
        {
            vec4 startCP = read(coord);
            vec4 endCP   = read(coord);

            vec3 toPos      = position  - startCP.xyz;
            vec3 startToEnd = endCP.xyz - startCP.xyz;

            //compute the u coordinate along the segment
            float sqrLen = dot(startToEnd, startToEnd);
            float u      = dot(toPos, startToEnd) / sqrLen;

            if (u>=0.0 && u<=1.0)
            {
///\todo the section normal could be passed in to improve precision
                //compute normal to the section described by the segment
                vec3 sectionNormal  = cross(startCP.xyz+center,
                                            endCP.xyz+center);
                sectionNormal       = normalize(sectionNormal);

                //compute the line tangent
                vec3 tangent = cross(normal, normalize(startToEnd));
                float tangentLen = length(tangent);
#if 1
                if (tangentLen<0.00001)
#else
                if (true)
#endif
                    tangent = lineWidth*sectionNormal;
                else
                    tangent *= lineWidth;

                //intersect ray from position along tangent with section
#if 1
                float sectionOffset = -dot(startCP.xyz, sectionNormal);
                float nume  = dot(position, sectionNormal) + sectionOffset;
                float denom = dot(tangent, sectionNormal);
                float v     = nume / denom;
#else
                //compute the v coordinate along the tangent
                toPos   = startCP.xyz + u*startToEnd;
                toPos   = position - toPos;
                float v = dot(toPos, tangent) / (lineWidth*lineWidth);
#endif

                if (v>=-1.0 && v<=1.0)
                {
//color = vec4(vec3(v), 0.3);
#if 1
                    //compute the scaled u coordinate
                    u = mix(startCP.w, endCP.w, u);
                    //fetch the color contribution from the texture atlas
                    vec2 symbolCoord = vec2(u, v);
                    symbolCoord     *= symbolOS.ba;
                    symbolCoord     += symbolOS.rg;
                    vec4 symbolColor = texture2D(symbolTex, symbolCoord);
                    //accumulate the contribution
                    color = mix(color, symbolColor, symbolColor.w);
#endif
                }
            }

            if (j == segmentsMax)
                break;
        }

        if (i == lineBitsMax)
            break;
    }

    gl_FragColor = mix(gl_FragColor, color, color.w);
}
