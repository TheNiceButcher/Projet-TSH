#ifndef COMMANDE_C
#define COMMANDE_C
#define _GNU_SOURCE
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "commande.h"
#include "shell.h"
/*
	Fichier qui gere la recuperation de la commande
*/
/*
Fonction qui prend en argument la commande en entier et une adresse de son index
courant.
Elle parcourt la commande et renvoie le premier mot qu'elle trouve.
Elle ne recupere pas les espaces et renvoie NULL si on a fini de lire la commande
*/
char *decoup_mot(char *commande,int *index)
{
	char *mot = malloc(strlen(commande)*sizeof(char));
	int i = 0;
	int i_commande = *index;
	if (commande[i_commande] == '\n' || commande[i_commande] == '\0')
		return NULL;
	while (commande[i_commande] == ' ') //Pour ne pas compter les espaces entre les mots
		i_commande++;
	//On parcourt la commande jusqu'a la fin de la ligne ou au prochain espace
	while (commande[i_commande] != ' ' && commande[i_commande] != '\n' && commande[i_commande] != '\0')
	{
		mot[i] = commande[i_commande];
		i++;
		i_commande++;
	}
	mot[i] = '\0';
	//On n'est pas arrive en fin de ligne. On continue de parcourir la commande
	if ((commande[i_commande] != '\n'))
		*index = i_commande;
	//On arrive a la fin de la ligne
	if(i==0)
		return NULL;
	return mot;
}
/*

*/
char *decoup_nom_fich(char *chemin,int *index)
{
	if (chemin == NULL || chemin[*index]=='\0')
		return NULL;
	int i = *index;
	int debut_mot = i;
	int lg_mot = 0;
	while (chemin[i] != '/')
	{
		lg_mot++;
		i++;
		if (chemin[i] == '\0')
			break;
	}
	i++;
	*index = i;
	char *fichier = malloc(lg_mot + 1);
	strncpy(fichier,&chemin[debut_mot],lg_mot);
	return fichier;
}
/*
Simplifie le chemin absolu en enlevant les .. et .
*/
char *simplifie_chemin(char *chemin)
{
	if (chemin == NULL)
		return NULL;
	else
	{
		char *nvx_chemin = malloc(1024);
		int i = 0;
		int index = 0;
		char *name = decoup_nom_fich(chemin,&index);
		while(name != NULL)
		{
			if (strcmp(name,".") == 0)
			{
				name = decoup_nom_fich(chemin,&index);
				continue;
			}
			if ((strcmp(name,"..") == 0))
			{
				i--;
				i--;
				while(nvx_chemin[i] != '/')
				{
					i--;
				}
				nvx_chemin[i] = '\0';
				i++;
				name = decoup_nom_fich(chemin,&index);
				continue;
			}
			else
			{
				strcpy(&nvx_chemin[i],name);
				i += strlen(name) + 1;
				strcat(nvx_chemin,"/");
				name = decoup_nom_fich(chemin,&index);
			}
		}
		free(name);
		nvx_chemin[i] = '\0';
		return nvx_chemin;
	}
}
/*
Recupere la commande de l'utilisateur, renvoie la liste des arguments (mot != " ")
et Stocke son nombre dans l'adresse en argument
*/
char **recuperer_commande(int * taille_commande)
{
	char * commande = NULL;
	commande = readline(">");
	commande[strlen(commande)] = '\0';
	int index_commande = 0; //Stocke l'index courant de commande
	int taille_commande_max = 10;
	int taille_argument_max = 250;
	char **liste_argument = (char **)malloc(taille_commande_max*sizeof(char*));
	for (int l = 0;l < taille_commande_max;l++)
		liste_argument[l] = malloc(taille_argument_max * sizeof(char));
	int j = 1;
	char *nom_commande = decoup_mot(commande,&index_commande);
	liste_argument[0] = nom_commande;
	char *mot;
	//Tant qu'il y a des mots a recuperer(mot !=NULL), on les recupere
	while ((mot = decoup_mot(commande,&index_commande)))
	{
		if(j == taille_commande_max)
		{
			taille_commande_max *= 2;
			liste_argument = (char **)realloc(liste_argument,taille_commande_max*sizeof(char*));
		}
		int len = strlen(mot);
		if (len > taille_argument_max)
		{
			taille_argument_max = len*2;
			liste_argument[j] = realloc(liste_argument[j],taille_argument_max*sizeof(char));
		}
		liste_argument[j] = mot;
		j++;
	}
	//On stocke le nombre d'arguments dans l'adresse donne en parametre
	*taille_commande = j;
	for (;j < taille_commande_max;j++)
	{
		liste_argument[j] = NULL;
	}
	return liste_argument;
}
/*
Analyse la ligne de commande et traite la commande
*/
int traitement_commande(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	char *nom_commande = malloc(1024);
	//On verifie si la commande est nulle / vide
	//Si oui on revient a la ligne
	if (liste_argument[0] == NULL)
		printf("\n");
	//Sinon on traite la commande
	else
	{
		strcpy(nom_commande,liste_argument[0]);
		//Execution de la commande "exit" et depart du shell
		if(memmem(nom_commande,sizeof("exit"),"exit",sizeof("exit")))
		{
			printf("Au revoir\n");
			tsh->quit = 1;
		}
		else
		{
			//On verifie que la commande peut s'effectuer sur les tar ou non
			if (estCommandeTar(nom_commande))
			{
				traitement_commandeTar(liste_argument,nb_arg_cmd,tsh);
			}
			//Sinon execution de la commande voulue si possible
			else
			{
				int pid = fork();
				if (pid == -1)
					perror("Fils non cree");
				if (pid==0)
				{
					if(execvp(nom_commande,liste_argument)==-1) //Si execvp renvoie -1, la commande n'existe pas
						printf("Commande introuvable\n");
					exit(0);
				}
				wait(NULL);
			}
		}
	}
	free(nom_commande);
	return 0;
}
#endif
