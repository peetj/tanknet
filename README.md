# TankNet

A tiny multiplayer top-down "tanks" style game for Windows (C++), optimized for low input latency.

- **Rendering/Input:** SDL2
- **Networking:** ENet (UDP w/ reliability)
- **Model:** server-authoritative, client-side prediction + reconciliation, snapshot interpolation

## Build (Windows)

This repo uses **vcpkg manifest mode**.

### Prereqs
- Visual Studio 2022 (Desktop development with C++)
- CMake 3.25+
- vcpkg

### Configure
```powershell
# from repo root
$env:VCPKG_ROOT="C:\\src\\vcpkg"   # adjust
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake
```

### Build
```powershell
cmake --build build --config Release
```

## Run

### Start a server
```powershell
build\Release\tanknet.exe --server --port 27015
```

### Start two clients
```powershell
build\Release\tanknet.exe --client --host 127.0.0.1 --port 27015 --name P1
build\Release\tanknet.exe --client --host 127.0.0.1 --port 27015 --name P2
```

## Controls
- **WASD**: move
- **Mouse**: aim
- **Left click**: shoot
- **Esc**: quit

## Notes on latency
- Clients send input commands at 60Hz.
- The local player is predicted immediately (no render delay).
- Server is authoritative; clients reconcile to server snapshots.
- Remote player is interpolated (small buffer) to hide jitter.
