# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)
include(ndk-stl-config.cmake)
#include_directories(native/jni/include)



set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Ofast")
# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        native-lib
        SHARED
        native-lib.cpp
        AudioListener.cpp
        AudioGenerator.cpp
        BPSKSignalGenerator.cpp
        MatchedFilterDetector.cpp
        AmpDetector.cpp
        kissfft/kiss_fft.c
        kissfft/tools/kiss_fftr.c
        )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.
set (OBOE_DIR ../../../../../oboe)
add_subdirectory (${OBOE_DIR} ./oboe)

set(OpenCV_DIR "../../../../../OpenCV-android-sdk-4.3.0/sdk/native/jni" )
set(OpenCV_STATIC ON)
find_package( OpenCV REQUIRED )

message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS ${OpenCV_VERSION})
include_directories (${OBOE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
        kissfft
        kissfft/tools
        )

target_link_libraries( # Specifies the target library.
        native-lib
        oboe
        ${OpenCV_LIBS}
        ${log-lib} )

get_target_property(__src opencv_java IMPORTED_LOCATION_RELEASE )
add_custom_command(TARGET native-lib POST_BUILD COMMAND
        ${CMAKE_COMMAND} -E copy ${__src} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        )

