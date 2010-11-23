add_executable(construo
    ../include/construo/Builder.h
    ../include/construo/Builder.hpp
    ../include/construo/construoGlobals.h
    ../include/construo/ConstruoSettings.h
    ../include/construo/ConstruoVisualizer.h
    ../include/construo/Converters.h
    ../include/construo/GdalImageFile.h
    ../include/construo/GdalImageFile.hpp
    ../include/construo/GdalTransform.h
    ../include/construo/GeometryTypes.h
    ../include/construo/GeoTransform.h
    ../include/construo/ImageCoverage.h
    ../include/construo/ImageFile.h
    ../include/construo/ImageFile.hpp
    ../include/construo/ImageFileLoader.h
    ../include/construo/ImageFileLoader.hpp
    ../include/construo/ImagePatch.h
    ../include/construo/ImagePatch.hpp
    ../include/construo/ImageTransform.h
    ../include/construo/SphereCoverage.h
    ../include/construo/SubsampleFilter.h
    ../include/construo/SubsampleFilter.hpp
    ../include/construo/TpmFile.h
    ../include/construo/TpmImageFile.h
    ../include/construo/TpmImageFile.hpp
    ../include/construo/Tree.h
    ../include/construo/Tree.hpp

    ../source/construo/Construo.cpp
    ../source/construo/construoGlobals.cpp
    ../source/construo/ConstruoSettings.cpp
    ../source/construo/ConstruoVisualizer.cpp
    ../source/construo/GdalImageFile.cpp
    ../source/construo/GdalTransform.cpp
    ../source/construo/GeoTransform.cpp
    ../source/construo/ImageCoverage.cpp
    ../source/construo/ImageTransform.cpp
    ../source/construo/SphereCoverage.cpp
    ../source/construo/TpmFile.cpp
    ../source/construo/Tree.cpp
)

set_target_properties(construo PROPERTIES
    COMPILE_FLAGS "${VRUI_CFLAGS} -DCONSTRUO_BUILD=1")
target_link_libraries(construo CrustaCore ${GDAL_LIBRARY})
