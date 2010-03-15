#version 150 compatibility

uniform sampler2D depthTex;
uniform sampler2D terrainTex;

uniform float lineStartCoord;
uniform float lineCoordStep;
uniform float lineLastSample;
uniform float lineSkipSamples;

uniform sampler2D lineDataTex;
uniform sampler2D symbolTex;


vec3 unproject(in mat4 inverseMVP, in vec2 fragCoord)
{
    gl_FragDepth = texture(depthTex, fragCoord).r;

    vec4 tmp;
    tmp.xy = (fragCoord * 2.0) - 1.0;
    tmp.z  = gl_FragDepth*2.0 - 1.0;
    tmp.w  = 1.0;

    tmp   = inverseMVP * tmp;
    tmp.w = 1.0 / tmp.w;
    return tmp.xyz * tmp.w;
}

vec4 read(inout vec2 coord)
{
    vec4 r   = texture(lineDataTex, coord);
    coord.x += lineCoordStep;
    return r;
}

void main()
{
    vec2 fragCoord = gl_TexCoord[0].xy;

    vec4 terrainAttribute = texture(terrainTex, fragCoord);
    if (all(equal(terrainAttribute, vec4(1.0))))
        discard;

    //compute the starting coordinate of the line data for the node
    vec2 coordShift = vec2(255.0*lineCoordStep,255.0*256.0*lineCoordStep);
    vec2 coordX = terrainAttribute.rg * coordShift;
    vec2 coordY = terrainAttribute.ba * coordShift;
    vec2 coord  = vec2(coordX.x + coordX.y, coordY.x + coordY.y);
    coord      += vec2(lineStartCoord);

    //read in the inverse model view projection matrix
    mat4 inverseMVP;
    inverseMVP[0] = read(coord);
    inverseMVP[1] = read(coord);
    inverseMVP[2] = read(coord);
    inverseMVP[3] = read(coord);

    //compute the position of the fragment
    vec3 pos = unproject(inverseMVP, fragCoord);

    //walk all the line bits for the node of the fragment
    vec4 color      = vec4(0.0, 0.0, 0.0, 0.0);
    int lineBitsMax = int(read(coord).r) - 1;

    for (int i=0; i<128; ++i)
    {
        vec4 symbolOS = read(coord);

        int segmentsMax = int(read(coord).r) - 1;
        for (int j=0; j<128; ++j)
        {
            vec3 startCP = read(coord).xyz;
            vec3 endCP   = read(coord).xyz;

            vec3 toPos      = pos   - startCP;
            vec3 startToEnd = endCP - startCP;

            //compute the u coordinate along the segment
            float sqrLen = dot(startToEnd, startToEnd);
            float u      = dot(toPos, startToEnd) / sqrLen;

            if (u>=0.0 && u<=1.0)
            {
                 //fetch the sample
                 vec2 sampleCoord = coord;          //first sample coord
                 coord.x         += lineLastSample; //last  sample coord
                 sampleCoord.x    = mix(sampleCoord.x, coord.x, u);
                 vec4 sample      = texture(lineDataTex, sampleCoord);
                 coord.x         += lineCoordStep;

                 //compute the v coordinate along the tangent
                 toPos   = startCP + u*startToEnd;
                 toPos   = pos - toPos;
                 sqrLen  = dot(sample.xyz, sample.xyz);
                 float v = dot(toPos, sample.xyz) / sqrLen;

                 if (v>=-1.0 && v<=1.0)
                 {
                    //accumulate the contribution
                    v = v*0.5 + 0.5;
                    vec2 symbolCoord = vec2(sample.w, v);
                    symbolCoord     *= symbolOS.ba;
                    symbolCoord     += symbolOS.rg;
                    vec4 symbolColor = texture(symbolTex, symbolCoord);
                    color = mix(color, symbolColor, symbolColor.w);
                }
            }
            else
            {
                //simply advance to the next segment
                coord.x += lineSkipSamples;
            }

            if (j == segmentsMax)
                break;
        }

        if (i == lineBitsMax)
            break;
    }

    gl_FragColor = mix(gl_FragColor, color, color.w);
}
