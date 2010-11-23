# Setup package name, version and description
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})

set(CPACK_PACKAGE_VERSION_MAJOR ${${PROJECT_NAME}_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${${PROJECT_NAME}_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${${PROJECT_NAME}_PATCH_VERSION})
set(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})

set(CPACK_PACKAGE_VENDOR "keckcaves.org")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "A geologist's virtual globe to support remote virtual studies")


# Setup the system name
if(APPLE)
    set(CPACK_SYSTEM_NAME macosx)
else()
   if (CMAKE_SIZEOF_VOID_P EQUAL 4)
      set(CPACK_SYSTEM_NAME Linux-i386)
   else()
      set(CPACK_SYSTEM_NAME Linux-amd64)
   endif()
endif()


# Setup the generators
if (APPLE)
   set(CPACK_GENERATOR TGZ;PackageMaker)
else()
   set(CPACK_GENERATOR TGZ;DEB;RPM)

   if (CMAKE_SIZEOF_VOID_P EQUAL 4)
      set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE i386)
      set(CPACK_RPM_PACKAGE_ARCHITECTURE i386)
   else()
      set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
      set(CPACK_RPM_PACKAGE_ARCHITECTURE x86_64)
   endif()

   set(CPACK_DEBIAN_PACKAGE_MAINTAINER tbernardin@ucdavis.edu)
endif()

# Keep the CMake install prefix as is
set(CPACK_SET_DESTDIR ON)


# Setup Apple specific properties
set(CPACK_OSX_PACKAGE_VERSION "10.6")


# Setup debian specific properties
#set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgdal1-1.7.0")

# Include the Cpack module
include(CPack)
