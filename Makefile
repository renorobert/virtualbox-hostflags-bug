exploit: helpers.c poc.c
	gcc -Wall -ggdb -o poc helpers.c poc.c 
clean:
	rm poc
