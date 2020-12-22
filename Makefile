CXX=g++
STD=c++17
# Replace with your own path to include/ folder of v8
V8_INCLUDE=../v8/include
# Replace with static lib name and path to out.gn/ folder. Follow https://v8.dev/docs/embed
V8_MONOLITH_LIB=v8_monolith
V8_OUT_GN_PATH=../v8/out.gn

.PHONY: hello platform

hello:
	$(CXX) --std=$(STD) -I../v8 -I$(V8_INCLUDE) hello/hello.cc -o hello/hello -l$(V8_MONOLITH_LIB) -L$(V8_OUT_GN_PATH)/x64.release.sample/obj/ -pthread -DV8_COMPRESS_POINTERS

platform:
	$(CXX) --std=$(STD) -I../v8 -I$(V8_INCLUDE) platform/platform.cc -o platform/platform -l$(V8_MONOLITH_LIB) -L$(V8_OUT_GN_PATH)/x64.release.sample/obj/ -pthread -DV8_COMPRESS_POINTERS