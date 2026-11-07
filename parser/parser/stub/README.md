# "stub"  usage:

## Description

- File libge_compiler.so ,libgraph.so are used in IR build application interface.

# Attention

- Don't link other library except libge_compiler.so ,libgraph.so, as they may be changed in the future.

# Usage

## Compile:   compile  the application invoking the IR build API.

Makefile:

```makefile
ATC_INCLUDE_DIR := $(ASCEND_PATH)/compiler/include
OPP_INCLUDE_DIR := $(ASCEND_PATH)/opp/op_proto/built-in/inc
LOCAL_DIR := $(shell pwd)
LOCAL_MODULE_NAME := ir_build
CC := g++
CFLAGS := -std=c++17 -g -Wall
SRCS := $(wildcard $(LOCAL_DIR)/main.cpp)
INCLUDES := -I $(OPP_INCLUDE_DIR) \
            -I $(ATC_INCLUDE_DIR)/graph \
            -I $(ATC_INCLUDE_DIR)/ge

LIBS := -L ${ASCEND_PATH}/compiler/lib64/stub \
    -lgraph \
    -lge_compiler
ir_build:
	mkdir -p out
	$(CC) $(SRCS) $(INCLUDES) $(LIBS) $(CFLAGS) -o ./out/$(LOCAL_MODULE_NAME)
clean:
	rm -rf out
```

```bash
make clean && make ir_build
```

## Run the application after set the LD_LIBRARY_PATH to include the real path of the library which locates in the directory of compiler/lib64

```bash
export LD_LIBRARY_PATH=${ASCEND_PATH}/compiler/lib64:$LD_LIBRARY_PATH
./out/ir_build
```
