EXE=Httpd

SOURCE=$(wildcard *.cpp)
OBJ=$(patsubst *.cpp, *.o, $(SOURCE))


LIBS := pthread 

CFLAGS=-Wall -fPIC -O2
CC=g++ -std=c++11

all:$(EXE)
$(EXE):$(OBJ)
	$(CC) $(CFLAGS) -o $@ $^  $(addprefix -l, $(LIBS)) 

%.o:%.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf *.o $(EXE) 