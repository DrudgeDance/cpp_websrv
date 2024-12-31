# Hot-Reloadable Web Server

A modular C++ web server with hot-reloadable endpoints, routers, and controllers. This project demonstrates dynamic loading of C++ modules at runtime, allowing you to modify and reload components without restarting the server.

## Project Structure 

```
project/
├── src/
│   ├── controllers/       # Controller modules
│   │   ├── TimeController.cpp
│   │   └── WebController.cpp
│   ├── routers/           # Router modules
│   │   ├── ApiRouter.cpp
│   │   └── WebRouter.cpp
│   ├── endpoints/         # Endpoint modules
│   │   ├── TimeEndpoint.cpp
│   │   ├── HelloEndpoint.cpp
│   │   ├── EchoEndpoint.cpp
│   │   └── NewEndpoint.cpp
│   └── Manager.cpp        # Application orchestrator
├── include/
│   └── plugin.hpp         # Interface definitions
├── .vscode/               # VSCode configuration
│   ├── settings.json
│   └── c_cpp_properties.json
├── CMakeLists.txt         # Build system configuration
└── conanfile.txt          # Package dependencies
```

## Features

- Hot-reloadable endpoints, routers, and controllers
- Modular architecture with clean separation of concerns
- Dynamic library loading at runtime
- Automatic component discovery and loading
- Built with modern C++17

## Dependencies

- CMake 3.15 or higher
- Conan package manager 2.0
- C++17 compatible compiler
- Libraries (managed by Conan):
  - Boost
  - fmt

## Quick Start

1. Clone and prepare the build directory:

```bash
git clone <repository-url>
cd <project-name>
mkdir -p build
cd build
```

2. Install dependencies and configure:

```bash
# Install dependencies
conan install .. --output-folder=. --build=missing
# Configure with CMake
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
```

3. Build the project:

```bash
cmake --build .
```

4. Run the server:

```bash
cd bin
./server
```

## Development Workflow

### Building Individual Components

```bash
# Build specific endpoints
cmake --build . --target TimeEndpoint
cmake --build . --target HelloEndpoint
cmake --build . --target EchoEndpoint
cmake --build . --target NewEndpoint

# Build routers
cmake --build . --target ApiRouter
cmake --build . --target WebRouter

# Build controllers
cmake --build . --target TimeController
cmake --build . --target WebController
```

### Testing Endpoints

With the server running:

```bash
# Get current time
curl http://localhost:63090/time

# Get greeting
curl http://localhost:63090/hello

# Echo test
curl -X POST -d "Hello World" http://localhost:63090/echo

# Test new endpoint
curl http://localhost:63090/new
```

### Hot Reloading

1. Modify an endpoint (e.g., `src/endpoints/TimeEndpoint.cpp`)
2. Rebuild just that endpoint:

```bash
cmake --build . --target TimeEndpoint
```

3. The server will automatically detect the change and reload the component.
4. Test the endpoint to see your changes.

## Project Organization

### Components

- **Endpoints**: Individual API endpoints (`/time`, `/hello`, etc.)
- **Routers**: Group and manage related endpoints
- **Controllers**: High-level application logic
- **Manager**: Orchestrates component loading and request handling

### Output Structure

```
build/
└── bin/
    ├── endpoints/       # Endpoint libraries
    ├── routers/         # Router libraries
    ├── controllers/     # Controller libraries
    ├── libmanager.dylib # Application manager
    └── server           # Main executable
```

## Troubleshooting

### Common Issues

1. **Missing Dependencies**
   ```bash
   conan install .. --output-folder=. --build=missing
   ```

2. **Build Errors**
   ```bash
   # Clean build
   cd build
   rm -rf *
   conan install .. --output-folder=. --build=missing
   cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
   cmake --build .
   ```

3. **Component Not Loading**
   - Check file permissions
   - Verify library path
   - Look for loading errors in server output

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## License

Do whatever you want with it.
