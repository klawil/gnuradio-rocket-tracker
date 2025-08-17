########################################################################
# Project setup
########################################################################

cmake_minimum_required(VERSION 3.25)
SET(CMAKE_CXX_STANDARD 20)

project(AltusTracker LANGUAGES CXX C VERSION "0.0.1")

# Make sure our local CMake Modules path comes first
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)

########################################################################
# Compiler specific setup
########################################################################
if(CMAKE_COMPILER_IS_GNUCXX AND NOT WIN32)
    #http://gcc.gnu.org/wiki/Visibility
    add_definitions(-fvisibility=hidden)
endif()

if(APPLE)
   set(CMAKE_MACOSX_RPATH ON)
endif()
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/altus-tracker")

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

########################################################################
# Install directories
########################################################################
include(FindPkgConfig)
find_package(Gnuradio REQUIRED)
include(GrVersion)

set(GR_REQUIRED_COMPONENTS RUNTIME ANALOG BLOCKS DIGITAL FILTER FFT PMT)
find_package(Gnuradio REQUIRED COMPONENTS runtime analog blocks digital filter fft)
if(Gnuradio_VERSION VERSION_LESS "3.8")
    find_package(Volk)
endif()

if(NOT CMAKE_MODULES_DIR)
  set(CMAKE_MODULES_DIR lib${LIB_SUFFIX}/cmake)
endif(NOT CMAKE_MODULES_DIR)

if(Gnuradio_VERSION VERSION_LESS "3.8")
    find_package(Volk)
endif()

if(NOT Gnuradio_FOUND)
    message(FATAL_ERROR "GnuRadio Runtime required to compile gqrx")
endif()

set(GR_RUNTIME_DIR      bin)
set(GR_LIBRARY_DIR      lib${LIB_SUFFIX})
set(GR_INCLUDE_DIR      include/howto)
set(GR_DATA_DIR         share)
set(GR_PKG_DATA_DIR     ${GR_DATA_DIR}/${CMAKE_PROJECT_NAME})
set(GR_DOC_DIR          ${GR_DATA_DIR}/doc)
set(GR_PKG_DOC_DIR      ${GR_DOC_DIR}/${CMAKE_PROJECT_NAME})
set(GR_CONF_DIR         etc)
set(GR_PKG_CONF_DIR     ${GR_CONF_DIR}/${CMAKE_PROJECT_NAME}/conf.d)
set(GR_LIBEXEC_DIR      libexec)
set(GR_PKG_LIBEXEC_DIR  ${GR_LIBEXEC_DIR}/${CMAKE_PROJECT_NAME})
set(GRC_BLOCKS_DIR      ${GR_PKG_DATA_DIR}/grc/blocks)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

########################################################################
# Find gnuradio build dependencies
########################################################################
# To run a more advanced search for GNU Radio and it's components and
# versions, use the following. Add any components required to the list
# of GR_REQUIRED_COMPONENTS (in all caps) and change "version" to the
# minimum API compatible version required.
#
# set(GR_REQUIRED_COMPONENTS RUNTIME BLOCKS FILTER ...)
# find_package(Gnuradio "version")
#if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
#    set(GR_REQUIRED_COMPONENTS ANALOG DIGITAL RUNTIME BLOCKS FILTER PMT)
#    find_package(Gnuradio)
#endif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")

math(EXPR GNURADIO_VERSION "(${Gnuradio_VERSION_MAJOR} / 10) << 20 | (${Gnuradio_VERSION_MAJOR} % 10) << 16 | (${Gnuradio_VERSION_MINOR} / 10) << 12 | (${Gnuradio_VERSION_MINOR} % 10) <<  8 | (${Gnuradio_VERSION_PATCH} / 10) <<  4 | (${Gnuradio_VERSION_PATCH} % 10) <<  0   ")
message(STATUS "GnuRadio Version: " ${GNURADIO_VERSION})

########################################################################
# Find build dependencies
########################################################################
# find_package(GnuradioUHD)
find_package(GnuradioOsmosdr)
# find_package(LibHackRF)
# find_package(LibUHD)
# find_package(OpenSSL REQUIRED)
# find_package(CURL REQUIRED)
# if (STREAMER)
#     find_package(Protobuf REQUIRED)
#     find_package(GRPC REQUIRED)
# endif()

########################################################################
# Setup boost
########################################################################
# MESSAGE(STATUS "Configuring Boost C++ Libraries...")

# Although not required on my system, some users have linking issues without
SET(BOOST_REQUIRED_COMPONENTS
    thread
    system
    program_options
    filesystem
    log_setup
    log
    regex
    random
)

if(UNIX AND NOT BOOST_ROOT AND EXISTS "/usr/lib64")
    list(APPEND BOOST_LIBRARYDIR "/usr/lib64") #fedora 64-bit fix
endif(UNIX AND NOT BOOST_ROOT AND EXISTS "/usr/lib64")

set(Boost_ADDITIONAL_VERSIONS
    "1.35.0" "1.35" "1.36.0" "1.36" "1.37.0" "1.37" "1.38.0" "1.38" "1.39.0" "1.39"
    "1.40.0" "1.40" "1.41.0" "1.41" "1.42.0" "1.42" "1.43.0" "1.43" "1.44.0" "1.44"
    "1.45.0" "1.45" "1.46.0" "1.46" "1.47.0" "1.47" "1.48.0" "1.48" "1.49.0" "1.49"
    "1.50.0" "1.50" "1.51.0" "1.51" "1.52.0" "1.52" "1.53.0" "1.53" "1.54.0" "1.54"
    "1.55.0" "1.55" "1.56.0" "1.56" "1.57.0" "1.57" "1.58.0" "1.58" "1.59.0" "1.59"
    "1.60.0" "1.60" "1.61.0" "1.61" "1.62.0" "1.62" "1.63.0" "1.63" "1.64.0" "1.64"
    "1.65.0" "1.65" "1.66.0" "1.66" "1.67.0" "1.67" "1.68.0" "1.68" "1.69.0" "1.69"
    "1.70.0" "1.70" "1.71.0" "1.71" "1.72.0" "1.72" "1.73.0" "1.73" "1.74.0" "1.74"
)

find_package(Boost COMPONENTS ${BOOST_REQUIRED_COMPONENTS} REQUIRED)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost required to build " ${CMAKE_PROJECT_NAME})
endif()
message(STATUS "Boost Found At " ${Boost_INCLUDE_DIRS})

ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK)
ADD_DEFINITIONS(-DBOOST_BIND_GLOBAL_PLACEHOLDERS)
if(NOT EXISTS /usr/include/spdlog/fmt/bundled)
  ADD_DEFINITIONS(-DSPDLOG_FMT_EXTERNAL)
endif()
add_definitions(-DGNURADIO_VERSION=${GNURADIO_VERSION})
message(STATUS "Gnuradio Version is: " ${Gnuradio_VERSION})

# add_definitions(-DENABLE_PERFORMANCE_COUNTERS=True)

# find_package(SoapySDR "0.7.2" NO_MODULE)
# if(SoapySDR_FOUND)
# 	message(STATUS "SoapySDR found, version: ${SoapySDR_VERSION}")
# 	# include_directories(${SoapySDR_INCLUDE_DIRS})
# 	# list(APPEND SDR_LIBRARIES ${SoapySDR_LIBRARIES})
# 	# ADD_DEFINITIONS(-DSOAPYSDR)
# endif()

########################################################################
# Setup the include and linker paths
########################################################################
include_directories(
    ${Boost_INCLUDE_DIRS}
    ${GNURADIO_RUNTIME_INCLUDE_DIRS}
    # ${GNURADIO_ALL_LIBRARIES}
    ${GNURADIO_OSMOSDR_INCLUDE_DIRS}
    # ${SoapySDR_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/lib
    ${CMAKE_BINARY_DIR}/include
    "lib"
)

link_directories(
    ${Boost_LIBRARY_DIRS}
    # ${GNURADIO_RUNTIME_INCLUDE_DIRS}
    ${GNURADIO_ALL_LIBRARY_DIRS}
    # ${SoapySDR_LIBRARY_DIRS}
    ${GROSMOSDR_LIBRARIES_DIRS}
)

set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wno-unused-local-typedef -Wno-deprecated-declarations -Wno-error=deprecated-declarations -g3")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wno-narrowing -fvisibility=hidden -fPIC")

list(APPEND altus_tracker_sources
  source/blocks/altus_decoder.cc
  source/blocks/altus_channel.cc
  source/blocks/altus_power_level.cc
  source/blocks/altus_detector.cc
  source/altus_packet.cc
)

# list(APPEND altus_tracker_headers)

add_library(altus_tracker_library
  STATIC
  ${altus_tracker_sources}
)

include(GNUInstallDirs)

# set_property(
#     TARGET gnuradio-op25_repeater
#     PROPERTY INSTALL_RPATH
#     "${CMAKE_INSTALL_LIBDIR}/altus-tracker"
# )

# set_target_properties(altus_tracker_library PROPERTIES OUTPUT_NAME "altus-tracker")

# Weird - when I add LIBRARY before the DESTINATION, it installs in the /usr/local/lib dir on a mac
# install(TARGETS altus_tracker_library 
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/altus-tracker
# )
    
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/source/" # source directory
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/altus-tracker" # target directory
    FILES_MATCHING # install only matched files
    PATTERN "*.h" # select header files
)

# install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/lib/json.hpp" # source file
#     DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/trunk-recorder" # target directory
# )

add_executable(altus-tracker source/main.cc) # ${trunk_recorder_sources})

target_link_libraries(altus-tracker altus_tracker_library ${GNURADIO_ALL_LIBRARIES} ${Boost_LIBRARIES} ${GNURADIO_OSMOSDR_LIBRARIES})

message(STATUS "All libraries:" ${GNURADIO_ALL_LIBRARIES})
if(NOT Gnuradio_VERSION VERSION_LESS "3.8")
    target_link_libraries(altus-tracker
    gnuradio::gnuradio-analog
    gnuradio::gnuradio-blocks
    gnuradio::gnuradio-digital
    gnuradio::gnuradio-filter
    gnuradio::gnuradio-pmt
    )  
endif()

install(TARGETS altus-tracker RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
