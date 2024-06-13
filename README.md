# Tiny Language Compiler

> [!NOTE]
> Tested on: <br>
> macOS Sonoma 14.5 <br>
> Apple clang version 15.0.0 (clang-1500.3.9.4) <br>
> Target: arm64-apple-darwin23.5.0

### Build
```sh
cmake -GNinja -B build -S . && ninja -C build
```

### Run
```
build/bin/ty <source_file>
```

> [!NOTE]
> Propably want to redirect stderr to /dev/null using 2>/dev/null
