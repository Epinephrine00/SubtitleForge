cmake_minimum_required(VERSION 3.20)

# vcpkg post-build (applocal.ps1) needs powershell.exe; MSBuild often has no PATH to it.
# Use full path so the next configure+generate gets a working PostBuildEvent.
if(WIN32 AND DEFINED ENV{SystemRoot})
  set(_ps "$ENV{SystemRoot}/System32/WindowsPowerShell/v1.0/powershell.exe")
  if(EXISTS "${_ps}")
    set(Z_VCPKG_POWERSHELL_PATH "${_ps}" CACHE INTERNAL "PowerShell for vcpkg applocal" FORCE)
  endif()
  unset(_ps)
endif()

project(SubtitleForge VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia Concurrent)

# --- FFmpeg via pkg-config (Linux/macOS) or manual paths (Windows) ---
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(AVFORMAT IMPORTED_TARGET libavformat)
    pkg_check_modules(AVCODEC  IMPORTED_TARGET libavcodec)
    pkg_check_modules(AVUTIL   IMPORTED_TARGET libavutil)
    pkg_check_modules(SWRESAMPLE IMPORTED_TARGET libswresample)
    pkg_check_modules(SWSCALE  IMPORTED_TARGET libswscale)
endif()

if(AVFORMAT_FOUND)
    # pkg-config found FFmpeg — use imported targets for correct paths
    set(FFMPEG_IMPORTED_TARGETS
        PkgConfig::AVFORMAT PkgConfig::AVCODEC PkgConfig::AVUTIL
        PkgConfig::SWRESAMPLE PkgConfig::SWSCALE)
    set(FFMPEG_INCLUDE_DIRS "")
    set(FFMPEG_LIBRARIES "")
else()
    # Manual path fallback (Windows without pkg-config)
    find_path(FFMPEG_INCLUDE_DIR libavformat/avformat.h
        HINTS ${FFMPEG_DIR}/include $ENV{FFMPEG_DIR}/include)
    find_library(AVFORMAT_LIB avformat
        HINTS ${FFMPEG_DIR}/lib $ENV{FFMPEG_DIR}/lib)
    find_library(AVCODEC_LIB avcodec
        HINTS ${FFMPEG_DIR}/lib $ENV{FFMPEG_DIR}/lib)
    find_library(AVUTIL_LIB avutil
        HINTS ${FFMPEG_DIR}/lib $ENV{FFMPEG_DIR}/lib)
    find_library(SWRESAMPLE_LIB swresample
        HINTS ${FFMPEG_DIR}/lib $ENV{FFMPEG_DIR}/lib)
    find_library(SWSCALE_LIB swscale
        HINTS ${FFMPEG_DIR}/lib $ENV{FFMPEG_DIR}/lib)

    set(FFMPEG_IMPORTED_TARGETS "")
    set(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})
    set(FFMPEG_LIBRARIES
        ${AVFORMAT_LIB} ${AVCODEC_LIB} ${AVUTIL_LIB}
        ${SWRESAMPLE_LIB} ${SWSCALE_LIB})
endif()

set(SOURCES
    src/main.cpp
    src/app/mainwindow.cpp
    src/model/subtitle.cpp
    src/model/subtitleeffect.cpp
    src/model/project.cpp
    src/model/animationengine.cpp
    src/model/subtitlerenderer.cpp
    src/widgets/timelinewidget.cpp
    src/widgets/previewwidget.cpp
    src/audio/audiodecoder.cpp
    src/audio/audioplayback.cpp
    src/video/videodecoder.cpp
    src/video/previewdecoder.cpp
    src/video/videoexporter.cpp
    src/io/projectio.cpp
)

set(HEADERS
    src/app/mainwindow.h
    src/model/subtitle.h
    src/model/subtitleeffect.h
    src/model/project.h
    src/model/animationengine.h
    src/model/subtitlerenderer.h
    src/widgets/timelinewidget.h
    src/widgets/previewwidget.h
    src/audio/audiodecoder.h
    src/audio/audioplayback.h
    src/video/videodecoder.h
    src/video/previewdecoder.h
    src/video/videoexporter.h
    src/io/projectio.h
)

set(RESOURCES resources/resources.qrc)

add_executable(SubtitleForge ${SOURCES} ${HEADERS} ${RESOURCES})

target_include_directories(SubtitleForge PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(SubtitleForge PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Concurrent
    ${FFMPEG_IMPORTED_TARGETS}
    ${FFMPEG_LIBRARIES}
)

if(APPLE)
    # macOS: app bundle (.app) and frameworks for video/audio
    set_target_properties(SubtitleForge PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.subtitleforge.app")
    find_library(COREMEDIA_FRAMEWORK CoreMedia)
    find_library(VIDEOTOOLBOX_FRAMEWORK VideoToolbox)
    if(COREMEDIA_FRAMEWORK AND VIDEOTOOLBOX_FRAMEWORK)
        target_link_libraries(SubtitleForge PRIVATE
            ${COREMEDIA_FRAMEWORK} ${VIDEOTOOLBOX_FRAMEWORK})
    endif()
endif()

if(WIN32)
    set_target_properties(SubtitleForge PROPERTIES
        WIN32_EXECUTABLE TRUE)
    # Deploy Qt plugins (e.g. platforms/qwindows.dll) and DLLs next to the exe
    set(QT_DEPLOY_HINTS "${CMAKE_PREFIX_PATH}/bin" "$ENV{VCPKG_ROOT}/installed/x64-windows/tools/qt6/bin")
    if(DEFINED Qt6_DIR)
        get_filename_component(_qt_bin "${Qt6_DIR}/../../bin" ABSOLUTE)
        list(PREPEND QT_DEPLOY_HINTS "${_qt_bin}")
    endif()
    find_program(WINDEPLOYQT_EXE windeployqt HINTS ${QT_DEPLOY_HINTS})
    if(WINDEPLOYQT_EXE)
        add_custom_command(TARGET SubtitleForge POST_BUILD
            COMMAND "${WINDEPLOYQT_EXE}" --no-compiler-runtime --no-translations "$<TARGET_FILE_DIR:SubtitleForge>"
            COMMENT "Deploying Qt plugins and DLLs..."
        )
    endif()
endif()
