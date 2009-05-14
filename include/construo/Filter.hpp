BEGIN_CRUSTA

template <typename PixelParam>
PixelParam Filter::
lookup(PixelParam* at, uint rowLen)
{
    PixelParam res(0);

    for (int y=-width; y<=width; ++y)
    {
        PixelParam* atY = at + y*rowLen;
        for (int x=-width; x<=width; ++x)
        {
            PixelParam* atYX = atY + x;
            res += (*atYX) * weights[y]*weights[x];
        }
    }

    return res;
}


END_CRUSTA
