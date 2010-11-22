#ifndef _TextureColorPixelOps_H_
#define _TextureColorPixelOps_H_


#include <crusta/PixelOps.h>

#include <crusta/TextureColor.h>


BEGIN_CRUSTA


template <>
struct PixelOps<TextureColor>
{
    static TextureColor average(const TextureColor& a, const TextureColor& b,
                                const TextureColor& nodata)
    {
        if (a==nodata)
            return b;
        else if (b==nodata)
            return a;
        
        TextureColor res(0,0,0);
        for (int i=0; i<TextureColor::dimension; ++i)
            res[i] = (((float)a[i] + (float)b[i]) * 0.5f) + 0.5f;
        return res;
    }

    static TextureColor average(const TextureColor& a, const TextureColor& b,
                                const TextureColor& c, const TextureColor& d,
                                const TextureColor& nodata)
    {
        const TextureColor p[4] = {a,b,c,d};

        float res[3] = {0.0f, 0.0f, 0.0f};
        float sumWeights = 0.0f;
        for (int i=0; i<4; ++i)
        {
            if (p[i] != nodata)
            {
                for (int j=0; j<TextureColor::dimension; ++j)
                    res[j] += (float)p[i][j] * 0.25f;
                sumWeights += 0.25f;
            }
        }

        if (sumWeights == 0.0f)
            return nodata;

        TextureColor ret;
        for (int i=0; i<TextureColor::dimension; ++i)
            ret[i] = (res[i] / sumWeights) + 0.5f;

        return ret;
    }

    static TextureColor minimum(const TextureColor& a, const TextureColor& b,
                                const TextureColor& nodata)
    {
        if (a==nodata)
            return b;
        else if (b==nodata)
            return a;

        TextureColor res(0,0,0);
        for (int i=0; i<TextureColor::dimension; ++i)
            res[i] = std::min(a[i], b[i]);
        return res;
    }

    static TextureColor maximum(const TextureColor& a, const TextureColor& b,
                                const TextureColor& nodata)
    {
        if (a==nodata)
            return b;
        else if (b==nodata)
            return a;

        TextureColor res(0,0,0);
        for (int i=0; i<TextureColor::dimension; ++i)
            res[i] = std::max(a[i], b[i]);
        return res;
    }
};


END_CRUSTA


#endif //_TextureColorPixelOps_H_
