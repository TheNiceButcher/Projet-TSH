FLAGS = -Wall -pedantic -g
FILES = tsh.o commande.o shell.o tar_c.o
all: $(FILES)
	gcc $(FLAGS) $(FILES) -o tsh -lreadline
tsh.o: shell.c commande.c tsh.c tar_c.c
	gcc -c $(FLAGS) tsh.c -o tsh.o
commande.o: shell.c commande.c tar_c.c
	gcc -c $(FLAGS) commande.c -o commande.o
shell.o: shell.c commande.c tar_c.c
	gcc -c $(FLAGS) shell.c -o shell.o
tar_c.o: tar_c.c
	gcc -c $(FLAGS) tar_c.c -o tar_c.o
clean:
	rm tsh *.o
