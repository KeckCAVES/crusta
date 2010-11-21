#include <cassert>

#include <construo/ImageTransform.h>

BEGIN_CRUSTA

ImageTransform::
ImageTransform() :
    pointSampled(false)
{
}

void ImageTransform::
setPointSampled(bool isPointSampled, const int* imgSize)
{
    pointSampled = isPointSampled;
    if (!pointSampled)
    {
        assert(imgSize != NULL);
        imageSize[0] = imgSize[0];
        imageSize[1] = imgSize[1];
    }
}

bool ImageTransform::
getPointSampled() const
{
    return pointSampled;
}

void ImageTransform::
setImageTransformation(const Point::Scalar newScale[2],
                       const Point::Scalar newOffset[2])
{
    for(int i=0;i<2;++i)
    {
        scale[i]=newScale[i];
        offset[i]=newOffset[i];
    }
}

void ImageTransform::
flip(int imageHeight)
{
    offset[1]+=Point::Scalar(imageHeight-1)*scale[1];
    scale[1]=-scale[1];
}

const Point::Scalar* ImageTransform::
getScale() const
{
    return scale;
}

const Point::Scalar* ImageTransform::
getOffset(void) const
{
    return offset;
}

Point ImageTransform::
imageToSystem(const Point& imagePoint) const
{
    return Point(imagePoint[0]*scale[0] + offset[0],
                 imagePoint[1]*scale[1] + offset[1]);
}

Point ImageTransform::
imageToSystem(int imageX,int imageY) const
{
    return Point(Point::Scalar(imageX)*scale[0] + offset[0],
                  Point::Scalar(imageY)*scale[1] + offset[1]);
}

Point ImageTransform::
systemToImage(const Point& systemPoint) const
{
    return Point((systemPoint[0]-offset[0]) / scale[0],
                 (systemPoint[1]-offset[1]) / scale[1]);
}

Box ImageTransform::
imageToSystem(const Box& imageBox) const
{
    Point min,max;
    for(int i=0;i<2;++i)
    {
        if(scale[i]>=Point::Scalar(0))
        {
            min[i]=imageBox.min[i]*scale[i] + offset[i];
            max[i]=imageBox.max[i]*scale[i] + offset[i];
        }
        else
        {
            min[i]=imageBox.max[i]*scale[i] + offset[i];
            max[i]=imageBox.min[i]*scale[i] + offset[i];
        }
    }
    return Box(min,max);
}

Box ImageTransform::
systemToImage(const Box& systemBox) const
{
    Point min,max;
    for(int i=0;i<2;++i)
    {
        if(scale[i]>=Point::Scalar(0))
        {
            min[i]=(systemBox.min[i]-offset[i]) / scale[i];
            max[i]=(systemBox.max[i]-offset[i]) / scale[i];
        }
        else
        {
            min[i]=(systemBox.max[i]-offset[i]) / scale[i];
            max[i]=(systemBox.min[i]-offset[i]) / scale[i];
        }
    }
    return Box(min,max);
}

Point ImageTransform::
imageToWorld(const Point& imagePoint) const
{
    return systemToWorld(Point(imagePoint[0]*scale[0] + offset[0],
                               imagePoint[1]*scale[1] + offset[1]));
}

Point ImageTransform::
imageToWorld(int imageX,int imageY) const
{
    return systemToWorld(Point(Point::Scalar(imageX)*scale[0] + offset[0],
                               Point::Scalar(imageY)*scale[1] + offset[1]));
}

Point ImageTransform::
worldToImage(const Point& worldPoint) const
{
    Point systemPoint=worldToSystem(worldPoint);
    return Point((systemPoint[0]-offset[0]) / scale[0],
                 (systemPoint[1]-offset[1]) / scale[1]);
}

Box ImageTransform::
imageToWorld(const Box& imageBox) const
{
    Point min,max;
    for(int i=0;i<2;++i)
    {
        if(scale[i]>=Point::Scalar(0))
        {
            min[i]=imageBox.min[i]*scale[i] + offset[i];
            max[i]=imageBox.max[i]*scale[i] + offset[i];
        }
        else
        {
            min[i]=imageBox.max[i]*scale[i] + offset[i];
            max[i]=imageBox.min[i]*scale[i] + offset[i];
        }
    }
    return systemToWorld(Box(min,max));
}

Box ImageTransform::
worldToImage(const Box& worldBox) const
{
    Box systemBox=worldToSystem(worldBox);
    Point min,max;
    for(int i=0;i<2;++i)
    {
        if(scale[i]>=Point::Scalar(0))
        {
            min[i]=(systemBox.min[i]-offset[i]) / scale[i];
            max[i]=(systemBox.max[i]-offset[i]) / scale[i];
        }
        else
        {
            min[i]=(systemBox.max[i]-offset[i]) / scale[i];
            max[i]=(systemBox.min[i]-offset[i]) / scale[i];
        }
    }
    return Box(min,max);
}

size_t ImageTransform::
getFileSize() const
{
    //writes scale, then offset
    return 4*sizeof(Point::Scalar);
}

bool ImageTransform::
isSystemCompatible(const ImageTransform& other) const
{
    //check if the two transformations have identical scale factors
    if(scale[0]!=other.scale[0] || scale[1]!=other.scale[1])
        return false;

    //check if the offset difference leads to an integer pixel offset
    for(int i=0;i<2;++i)
    {
        Point::Scalar pixelOffset = (other.offset[i]-offset[i]) / scale[i];
        if (Math::abs(pixelOffset-Math::floor(pixelOffset+Point::Scalar(0.5)))>=
            Point::Scalar(0.001))
        {
            return false;
        }
    }

    return true;
}

END_CRUSTA
