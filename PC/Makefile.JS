CC = em++

#lxrad automatic generated block start, don't edit below!

FLAGS=  -s DEMANGLE_SUPPORT=0 \
        -s SAFE_HEAP=0 -s ASSERTIONS=0 -s TOTAL_MEMORY=128MB -s USE_SDL=2 -s USE_SDL_IMAGE=2 \
	-s SDL2_IMAGE_FORMATS='["png","bmp"]' -s USE_SDL_TTF=2 \
	-Os -I.  -Wall  -I../../LXRAD_SDL2/include/  --preload-file assets

LIBS= ../../LXRAD_SDL2/lib/liblxrad_SDL2_js.a 

OBJS = pMicroDSO.o MicroDSO1.o  Wrapper.o lcd_ili9341.o

SRC_DIR := src

SRC_FILES := $(shell find $(SRC_DIR)/ -name '*.cpp')
	
#SRC_FILES := $(wildcard $(SRC_DIR)/*/*.cpp) 
OBJS += $(patsubst %.cpp,%.o,$(SRC_FILES))
	
#lxrad automatic generated block end, don't edit above!


all: $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -oMicroDSO.html $(LIBS)

%.o: %.cc
	$(CC) -c $(FLAGS) $< 
	
%.o : %.cpp
	$(CC) -c $(FLAGS) $< -o $@	
	 


run: all
	./MicroDSO

install:
	cp -dvf MicroDSO.html ../docs
	cp -dvf MicroDSO.data ../docs
	cp -dfv MicroDSO.wasm ../docs
	cp -dfv MicroDSO.js ../docs

clean:
	rm -f MicroDSO *.o core src/*/*.o *.html *.data *.wasm *.js
