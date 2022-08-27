### build env
* cmake version >= 3.16
* gcc version >= 4.8.5(require c++11)

## build step
* cmake -Bbuild [-DCMAKE_BUILD_TYPE=<Debug | Release>]
* cmake --build build
* cd build/test && ctest