# EIM

## Build

### Prerequisites

- Microsoft Visual Studio 2022
- [CMake](https://cmake.org/)
- [VCPkg](https://github.com/microsoft/vcpkg)

### Install boost

```bash
vcpkg install boost:x64-windows-static protobuf:x64-windows-static
```

### Clone project

```bash
git clone --recursive https://github.com/EchoInMirror/EIM.git

cd EIM
```

### Build backend application

```bash
npm install

npm run generate:proto

mkdir build

cd build

cmake -G "Visual Studio 17 2022" -A x64 .. -DCMAKE_TOOLCHAIN_FILE=<VCPkg install location>/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static
```

Then open **EIM/build/EIM.sln**

### Development

```bash

npm start
```

### Build frontend application

```bash
npm run build
```

## License

[AGPL-3.0](./LICENSE)

## Author

Shirasawa
