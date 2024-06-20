# schunkdiscover - the network discovery tool for Schunk 2D Grasping Kit
#
# Copyright (c) 2019 Roboception GmbH
# All rights reserved
#
# Author: Heiko Hirschmueller
#
# Copyright (c) 2024 Schunk SE & Co. KG
# All rights reserved
#
# Author: Divya Sasidharan
#
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
# may be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

project(schunkdiscover CXX)

if (UNIX)
  find_package( Threads REQUIRED)
endif ()

add_definitions(-DHAVE_PCAP)

set(schunkdiscover_src
        deviceinfo.cc
        discover.cc
        force_ip.cc
        force_perm_ip.cc
        operation_not_permitted.cc
        wol_exception.cc
        socket_exception.cc
        ping.cc
        wol.cc
        gige_request_counter.cc
        )
set(schunkdiscover_hh
        deviceinfo.h
        discover.h
        force_ip.h
        force_perm_ip.h
        operation_not_permitted.h
        wol_exception.h
        socket_exception.h
        socket.h
        ping.h
        wol.h
        gige_request_counter.h
        utils.h)

if (WIN32)
  set(schunkdiscover_src ${schunkdiscover_src} socket_windows.cc)
  set(schunkdiscover_hh ${schunkdiscover_hh} socket_windows.h)
else (WIN32)
  set(schunkdiscover_src ${schunkdiscover_src} socket_linux.cc)
  set(schunkdiscover_hh ${schunkdiscover_hh} socket_linux.h)
endif (WIN32)

# Static library
add_library(schunkdiscover_static STATIC ${schunkdiscover_src})
add_library(${PROJECT_NAMESPACE}::schunkdiscover_static ALIAS schunkdiscover_static)
if (UNIX)
    target_link_libraries (schunkdiscover_static PUBLIC ${CMAKE_THREAD_LIBS_INIT})
endif(UNIX)
target_include_directories(schunkdiscover_static
        PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>)

target_compile_features(schunkdiscover_static PUBLIC cxx_std_11)

# Install shared library
if (BUILD_SCHUNKDISCOVER_SHARED_LIB)
  message(STATUS "Building shared library: schunkdiscover")
  add_library(schunkdiscover SHARED ${schunkdiscover_src})
  add_library(${PROJECT_NAMESPACE}::schunkdiscover  ALIAS schunkdiscover )
  target_include_directories(schunkdiscover
          PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>)

  target_compile_features(schunkdiscover PUBLIC cxx_std_11)
  set_target_properties(schunkdiscover PROPERTIES SOVERSION ${abiversion})
  target_compile_options(schunkdiscover PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)
  if (UNIX)
    target_link_libraries (schunkdiscover ${CMAKE_THREAD_LIBS_INIT})
  endif(UNIX)

  install(TARGETS schunkdiscover
          EXPORT PROJECTTargets
          COMPONENT bin
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(FILES ${schunkdiscover_hh} COMPONENT dev DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/schunkdiscover)
endif()
