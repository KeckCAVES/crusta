#extension GL_EXT_gpu_shader4   : enable
#extension GL_EXT_texture_array : enable

uniform sampler2D      symbolTex;
uniform sampler2D      lineDataTex;
uniform sampler2DArray lineCoverageTex;

uniform float lineStartCoord;
uniform float lineCoordStep;

uniform int lineNumSegments;
uniform float lineSymbolLength;
uniform float lineSymbolWidth;

uniform vec3 center;

varying vec3 position;
varying vec3 normal;
varying vec2 texCoord;

const float EPSILON = 0.00001;

#define USE_COVERAGE 0
#define NORMAL   1
#define TWIST    1
#define COVERAGE 0
#define COLORIZE_COVERAGE 0

///\todo move to uniform specification
#define U_SCALE 1.0
#define V_SCALE 1.0



##DataSourceCode##



struct Segment
{
    vec4 symbol;
    vec4 start;
    vec4 end;
    vec3 normal;
};

vec4 read(inout float coord)
{
    vec4 r = ##sampleLineData#coord##;
    coord += lineCoordStep;
    return r;
}

Segment readSegment(inout float coord)
{
    Segment segment;
    segment.symbol = read(coord);
    segment.start  = read(coord);
    segment.end    = read(coord);
    segment.normal = read(coord).xyz;
    return segment;
}

void computeSegment(in Segment segment, inout vec4 color)
{
//- compute lateral coordinate v

    //compute v without taking distortion into account:
    float v = dot(position - segment.start.xyz, segment.normal);

#if TWIST
    // Correct for distortion by dividing by sin(alpha):
    float snn = dot(segment.normal, normal);
    v = v / sqrt(1.0 - snn * snn);
#endif //TWIST

    // Scale v to normalize to the segment width
    v /= V_SCALE*lineSymbolWidth;

    //- Reject if v<-1 or v>1:
    if(abs(v) <= 1.0)
    {
    //- Compute longitudinal coordinate u using simplified formula:

        // Compute the normal vector of the plane containing center and
        // position and being orthogonal to the wedge:
        vec3 b = cross(segment.normal, position + center);

        // Compute the plane's intersection ration with the line's skeleton:
        vec3 startToEnd = segment.end.xyz - segment.start.xyz;
        float u = (dot(position, b) - dot(segment.start.xyz, b)) /
                  dot(startToEnd, b);

        //- Reject if u<0 or u>1:
        if(u>=0.0 && u<=1.0)
        {
#if NORMAL
            //compute the u coordinate as normal projection
            vec3  startToPos = position  - segment.start.xyz;
            float sqrLen     = dot(startToEnd, startToEnd);
            u                = dot(startToPos, startToEnd);
            u               /= sqrLen;
#endif //NORMAL

            //compute the u coordinate wrt the length of the segment
            u  = mix(segment.start.w, segment.end.w, u);
            u *= U_SCALE*lineSymbolLength;
            u  = fract(u / segment.symbol.b);

            //fetch the color contribution from the texture atlas
            vec2 symbolCoord = vec2(u, v);
            symbolCoord     *= segment.symbol.ba;
            symbolCoord     += segment.symbol.rg;
            vec4 symbolColor = texture2D(symbolTex, symbolCoord);

            //accumulate the contribution
            color = mix(color, symbolColor, symbolColor.w);
        }
    }
}


void render(inout vec4 fragColor)
{
    //does this node contain any lines?
    if (lineNumSegments == 0)
        return;

    vec2 coverage = ##sampleCoverage#texCoord##.rg;
#if COVERAGE
    if (coverage.g < 0.5)
    {
        if (coverage.g > 0.0)
            fragColor.rgb = vec3(0.2, 0.8, 0.4);
        else
            fragColor.rgb = vec3(0.0);
    }
    else
        fragColor.rgb = vec3(0.8, 0.4, 0.2);
    fragColor.a   = 1.0;
    return;
#endif

#if USE_COVERAGE
    //do lines overlap this fragment?
    if (coverage == vec2(0.0))
        return;
#endif //USE_COVERAGE

    vec4 color = vec4(0.0);

#if USE_COVERAGE
    //optimize for single coverage
    if (coverage.g < 0.5)
    {
        vec2 coordShift = vec2(255.0, 255.0*256.0);

        vec2 off         = coverage * coordShift;
        off.x           -= 64.0*256.0;
        float segmentOff = off.x + off.y;

        //read in the segment data
        segmentOff      = lineStartCoord + segmentOff*lineCoordStep;
        Segment segment = readSegment(segmentOff);

        //compute segment and done
#if COLORIZE_COVERAGE
        color = vec4(0.2, 0.8, 0.4, 0.2);
#endif //COLORIZE_COVERAGE
        computeSegment(segment, color);
    }
    else
#endif //USE_COVERAGE
    {
#if COLORIZE_COVERAGE
        color = vec4(0.8, 0.4, 0.2, 0.2);
#endif //COLORIZE_COVERAGE
        //walk all the line segments for the node of the fragment

        float coord = lineStartCoord;
        for (int i=0; i<lineNumSegments; ++i)
        {
            Segment segment = readSegment(coord);
            computeSegment(segment, color);
        }
    }

    fragColor = mix(fragColor, color, color.w);
}
