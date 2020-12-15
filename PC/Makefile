CC = g++

#lxrad automatic generated block start, don't edit below!

FLAGS = -I. -Wall -ggdb `lxrad-config --cxxflags`

LIBS = `lxrad-config --libs`  

OBJS = pMicroDSO.o MicroDSO1.o  Wrapper.o lcd_ili9341.o

SRC_DIR := src

SRC_FILES := $(shell find $(SRC_DIR)/ -name '*.cpp')
	
#SRC_FILES := $(wildcard $(SRC_DIR)/*/*.cpp) 
OBJS += $(patsubst %.cpp,%.o,$(SRC_FILES))
	
#lxrad automatic generated block end, don't edit above!


all: $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -oMicroDSO $(LIBS)

%.o: %.cc
	$(CC) -c $(FLAGS) $< 
	
%.o : %.cpp
	$(CC) -c $(FLAGS) $< -o $@	
	 


run: all
	./MicroDSO

install:

clean:
	rm -f MicroDSO *.o core src/*/*.o
