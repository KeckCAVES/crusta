#ifndef _GlobeFile_HPP_
#define _GlobeFile_HPP_


BEGIN_CRUSTA


template <typename PixelParam>
GlobeFile<PixelParam>::
~GlobeFile()
{
    close();
}

template <typename PixelParam>
const PixelParam* GlobeFile<PixelParam>::
getBlank() const
{
    return &blank.front();
}

template <typename PixelParam>
const std::string& GlobeFile<PixelParam>::
getDataType()
{
    return dataType;
}

template <typename PixelParam>
const int& GlobeFile<PixelParam>::
getNumChannels()
{
    return numChannels;
}

template <typename PixelParam>
const PixelParam& GlobeFile<PixelParam>::
getDefaultPixelValue() const
{
    return defaultPixelValue;
}

template <typename PixelParam>
const int* GlobeFile<PixelParam>::
getTileSize() const
{
    return &tileSize[0];
}

template <typename PixelParam>
const TileIndex& GlobeFile<PixelParam>::
getNumTiles() const
{
    return numTiles;
}


END_CRUSTA


#endif //_GlobeFile_HPP_
