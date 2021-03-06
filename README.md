# Crusta Virtual Globe

## Introduction

Crusta is an immersive virtual globe application.

Crusta enables:

* representation of global, high-resolution surface data,
* visualization of these data on a real-time virtual globe, and
* efficient exploration and annotation using real-time interactive software tools.

Using Crusta one can easily import sub-meter resolution DEM or aerial imagery for any location on the globe.  Dynamic manipulation of the visualization using local illumination, real-time scaling of vertical exaggeration, and hardware accelerated textured iso-lines support explorative discovery of key surface features that results in a clear understanding of their three-dimensional embedding.  Key features such as fault scarps, landslides, and other historically preserved geologic markers can be directly mapped on the virtual landscape.  Real-time mapping and the identification of features using a three-dimensional immersive application greatly improves the confidence and localization of mapped features.

## Install

To install Crusta from source, you'll need the following:

* git
* cmake 2.8.0+
* Vrui 2.7 w/ development files
* GDAL 1.8+ w/ development files
* GLU w/ development files
* GLEW

If you want the pretty scenegraph font additions, you'll also need:

* FTGL
* Fontconfig

To get going quickly, from the same directory as this file, just do:

    cmake . -DCMAKE_INSTALL_PREFIX=/path/for/crusta && make install

If you have any software in a non-standard place, you may need to set the environment variable `PKG_CONFIG_PATH`.  E.g.

    PKG_CONFIG_PATH=/path/to/Vrui/pkgconfig cmake . \
      -DCMAKE_INSTALL_PREFIX=/path/for/crusta && make install
