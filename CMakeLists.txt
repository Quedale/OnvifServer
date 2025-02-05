cmake_minimum_required(VERSION 3.5.0)
project(onvif-device VERSION 0.1.0)

execute_process (
    COMMAND bash -c "${CMAKE_CURRENT_SOURCE_DIR}/autogen.sh"
    RESULT_VARIABLE ret
)
if(ret EQUAL "1")
    message( FATAL_ERROR "autogen.sh script failed.")
endif()

set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/dist)
set(LIBRARY_OUTPUT_PATH    ${OUTPUT_DIR})
set(EXECUTABLE_OUTPUT_PATH ${OUTPUT_DIR})

if (NOT DEFINED GSOAP_SRC_DIR)
  set(GSOAP_SRC_DIR  ${CMAKE_SOURCE_DIR}/subprojects/gsoap-2.8)
endif()

set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${CMAKE_CURRENT_SOURCE_DIR}/subprojects/CUtils/build/dist/lib/pkgconfig")
set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libntlm-1.8/build/dist/lib/pkgconfig")

find_package(PkgConfig)
pkg_check_modules(CUTILS REQUIRED cutils)
pkg_check_modules(NTLM REQUIRED libntlm)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GOBJECT REQUIRED gobject-2.0)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

include_directories(src
                    src/generated
                    ${GSOAP_SRC_DIR}/gsoap/ 
                    ${GSOAP_SRC_DIR}/gsoap/import 
                    ${GSOAP_SRC_DIR}/gsoap/custom 
                    ${GSOAP_SRC_DIR}/gsoap/plugin
                    ${CUTILS_INCLUDE_DIRS}
                    ${NTLM_INCLUDE_DIRS}
                    ${GLIB_INCLUDE_DIRS}
                    ${GOBJECT_INCLUDE_DIRS})

aux_source_directory(src src_dir)
aux_source_directory(src/common common_dir)
aux_source_directory(src/devicemgmt devicemgmt_dir)
aux_source_directory(src/discovery discovery_dir)
aux_source_directory(src/generated generated_dir)
aux_source_directory(src/media media_dir)

add_executable(onvifserver 
                    ${src_dir}
                    ${common_dir}
                    ${devicemgmt_dir}
                    ${discovery_dir}
                    ${generated_dir}
                    ${media_dir}
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

target_compile_options(onvifserver PRIVATE "SHELL: -c -Wall -Wextra -Wpedantic -Wno-unused-parameter -DWITH_NONAMESPACES -DWITH_NOEMPTYSTRUCT -DWITH_NOEMPTYNAMESPACES -DWITH_DOM -DBUILD_SHARD -DSOAP_H_FILE=onvifsoapH.h")
target_compile_options(onvifserver PRIVATE "SHELL: -DWITH_OPENSSL -DWITH_ZLIB -DWITH_GZIP -DWITH_NTLM")
target_link_libraries(onvifserver PRIVATE m ${CUTILS_LDFLAGS} ${NTLM_LDFLAGS} ${GLIB_LDFLAGS} ${GOBJECT_LDFLAGS})
target_link_libraries(onvifserver PRIVATE OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB Threads::Threads)

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
