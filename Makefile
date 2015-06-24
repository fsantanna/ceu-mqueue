all:
	ceu $(CEUFILE)
	gcc main.c $(CFLAGS) -lrt

qu:
	gcc qu.c -o qu -lrt

clean:
	rm -f *.exe _ceu_*

.PHONY: all clean
