
coffe : coffe.o
	gcc $< -o coffe -lcurl

coffe.o : coffe.c
	gcc -c $<

clean :
	rm *.o coffe
