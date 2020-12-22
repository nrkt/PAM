include ../Makeheader 

test:	lineitem.h tables.h utils.h readCSV.h test.cpp
	$(CC) $(CFLAGS) -w test.cpp -o test $(LFLAGS)

runtest20: test
	NUM_THREADS=20 numactl --cpunodebind=0 --membind=0 ./test -q -s 10

runtest1: test
	NUM_THREADS=1 numactl --cpunodebind=0 --membind=0 ./test -q -s 10

runtest64: test
	NUM_THREADS=64 numactl -i all -C 0-32,72-104 ./test -q -s 100

clean:
	rm -f test
