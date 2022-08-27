### build env
* cmake version >= 3.16
* gcc version >= 4.8.5(require c++11)

## build step
### windows
vs打开CMakeLists.txt
或者
#### debug
[与other相同](#other)
#### release
* cmake -Bbuild -DCMAKE_BUILD_TYPE=Release 
* cmake --build build --config Release
### other
<a href="other"></a>

* cmake -Bbuild [-DCMAKE_BUILD_TYPE=<Debug | Release>]
* cmake --build build
* cd build/test && ctest

### 测试用例
* src/protocol/srt/README.md

部分实现参考
* https://github.com/ZLMediaKit/ZLMediaKit.git
* https://github.com/Haivision/srt.git