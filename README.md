# Tiny Language Compiler

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
