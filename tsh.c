#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include "commande.h"
#include "shell.h"

int main(int argc, char const *argv[]) {
	//Structure shell qui repertorie la variable pour sortir du shell, le repertoire courante et si on est dans un tarball
	shell tsh;
	tsh.quit = 0;
	tsh.repertoire_courant = malloc(1024);
	strcpy(tsh.repertoire_courant,getcwd(NULL,1024));
	tsh.tarball = 0;
	while (tsh.quit == 0)
	{
		int nb_arg_cmd; //Stocke le nombre d'arguments de la commande
		char **liste_argument =  recuperer_commande(&nb_arg_cmd);
		traitement_commande(liste_argument,nb_arg_cmd,&tsh);
		//Liberation de la memoire
		for(int i = 0; i < nb_arg_cmd;i++)
			free(liste_argument[i]);
		free(liste_argument);
	}
	return 0;
}