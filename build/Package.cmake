# Setup package name, version and description
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})

set(CPACK_PACKAGE_VERSION_MAJOR ${${PROJECT_NAME}_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${${PROJECT_NAME}_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${${PROJECT_NAME}_PATCH_VERSION})
set(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})

set(CPACK_PACKAGE_VENDOR "keckcaves.org")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "A geologist's virtual globe to support remote virtual studies.")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/readme.txt")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/license.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/readme.txt")


# Setup the system name
if(APPLE)
   if (CMAKE_SIZEOF_VOID_P EQUAL 4)
       set(CPACK_SYSTEM_NAME macosx32)
   else()
       set(CPACK_SYSTEM_NAME macosx64)
   endif()
else()
   if (CMAKE_SIZEOF_VOID_P EQUAL 4)
      set(CPACK_SYSTEM_NAME Linux-i386)
   else()
      set(CPACK_SYSTEM_NAME Linux-amd64)
   endif()
endif()


# Setup the generators
if (APPLE)
    set(CPACK_GENERATOR Bundle)

    set(CPACK_INSTALL_PREFIX "")
    set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}-${CPACK_PACKAGE_VERSION})
    set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/Crusta.icns")

    set(CPACK_BUNDLE_NAME ${PROJECT_NAME})
    set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/Crusta.icns")
    set(CPACK_BUNDLE_PLIST "${CMAKE_SOURCE_DIR}/Info.plist")
    set(CPACK_BUNDLE_STARTUP_COMMAND "${CMAKE_SOURCE_DIR}/Crusta.scpt")

#what do I actually pass to the <apps> of fixup_bundle
#    install(CODE "include(BundleUtilities)
#                  fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/bin/crustaDesktop\" \"\" \"\")")
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
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgdal1-1.7.0 | libgdal1-1.6.0")

# Include the Cpack module
include(CPack)
