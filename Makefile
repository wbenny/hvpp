CXX         := g++
CXXFLAGS    := \
	-c \
	-O1 \
	-std=c++17 \
	-mcmodel=kernel \
	-I./src/hvpp \
	-Wall \
	-fno-exceptions \
	-fno-rtti \
	-fno-stack-protector \
	-fno-pic \
	-Wno-multichar \
	-Wno-invalid-offsetof \
	-Wno-switch \
	-Wno-strict-aliasing # TEMPORARY


AS          := as
ASFLAGS     := \
	-c \
	-I./src/hvpp

AR          := ar

SOURCES_C   := \
	$(wildcard src/hvpp/lib/linux/*.c)
OBJECTS_C   := src/hvpp/lib/linux/main.o

SOURCES_CPP := \
	$(wildcard src/hvpp/*.cpp) \
	$(wildcard src/hvpp/ia32/linux/*.cpp) \
	$(wildcard src/hvpp/lib/*.cpp) \
	$(wildcard src/hvpp/lib/vmware/*.cpp) \
	$(wildcard src/hvpp/lib/linux/*.cpp) \
	$(wildcard src/hvpp/hvpp/*.cpp) \
	$(wildcard src/hvpp/hvpp/vmexit/*.cpp)
OBJECTS_CPP := $(SOURCES_CPP:%.cpp=%.cpp.o)

SOURCES_S   := \
	$(wildcard src/hvpp/ia32/linux/*.S) \
	$(wildcard src/hvpp/lib/vmware/*.S) \
	$(wildcard src/hvpp/hvpp/*.S)
OBJECTS_S   := $(SOURCES_S:%.S=%.S.o)

ARCHIVE     := hvpp.a

#
# MODULE
#

BIN         := hvpp.ko
KVERSION    := 4.15.0-33-generic
KDIR        := /lib/modules/$(KVERSION)
KBUILD      := $(KDIR)/build
PWD         := $(shell pwd)

hvpp-objs   := $(OBJECTS_C) $(ARCHIVE)
ccflags-y   := -g -fno-stack-protector
obj-m       += hvpp.o

all: $(SOURCES_CPP) $(SOURCES_S) $(ARCHIVE)
	echo $(OBJECTS_S)
	make -C $(KBUILD) M=$(PWD) modules

$(ARCHIVE): $(OBJECTS_CPP) $(OBJECTS_S)
	$(AR) r $(ARCHIVE) $(OBJECTS_CPP) $(OBJECTS_S)

%.cpp.o: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

%.S.o: %.S
	$(CC) $(ASFLAGS) $< -o $@

clean:
	rm -f $(ARCHIVE) $(OBJECTS_CPP) $(OBJECTS_S)
	make -C $(KBUILD) M=$(PWD) clean

rload:
	rsync $(BIN) benny@192.168.63.133:/home/benny
	ssh -t benny@192.168.63.133 "sudo insmod /home/benny/$(BIN)"

