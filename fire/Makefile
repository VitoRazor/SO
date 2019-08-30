GPU=1
CUDNN=1
OPENCV=1
LIBSO=1

# set GPU=1 and CUDNN=1 to speedup on GPU


DEBUG=0

ARCH= -gencode arch=compute_30,code=sm_30 \
      -gencode arch=compute_35,code=sm_35 \
      -gencode arch=compute_50,code=[sm_50,compute_50] \
      -gencode arch=compute_52,code=[sm_52,compute_52] \
	  -gencode arch=compute_61,code=[sm_61,compute_61]

OS := $(shell uname)




VPATH=./src/

ifeq ($(LIBSO), 1)
LIBNAMESO=ji.so
APPNAMESO=uselib
endif

CC=gcc
CPP=g++
NVCC=nvcc 
OPTS=-Ofast
LDFLAGS= -lm -pthread 
COMMON= 
CFLAGS=-Wall -Wfatal-errors -Wno-unused-result -Wno-unknown-pragmas

ifeq ($(DEBUG), 1) 
OPTS= -O0 -g
else

endif

CFLAGS+=$(OPTS)

ifeq ($(OPENCV), 1) 
COMMON+= -DOPENCV
CFLAGS+= -DOPENCV
LDFLAGS+= `pkg-config --libs opencv` 
COMMON+= `pkg-config --cflags opencv` 
endif
ifeq ($(GPU), 1)
COMMON+= -DGPU -I/usr/local/cuda/include/
CFLAGS+= -DGPU
ifeq ($(OS),Darwin) #MAC
LDFLAGS+= -L/usr/local/cuda/lib -lcuda -lcudart -lcublas -lcurand
else
LDFLAGS+= -L/usr/local/cuda/lib64 -L/usr/local/cuda/lib64/stubs -lcuda -lcudart -lcublas -lcurand
endif
endif

ifeq ($(CUDNN), 1)
COMMON+= -DCUDNN
ifeq ($(OS),Darwin) #MAC
CFLAGS+= -DCUDNN -I/usr/local/cuda/include
LDFLAGS+= -L/usr/local/cuda/lib -lcudnn
else
CFLAGS+= -DCUDNN -I/usr/local/cudnn/include
LDFLAGS+= -L/usr/local/cudnn/lib64 -lcudnn
endif
endif

DEPS = $(wildcard src/*.h) Makefile

export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH 
ifeq ($(LIBSO), 1) 
CFLAGS+= -fPIC
#src/autolink.h src/config.h src/features.h src/forwards.h src/json.h src/reader.h src/value.h src/#writer.h	
$(APPNAMESO): $(LIBNAMESO) src/ji.hpp src/demo.cpp 
	$(CPP) -std=c++11 $(COMMON) $(CFLAGS) -o $@ src/demo.cpp src/jsoncpp.cpp src/ji.cpp src/ji.h src/ji_util.h src/roi/BoostInterface.cpp src/roi/BoostInterface.h $(LDFLAGS) -L ./ -l:$(LIBNAMESO)
endif
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH

backup:
	mkdir -p backup
results:
	mkdir -p results

.PHONY: clean

clean:
	rm -rf  $(APPNAMESO)

