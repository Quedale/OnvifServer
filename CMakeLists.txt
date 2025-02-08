cmake_minimum_required(VERSION 3.5.0)
project(onvif-device VERSION 0.1.0)

set(GSOAP_SRC_DIR ${CMAKE_SOURCE_DIR}/subprojects/gsoap-2.8 CACHE STRING "Location of the gsoap source directory")
set(ONVIF_MAX_USERS CACHE STRING "Maximum number of users allowed")
set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/dist)
set(LIBRARY_OUTPUT_PATH    ${OUTPUT_DIR})
set(EXECUTABLE_OUTPUT_PATH ${OUTPUT_DIR})

execute_process (
    COMMAND bash -c "${CMAKE_CURRENT_SOURCE_DIR}/autogen.sh"
    RESULT_VARIABLE ret)

if(ret EQUAL "1")
    message( FATAL_ERROR "autogen.sh script failed.")
endif()

aux_source_directory(src src_dir)
aux_source_directory(src/common common_dir)
aux_source_directory(src/devicemgmt devicemgmt_dir)
aux_source_directory(src/discovery discovery_dir)
aux_source_directory(src/generated generated_dir)
aux_source_directory(src/media media_dir)
aux_source_directory(src/rtsp rtsp_dir)

add_executable(onvifserver 
                    ${src_dir}
                    ${common_dir}
                    ${devicemgmt_dir}
                    ${discovery_dir}
                    ${generated_dir}
                    ${media_dir}
                    ${rtsp_dir}
                    ${GSOAP_SRC_DIR}/gsoap/dom.c 
                    ${GSOAP_SRC_DIR}/gsoap/stdsoap2.c
                    ${GSOAP_SRC_DIR}/gsoap/plugin/logging.c
                    ${GSOAP_SRC_DIR}/gsoap/plugin/smdevp.c 
                    ${GSOAP_SRC_DIR}/gsoap/plugin/mecevp.c 
                    ${GSOAP_SRC_DIR}/gsoap/plugin/wsaapi.c 
                    ${GSOAP_SRC_DIR}/gsoap/plugin/wsseapi.c 
                    ${GSOAP_SRC_DIR}/gsoap/plugin/threads.c
                    ${GSOAP_SRC_DIR}/gsoap/plugin/httpda.c
                    ${GSOAP_SRC_DIR}/gsoap/plugin/wsddapi.c
                    ${GSOAP_SRC_DIR}/gsoap/custom/struct_timeval.c)

if (ONVIF_MAX_USERS)
  target_compile_options(onvifserver PRIVATE "SHELL: -DONVIF_MAX_USERS=${ONVIF_MAX_USERS}")
endif()

set(GSTREAMER_DEP_LIST               
        gstreamer-1.0>=1.18                 #core
        gstreamer-rtsp-server-1.0>=1.18
        gstcoreelements>=1.18
        gstapp>=1.18                              #base
        gstaudioconvert>=1.18
        gstaudioresample>=1.18
        gstaudiotestsrc>=1.18
        gstpbtypes>=1.18
        gstrawparse>=1.18
        gsttcp>=1.18
        gsttypefindfunctions>=1.18
        gstvideoconvertscale>=1.18
        gstvideotestsrc>=1.18
        gstvideorate>=1.18
        gstvolume>=1.18
        gstalaw>=1.18                            #good
        gstaudioparsers>=1.18
        gstautodetect>=1.18
        gstmulaw>=1.18
        gstalsa>=1.18     #gstpulseaudio
        gstrtp>=1.18
        gstrtpmanager>=1.18
        gstrtsp>=1.18
        gstinterleave>=1.18
        gstjpeg>=1.18
        gstudp>=1.18
        gstvideo4linux2>=1.18
        gstfdkaac>=1.18                           #bad
        gstinterlace>=1.18                        #bad
        gstjpegformat>=1.18
        gstopenh264>=1.18
        gstrtponvif>=1.18
        gstv4l2codecs>=1.18
        gstvideoparsersbad>=1.18
        gstde265>=1.18)

find_package(PkgConfig)
message(STATUS "Checking system libraries for Gstreamer.")
pkg_check_modules(GSTDEPS IMPORTED_TARGET NO_CMAKE_PATH ${GSTREAMER_DEP_LIST})

set(ENV{PKG_CONFIG_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/subprojects/CUtils/build/dist/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
set(ENV{PKG_CONFIG_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libntlm-1.8/build/dist/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
set(ENV{PKG_CONFIG_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/subprojects/pulseaudio/build/dist/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
set(ENV{PKG_CONFIG_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/subprojects/gstreamer/build/dist/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
set(ENV{PKG_CONFIG_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/subprojects/gstreamer/build/dist/lib/gstreamer-1.0/pkgconfig:$ENV{PKG_CONFIG_PATH}")
set(ENV{PKG_CONFIG_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libde265/dist/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
set(ENV{PKG_CONFIG_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/subprojects/alsa-lib/dist/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")

if(NOT GSTDSPS_FOUND)
  message(STATUS "System doesn't have the appropriate Gstreamer installed")
  message(STATUS "\tUsing static libraries instead.")
  message(STATUS "\tChecking under ${CMAKE_CURRENT_SOURCE_DIR}/subprojects")
  pkg_check_modules(GSTDEPS REQUIRED QUIET IMPORTED_TARGET NO_CMAKE_PATH ${GSTREAMER_DEP_LIST})
  target_compile_options(onvifserver PRIVATE "SHELL: -DSTATIC_BUILD")
  message(STATUS "Static gstreamer found")
endif()

pkg_check_modules(DEPS REQUIRED IMPORTED_TARGET 
              cutils libntlm glib-2.0 gobject-2.0)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

target_include_directories(onvifserver PRIVATE
                    src
                    src/generated
                    ${GSOAP_SRC_DIR}/gsoap/ 
                    ${GSOAP_SRC_DIR}/gsoap/import 
                    ${GSOAP_SRC_DIR}/gsoap/custom 
                    ${GSOAP_SRC_DIR}/gsoap/plugin)
target_include_directories(onvifserver PRIVATE ${DEPS_INCLUDE_DIRS} ${GSTDEPS_INCLUDE_DIRS})
target_compile_options(onvifserver PRIVATE "SHELL: -c -Wall -Wextra -Wpedantic -Wno-unused-parameter -DWITH_NONAMESPACES -DWITH_NOEMPTYSTRUCT -DWITH_NOEMPTYNAMESPACES -DWITH_DOM -DBUILD_SHARD -DSOAP_H_FILE=onvifsoapH.h")
target_compile_options(onvifserver PRIVATE "SHELL: -DWITH_OPENSSL -DWITH_ZLIB -DWITH_GZIP -DWITH_NTLM")
target_link_libraries(onvifserver PRIVATE m OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB Threads::Threads)
# set_target_properties(onvifserver PROPERTIES LINKER_LANGUAGE C)
# set_target_properties(onvifserver PROPERTIES LINK_SEARCH_START_STATIC 1)
# set_target_properties(onvifserver PROPERTIES LINK_SEARCH_END_STATIC 1)
# set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
#We have to link using LDFLAGS in order to keep the order or -lstdc++
target_link_libraries(onvifserver PRIVATE ${GSTDEPS_LDFLAGS} ${DEPS_LDFLAGS} c stdc++)

#Override SOAP_WSDD_2005 so that the wsdd plugins behaves appropriately for ONVIF remotediscovery.wsdl
#This is to overcome a (in my opinion) bug where remotediscovery.wsdl uses an older version of ws-address compared to the reste of the wsdls.
set_source_files_properties(${GSOAP_SRC_DIR}/gsoap/plugin/wsddapi.c PROPERTIES COMPILE_FLAGS -USOAP_WSDD_2005)

#Ingore gsoap warnings
if (MSVC)
  set_source_files_properties(${GSOAP_SRC_DIR}/gsoap/dom.c 
                              ${GSOAP_SRC_DIR}/gsoap/stdsoap2.c 
                              ${GSOAP_SRC_DIR}/gsoap/plugin/logging.c
                              ${GSOAP_SRC_DIR}/gsoap/plugin/smdevp.c 
                              ${GSOAP_SRC_DIR}/gsoap/plugin/mecevp.c 
                              ${GSOAP_SRC_DIR}/gsoap/plugin/wsaapi.c 
                              ${GSOAP_SRC_DIR}/gsoap/plugin/wsseapi.c 
                              ${GSOAP_SRC_DIR}/gsoap/plugin/threads.c
                              ${GSOAP_SRC_DIR}/gsoap/plugin/httpda.c
                              ${GSOAP_SRC_DIR}/gsoap/plugin/wsddapi.c
                              ${GSOAP_SRC_DIR}/gsoap/custom/struct_timeval.c PROPERTIES COMPILE_FLAGS -wd4244 -wd4267)
else()
  set_source_files_properties(${GSOAP_SRC_DIR}/gsoap/dom.c 
                              ${GSOAP_SRC_DIR}/gsoap/stdsoap2.c 
                              ${GSOAP_SRC_DIR}/gsoap/plugin/logging.c
                              ${GSOAP_SRC_DIR}/gsoap/plugin/smdevp.c 
                              ${GSOAP_SRC_DIR}/gsoap/plugin/mecevp.c 
                              ${GSOAP_SRC_DIR}/gsoap/plugin/wsaapi.c 
                              ${GSOAP_SRC_DIR}/gsoap/plugin/wsseapi.c 
                              ${GSOAP_SRC_DIR}/gsoap/plugin/threads.c
                              ${GSOAP_SRC_DIR}/gsoap/plugin/httpda.c
                              ${GSOAP_SRC_DIR}/gsoap/plugin/wsddapi.c
                              ${GSOAP_SRC_DIR}/gsoap/custom/struct_timeval.c PROPERTIES COMPILE_FLAGS -w)
endif()


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
