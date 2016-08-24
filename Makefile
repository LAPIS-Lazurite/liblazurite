
LIB:
	g++ -shared -fPIC -o liblazurite.so dyliblazurite.cpp
	sudo cp liblazurite.so /usr/lib

static:
	g++ -c dyliblazurite.cpp -o liblazurite.o
	ar r liblazurite.a liblazurite.o

tx:
	g++ -I./ -o test_tx test_tx.cpp -L/usr/lib -llazurite

raw:
	g++ -I./ -o test_raw test_raw.cpp -L/usr/lib -llazurite

rx:
	g++ -I./ -o test_rx test_rx.cpp -L/usr/lib -llazurite

link:
	g++ -I./ -o test_link test_link.cpp -L/usr/lib -llazurite

clean:
	-rm *.o *.a *.so test_tx test_rx test_raw
