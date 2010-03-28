uniform float lineStartCoord;
uniform float lineCoordStep;

uniform float lineCoordScale;
uniform float lineWidth;

uniform vec3 center;

uniform sampler1D lineDataTex;
uniform sampler2D lineCoverageTex;
uniform sampler2D symbolTex;

varying vec3 position;
varying vec3 normal;
varying vec2 texCoord;

const float EPSILON = 0.00001;

vec4 read(inout float coord)
{
    vec4 r = texture1D(lineDataTex, coord);
    coord += lineCoordStep;
    return r;
}

#define NORMAL   1
#define TWIST    1
#define COVERAGE 0
#define COLORIZE_COVERAGE 1

void computeLine(in vec4 symbolOS, in vec4 startCP, in vec4 endCP,
                 in vec3 sectionNormal, inout vec4 color)
{
    vec3 startToEnd = endCP.xyz - startCP.xyz;

//- radially intersect segment to compute containment
    //project position into the section
    vec3 projP = position - dot(position, sectionNormal)*sectionNormal;

    vec3 toOrig = -(position + center);
    vec3 w0     = startCP.xyz - projP;

    float a = dot(startToEnd, startToEnd);
    float b = dot(startToEnd, toOrig);
    float c = dot(toOrig, toOrig);
    float d = dot(startToEnd, w0);
    float e = dot(toOrig, w0);

    float u = a*c - b*b;
    u       = u<EPSILON ? -1.0 : (b*e - c*d) / u;

    if (u>=0.0 && u<=1.0)
    {
#if NORMAL
    //- use normal projection on the line segment instead for the u
        vec3 startToPos = position  - startCP.xyz;

        //compute the u coordinate along the segment
        float sqrLen = dot(startToEnd, startToEnd);
        float u      = dot(startToPos, startToEnd);
        u           /= sqrLen;
#endif

        //compute the line tangent
        vec3 tangent = cross(normal, normalize(startToEnd));
        float tangentLen = length(tangent);
#if TWIST
        if (tangentLen<EPSILON)
            tangent = -lineWidth*sectionNormal;
        else
            tangent *= 3.0*lineWidth;
#else
        tangent = -lineWidth*sectionNormal;
#endif

        //intersect ray from position along tangent with section
        float sectionOffset = -dot(startCP.xyz, sectionNormal);
        float nume  = dot(position, sectionNormal) + sectionOffset;
        float denom = dot(tangent, sectionNormal);
        float v     = nume / denom;

        if (v>=-1.0 && v<=1.0)
        {
//color = vec4(vec3(v), 0.3);
            //compute the u coordinate wrt the length of the segment
            u  = mix(startCP.w, endCP.w, u);
            u *= 0.15*lineCoordScale;
            u  = fract(u/symbolOS.b);

            //fetch the color contribution from the texture atlas
            vec2 symbolCoord = vec2(u, v);
            symbolCoord     *= symbolOS.ba;
            symbolCoord     += symbolOS.rg;
            vec4 symbolColor = texture2D(symbolTex, symbolCoord);

            //accumulate the contribution
            color = mix(color, symbolColor, symbolColor.w);
        }
    }
}

void main()
{
    //setup the default color to whatever the vertex program computed
    gl_FragColor = gl_Color;

    float coord = lineStartCoord;

    //does this node contain any lines?
    if (coord == 0.0)
        return;

    vec4 coverage = texture2D(lineCoverageTex, texCoord);
#if COVERAGE
    if (coverage.g < 0.5)
    {
        if (coverage.r > 0.0)
            gl_FragColor.rgb = vec3(0.2, 0.8, 0.4);
        else
            gl_FragColor.rgb = vec3(0.0);
    }
    else
        gl_FragColor.rgb = vec3(0.8, 0.4, 0.2);
    gl_FragColor.a   = 1.0;
    return;
#endif

    //do lines overlap this fragment?
    if (coverage == vec4(0.0))
        return;

    vec4 color = vec4(0.0);

    //optimize for single coverage
    if (coverage.g < 0.5)
    {
        vec2 coordShift = vec2(255.0, 255.0*256.0);

        vec2 off        = coverage.rg * coordShift;
        off.x          -= 64.0*256.0;
        float symbolOff = off.x + off.y;

        off              = coverage.ba * coordShift;
        float segmentOff = off.x + off.y;

        //read in the symbol
        coord         = lineStartCoord + symbolOff*lineCoordStep;
        vec4 symbolOS = texture1D(lineDataTex, coord);

        //read in the segment data
        coord              = lineStartCoord + segmentOff*lineCoordStep;
        vec4 startCP       = read(coord);
        vec4 endCP         = read(coord);
        vec3 sectionNormal = read(coord).xyz;

        //compute line and done
#if COLORIZE_COVERAGE
        color = vec4(0.2, 0.8, 0.4, 0.2);
#endif //COLORIZE_COVERAGE
        computeLine(symbolOS, startCP, endCP, sectionNormal, color);
    }
    else
    {
#if COLORIZE_COVERAGE
        color = vec4(0.8, 0.4, 0.2, 0.2);
#endif //COLORIZE_COVERAGE
        //walk all the line bits for the node of the fragment
        int lineBitsMax = int(read(coord).r) - 1;

        for (int i=0; i<128; ++i)
        {
            vec4 symbolOS = read(coord);

            int segmentsMax = int(read(coord).r) - 1;
            for (int j=0; j<128; ++j)
            {
                vec4 startCP       = read(coord);
                vec4 endCP         = read(coord);
                vec3 sectionNormal = read(coord).xyz;

                computeLine(symbolOS, startCP, endCP, sectionNormal, color);

                if (j == segmentsMax)
                    break;
            }

            if (i == lineBitsMax)
                break;
        }
    }

    gl_FragColor = mix(gl_FragColor, color, color.w);
}
