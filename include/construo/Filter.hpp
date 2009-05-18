BEGIN_CRUSTA

template <typename PixelParam>
PixelParam Filter::
lookup(PixelParam* at, uint rowLen)
{
    PixelParam res(0);

    for (int y=-width; y<=static_cast<int>(width); ++y)
    {
        PixelParam* atY = at + y*rowLen;
        for (int x=-width; x<=static_cast<int>(width); ++x)
        {
            PixelParam* atYX = atY + x;
            res += (*atYX) * weights[y]*weights[x];
        }
    }

    return res;
}


END_CRUSTA
