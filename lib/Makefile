All: LIB static

LIB:
	g++ -shared -fPIC -o liblazurite.so dyliblazurite.cpp
	sudo cp liblazurite.so /usr/lib

static:
	g++ -c dyliblazurite.cpp -o liblazurite.o
	ar r liblazurite.a liblazurite.o

clean:
	-rm -r *.o *.a *.so
