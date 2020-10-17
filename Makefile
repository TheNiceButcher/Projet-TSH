FLAGS = -Wall -pedantic -g
FILES = tsh.o commande.o shell.o
all: $(FILES)
	gcc $(FLAGS) $(FILES) -o tsh -lreadline
tsh.o: shell.c commande.c tsh.c
	gcc -c $(FLAGS) tsh.c -o tsh.o
commande.o: shell.c commande.c
	gcc -c $(FLAGS) commande.c -o commande.o
shell.o: shell.c commande.c
	gcc -c $(FLAGS) shell.c -o shell.o

clean:
	rm tsh *.o
