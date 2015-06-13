all : joytest thrustcurve

thrustcurve : thrustcurve.c
	gcc -o $@ $^ -lm

joytest : joytest.c os_generic.c
	gcc -o $@ $^ -lpthread

clean :
	rm -rf *.o *~ joytest

