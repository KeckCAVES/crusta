#include <construo/GdalTransform.h>

#include <iostream>
#include <sstream>

#include <Misc/File.h>

#include <construo/Converters.h>

BEGIN_CRUSTA

GdalTransform::
GdalTransform() :
    geoToWorld(NULL), worldToGeo(NULL)
{
}

GdalTransform::
GdalTransform(const char* projectionFileName)
{
///\todo remove: this is my WKT generator
#if 0
    OGRSpatialReference outSrs;
    outSrs.SetProjCS("Blue Marble Topograpy");
    outSrs.SetWellKnownGeogCS("WGS84");
//    outSrs.SetUTM( 17, TRUE );
    outSrs.SetEquirectangular();
    exit(5);
#endif

    Misc::File file(projectionFileName, "r");

    OGRSpatialReference geoSys;

    //read in the transformations from the file
    char line[1024];
	while (file.gets(line, sizeof(line)))
    {
        if (strcasecmp(line, "Geotransform:\n")==0)
        {
            //read in the geotransform matrix components
            Transform::Matrix& m = pixelToGeo.getMatrix();

            /* Xgeo = GT(0) + Xpixel*GT(1) + Yline*GT(2)
               Ygeo = GT(3) + Xpixel*GT(4) + Yline*GT(5)
               (see GDAL documentation) */
            file.gets(line, sizeof(line));
            std::istringstream iss(line);
            iss >> m(0,2) >> m(0,0) >> m(0,1) >> m(1,2) >> m(1,0) >> m(1,1);

            geoToPixel = pixelToGeo;
            geoToPixel.doInvert();
        }
        else if (strcasecmp(line, "Projection:\n")==0)
        {
            //read in the projection
            file.gets(line, sizeof(line));
            char* in = line;
            OGRErr err = geoSys.importFromWkt(&in);
            if (err != OGRERR_NONE)
            {
                Misc::throwStdErr("GdalTransform: failed to read the "
                                  "projection");
            }
        }
    }

    OGRSpatialReference worldSys;
    worldSys.SetGeogCS("Crusta World Coordinate System", "WGS_1984",
                       "Crusta WGS84 Spheroid",
                       SRS_WGS84_SEMIMAJOR, SRS_WGS84_INVFLATTENING,
                       "Greenwich", 0.0,
                       SRS_UA_RADIAN, 1.0);

    geoToWorld = OGRCreateCoordinateTransformation(&geoSys, &worldSys);
    if (geoToWorld == NULL)
    {
        Misc::throwStdErr("GdalTransform: unable to generate a transformation "
                          "from the file's coordinate system to crusta's world "
                          "coordinate system (WGS84, radians)");
    }
    worldToGeo = OGRCreateCoordinateTransformation(&worldSys, &geoSys);
    if (worldToGeo == NULL)
    {
        Misc::throwStdErr("GdalTransform: unable to generate a transformation "
                          "from crusta's world coordinate system (WGS84, "
                          "radians) to the file's coordinate system");
    }
}


void GdalTransform::
flip(int imageHeight)
{
///\todo support rotated images
    Transform::Matrix& m = pixelToGeo.getMatrix();
    m(1,2) += Point::Scalar(imageHeight-1)*m(1,1);
    m(1,1)  = -m(1,1);

    geoToPixel = pixelToGeo;
    geoToPixel.doInvert();
}


Point::Scalar GdalTransform::
getFinestResolution(const int size[2]) const
{
///\todo consider distortion in projection?
    int half[2] = { size[0]>>1, size[1]>>1 };
    Point origin = imageToWorld(half[0],   half[1]);
    Point left   = imageToWorld(half[0]+1, half[1]);
    Point up     = imageToWorld(half[0],   half[1]+1);

    Point::Scalar lx = Converter::haversineDist(origin, left, SPHEROID_RADIUS);
    Point::Scalar ly = Converter::haversineDist(origin,   up, SPHEROID_RADIUS);

    return std::min(lx, ly);
}




Point GdalTransform::
imageToSystem(const Point& imagePoint) const
{
    if (!pointSampled)
    {
        Point mapped = imagePoint;
        mapped[0] += Point::Scalar(0.5);
        mapped[1] += Point::Scalar(0.5);
        return pixelToGeo.transform(mapped);
    }
    else
        return pixelToGeo.transform(imagePoint);
}

Point GdalTransform::
imageToSystem(int imageX, int imageY) const
{
    return imageToSystem(Point(imageX, imageY));
}

Point GdalTransform::
systemToImage(const Point& systemPoint) const
{
    Point res = geoToPixel.transform(systemPoint);
    if (!pointSampled)
    {
        for (int i=0; i<2; ++i)
        {
            //map it to cell centered pixel look-up
            res[i] -= Point::Scalar(0.5);

            /* now we can have valid values half a pixel beyong the boundaries.
               Snap these to the boundary pixel */
            if (res[i]<Point::Scalar(0) && res[i]>=Point::Scalar(-0.5))
            {
                res[i] = Point::Scalar(0);
            }
            else if (res[i] >  Point::Scalar(imageSize[i]-1) &&
                     res[i] <= Point::Scalar(imageSize[i]-1)+Point::Scalar(0.5))
            {
                res[i] = Point::Scalar(imageSize[i]-1);
            }
        }
    }
    return res;
}

Box GdalTransform::
imageToSystem(const Box& imageBox) const
{
    Box res(HUGE_VAL, -HUGE_VAL);
    res.addPoint(imageToSystem(imageBox.min));
    res.addPoint(imageToSystem(imageBox.max));
    return res;
}

Box GdalTransform::
systemToImage(const Box& systemBox) const
{
    Box res(HUGE_VAL, -HUGE_VAL);
    res.addPoint(systemToImage(systemBox.min));
    res.addPoint(systemToImage(systemBox.max));
    return res;
}




Point GdalTransform::
systemToWorld(const Point& systemPoint) const
{
    Point res = systemPoint;
    if (!geoToWorld->Transform(1, &res[0], &res[1]))
    {
        std::cout << "GdalTransform::systemToWorld: failed to transform ("
                  << systemPoint[0] << ", " << systemPoint[1] << ")"
                  << std::endl;
    }
    return res;
}

Point GdalTransform::
worldToSystem(const Point& worldPoint) const
{
    Point res = worldPoint;
    if (!worldToGeo->Transform(1, &res[0], &res[1]))
    {
        std::cout << "GdalTransform::systemToWorld: failed to transform ("
                  << worldPoint[0] << ", " << worldPoint[1] << ")" << std::endl;
    }
    return res;
}

Box GdalTransform::
systemToWorld(const Box& systemBox) const
{
///\todo this is poorly implemented neglecting intricacies of projections
    Box::Scalar xs[2] = { systemBox.min[0], systemBox.max[0] };
    Box::Scalar ys[2] = { systemBox.min[1], systemBox.max[1] };

    if (!geoToWorld->Transform(2, xs, ys))
    {
        std::cout << "GdalTransform::systemToWorld(Box): failed to transform {("
                  << systemBox.min[0] << ", " << systemBox.min[1] << "); ("
                  << systemBox.max[0] << ", " << systemBox.max[1] << ")}"
                  << std::endl;
    }
    Box res(HUGE_VAL, -HUGE_VAL);
    res.addPoint(Point(xs[0], ys[0]));
    res.addPoint(Point(xs[1], ys[1]));
    return res;
}

Box GdalTransform::
worldToSystem(const Box& worldBox) const
{
///\todo this is poorly implemented neglecting intricacies of projections
    Box::Scalar xs[2] = { worldBox.min[0], worldBox.max[0] };
    Box::Scalar ys[2] = { worldBox.min[1], worldBox.max[1] };

    if (!worldToGeo->Transform(2, xs, ys))
    {
        std::cout << "GdalTransform::worldToSystem(Box): failed to transform {("
                  << worldBox.min[0] << ", " << worldBox.min[1] << "); ("
                  << worldBox.max[0] << ", " << worldBox.max[1] << ")}"
                  << std::endl;
    }
    Box res(HUGE_VAL, -HUGE_VAL);
    res.addPoint(Point(xs[0], ys[0]));
    res.addPoint(Point(xs[1], ys[1]));
    return res;
}




Point GdalTransform::
imageToWorld(const Point& imagePoint) const
{
    Point p = imageToSystem(imagePoint);
    return systemToWorld(p);
}

Point GdalTransform::
imageToWorld(int imageX,int imageY) const
{
    Point p = imageToSystem(Point(imageX, imageY));
    return systemToWorld(p);
}

Point GdalTransform::
worldToImage(const Point& worldPoint) const
{
    Point p = worldToSystem(worldPoint);
    return systemToImage(p);
}

Box GdalTransform::
imageToWorld(const Box& imageBox) const
{
    Box b = imageToSystem(imageBox);
    return systemToWorld(b);
}

Box GdalTransform::
worldToImage(const Box& worldBox) const
{
    Box b = worldToSystem(worldBox);
    return systemToImage(b);
}




bool GdalTransform::
isCompatible(const ImageTransform& other) const
{
///\todo implement
    Misc::throwStdErr("GdalTransform::isCompatible: not implemented!");
    return false;
}

size_t GdalTransform::
getFileSize() const
{
///\todo implement
    Misc::throwStdErr("GdalTransform::isCompatible: not implemented!");
    return 0;
}

END_CRUSTA
