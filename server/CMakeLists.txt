
cmake_minimum_required(VERSION 3.14)

project(AeroStructAPI LANGUAGES CXX)

set(
    mysql-concpp_DIR
    "C:/Program Files/MySQL/MySQL Connector C++ 26.10"
    CACHE PATH "MySQL Connector/C++ installation directory"
)

find_package(mysql-concpp CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)

# Collect existing AeroStruct core modules.
file(
    GLOB AEROSTRUCT_CORE_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/*.cpp"
)

# Collect API route modules.
file(
    GLOB API_ROUTE_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/routes/*.cpp"
)

# Keep the console application's main() separate from the API.
list(
    FILTER AEROSTRUCT_CORE_SOURCES
    EXCLUDE REGEX "/main\\.cpp$"
)

# Build the API together with the existing C++ modules.
add_executable(
    AeroStructAPI
    api_main.cpp
    ${AEROSTRUCT_CORE_SOURCES}
    ${API_ROUTE_SOURCES}
)

target_compile_features(AeroStructAPI PRIVATE cxx_std_17)

target_include_directories(
    AeroStructAPI PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

target_link_libraries(
    AeroStructAPI PRIVATE
    mysql::concpp
    unofficial-sodium::sodium
)

if(WIN32)
    target_link_libraries(AeroStructAPI PRIVATE ws2_32)
endif()
