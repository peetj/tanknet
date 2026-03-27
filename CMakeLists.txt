cmake_minimum_required(VERSION 3.25)
project(tanknet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Prefer fast builds + smaller latency.
if (MSVC)
  add_compile_options(/W4 /permissive- /Zc:__cplusplus)
  add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
  # Release: favor speed.
  add_compile_options($<$<CONFIG:Release>:/O2 /Ob2 /DNDEBUG>)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(SDL2 CONFIG REQUIRED)
find_package(unofficial-enet CONFIG REQUIRED)

add_executable(tanknet
  src/main.cpp
  src/net/NetCommon.h
  src/net/Serialize.h
  src/net/Serialize.cpp
  src/net/Protocol.h
  src/game/Math.h
  src/game/Sim.h
  src/game/Sim.cpp
  src/game/World.h
  src/game/World.cpp
  src/client/Client.h
  src/client/Client.cpp
  src/server/Server.h
  src/server/Server.cpp
  src/render/Render.h
  src/render/Render.cpp
)

target_include_directories(tanknet PRIVATE src)

target_link_libraries(tanknet PRIVATE SDL2::SDL2 SDL2::SDL2main unofficial::enet::enet)

if (MSVC)
  target_compile_options(tanknet PRIVATE /MP)
endif()
