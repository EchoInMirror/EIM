# EIM

## Build

### Prerequisites

- Microsoft Visual Studio 2022
- [CMake](https://cmake.org/)
- [VCPkg](https://github.com/microsoft/vcpkg)

### Install boost

```bash
vcpkg install boost:x64-windows-static
```

### Clone project

```bash
git clone --recursive https://github.com/EchoInMirror/EIM.git

cd EIM
```

### Build backend application

```bash
mkdir build

cd build

cmake -G "Visual Studio 17 2022" -A x64 ..
```

Then open **EIM/build/EIM.sln**

### Build frontend application

```bash
cd web

npm install --production

npm run build
```

## License

[AGPL-3.0](./LICENSE)

## Author

Shirasawa
