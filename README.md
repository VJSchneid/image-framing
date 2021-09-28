```
emcc ../../main.cpp -O3  -I ../root/include -L ../root/lib -ljpeg -lexiv2 -lexiv2-xmp -lexpat -lz -s ALLOW_MEMORY_GROWTH=1 -s NO_EXIT_RUNTIME=1  -s "EXPORTED_RUNTIME_METHODS=['ccall']" -s EXPORTED_FUNCTIONS='["_run"]' -s WASM=1 -DNO_MAIN -o image-framing.js
```
