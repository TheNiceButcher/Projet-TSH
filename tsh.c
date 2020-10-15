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
#include "commande.h"
#include <glib.h>

int main(int argc, char const *argv[]) {
	int quit = 0; //Variable qui enregistre si on execute la commande "exit" qui permet de sortir du shell
	while (quit == 0)
	{
		int nb_arg_cmd; //Stocke le nombre d'arguments de la commande
		char **liste_argument =  recuperer_commande(&nb_arg_cmd);
		char *nom_commande = liste_argument[0];
		//On verifie si la commande est nulle / vide
		//Si oui on revient a la ligne
		if (nom_commande == NULL)
			printf("\n");
		//Sinon on traite la commande
		else
		{
			//Execution de la commande "exit" et depart du shell
			if(memmem(nom_commande,sizeof("exit"),"exit",sizeof("exit")))
			{
				printf("Au revoir\n");
				quit = 1;
			}
			//Execution de la commande "cd" apres avoir sauvegarder le répertoire courant
			gchar *g_get_current_dir (void);
			else if(memmem(nom_commande,sizeof("cd"),"cd",sizeof("cd")))
           {
             gdir =nom_commande ;
             dir = strcat(gdir, "/");
             to = strcat(dir,liste_argument[1]);
             chdir(to);
           }
			
			
			//Sinon execution de la commande voulue si possible
			else
			{
				int pid = fork();
				if (pid==0)
				{
					if(execvp(nom_commande,liste_argument)==-1) //Si execvp renvoie -1, la commande n'existe pas
						printf("Commande introuvable\n");
					exit(0);
				}
				wait(NULL);
			}
		}
		//Liberation de la memoire
		for(int i = 0; i < nb_arg_cmd;i++)
			free(liste_argument[i]);
		free(liste_argument);
	}
	return 0;
}
