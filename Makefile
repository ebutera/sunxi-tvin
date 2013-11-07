CC = arm-angstrom-linux-gnueabi-gcc

CFLAGS = -Wall 
LDFLAGS = -static 

#CFLAGS = -Wall -I$(HOME)/lichee/buildroot/output/staging/usr/include
#LDFLAGS = -L$(HOME)/lichee/buildroot/output/target/usr/lib 
#-lSDL -lSDL_mixer -lmad -lSDL_ttf -lSDL_gfx -lpng -ljpeg -lfreetype -lSDL_image -lts -lm -ldirectfb -lfusion -ldirect -lz

EXE = $(shell basename `pwd`)
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

.PHONY : all
all: $(SRC) $(EXE)
	
$(EXE): $(OBJ) 
	$(CC) $(LDFLAGS) $(OBJ) -o $@
	
.c.o:
	$(CC) -c $(CFLAGS) $< -o $@
	
.PHONY : all
clean : 
	-rm *.o	$(EXE)
