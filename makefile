EXE=./httpd.cpp

SOURCE=$(wildcard *.cpp)
OBJ=$(patsubst *.cpp, *.o, $(SOURCE))

INC=
LIBPATH=

LIB=-pthread

CFLAGS=-Wall -fPIC -O2
CC=g++ -std=c++11

all:$(EXE)
$(EXE):$(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(addprefix -L, $(LIBPATH)) $(addprefix -l, $(LIB)) 

%.o:%.cpp
	$(CC) $(CFLAGS) -o $@ -c $< $(addprefix -I, $(INC))

clean:
	rm -rf *.d *.o $(EXE) 