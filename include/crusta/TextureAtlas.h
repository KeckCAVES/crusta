#ifndef TEXTUREATLAS_H
#define TEXTUREATLAS_H

#include <crusta/basics.h>

template <size_t TileWidth, GLint GLFormat>
        class TextureAtlas {

    public:
    TextureAtlas(size_t tileCount) : _tileCount(tileCount), _freeBitmap(TileCount, true), Bitmap(TileCount) {
        glGenTextures(1, &_tex);

        // find the appropriate atlas width such that the atlas can hold TileCount tiles
        _texWidth = 0;

        while ((_texWidth / TileWidth) * (_texWidth / TileWidth) < _tileCount)
            _texWidth += 8;

        _atlasWidth = _texWidth / TileWidth;

        std::cerr << "TextureAtlas::TextureAtlas(): texWidth = " << _texWidth << std::endl;

        glBindTexture(GL_TEXTURE_2D, _tex);
        glTexture2D(GL_TEXTURE_2D, 0, GLFormat, _texWidth, _texWidth, 0, foo, foo, (const GLvoid*)0);
    }

    void getTileUV(size_t tileIdx, float &u, float &v) {
        size_t uInt, vInt;
        tileIdxToUV(tileIdx, uInt, vInt);

        // FIXME: check if texel coords are centered on texels
        u = (float)uInt / _texWidth;
        v = (float)vInt / _texWidth;
    }

    bool findFreeTile(size_t &tileIdx) {
        for (size_t i=0; i < _freeBitmap.size(); ++i) {
            if (_freeBitmap[i]) {
                tileIdx = i;
                return true;
            }
        }

        return false;
    }

    bool freeTile(size_t tileIdx) {
        assert(tileIdx < _tileCount);
        assert(!_freeBitmap[tileIdx]);

        _freeBitmap[tileIdx] = true;
    }

    void setTile(size_t tileIdx, GLenum glFormat, GLenum glType, )
    private:
       GLuint _tex;
       size_t _tileCount;
       size_t _texWidth; // in texels
       size_t _width; // in number of tiles

       std::vector<bool> _freeBitmap;

       void tileIdxToUV(size_t tileIdx, size_t &u, size_t &v) {
            assert(tileIdx < _tileCount);

            v = TileWidth * (tileIdx / _atlasWidth);
            u = TileWidth * (tileIdx % _atlasWidth);
       }
};

#endif // TEXTUREATLAS_H
