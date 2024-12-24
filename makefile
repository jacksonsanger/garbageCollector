mallocTest: duMalloc.o mallocTest.o
	gcc duMalloc.o mallocTest.o -o mallocTest

mallocTest.o: mallocTest.c dumalloc.h
	gcc mallocTest.c -c

duMalloc.o: duMalloc.c dumalloc.h
	gcc duMalloc.c -c

clean:
	rm -f *.o
	rm -f mallocTest