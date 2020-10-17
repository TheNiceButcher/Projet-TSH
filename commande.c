#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "shell.h"
#include "commande.h"
#include "tar.h"
/*
	Fichier qui gere les tarballs et les commandes associees
*/
/*
Verifie si le fichier est un tarball (fini par ".tar")
Renvoie 1 si le fichier est un Tarball, 0 sinon
*/
int estTarball(char *nom_fichier)
{
		if(strcmp(&nom_fichier[strlen(nom_fichier)-4],".tar")==0)
			return 1;
		return 0;
}
/*
Verifie si le chemin en argument contient un tarball
Renvoie 1 si on est dans le contexte d'un Tarball, 0 sinon
*/
int contexteTarball(char const *chemin)
{
	if (chemin == NULL)
		return 0;
	else
	{
		int i = 0;
		while (chemin[i] != '\0')
		{
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
			char *fichier = malloc(lg_mot + 1);
			strncpy(fichier,&chemin[debut_mot],lg_mot);
			if (estTarball(fichier))
			{
				free(fichier);
				return 1;
			}
			free(fichier);
		}
		return 0;
	}
}
/*
Verifie si la commande en argument est une commande fonctionnant dans un tarball
Renvoie 1 si c'est le cas, 0 sinon
*/
int estCommandeTar(char *mot_commande)
{
	char *cmd_tarballs[] = {"cd","ls","cat","mkdir","rmdir","mv","pwd","cp","rm"};
	for (int i = 0; i < 9;i++)
	{
		if (strcmp(mot_commande,cmd_tarballs[i])==0)
			return 1;
	}
	return 0;
}
/*

*/
int traitement_commandeTar(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	char *nom_commande = malloc(10);
	strcpy(nom_commande,liste_argument[0]);
	int (*tab[9])(char**,int,shell*)= {&cd,&ls,&cat,&mkdir_tar,&rmdir_tar,&mv,&pwd,&cp,&rm};
	char *cmd_tarballs[] = {"cd","ls","cat","mkdir","rmdir","mv","pwd","cp","rm"};
	for(int i = 0; i < 9;i++)
	{
		if(strcmp(nom_commande,cmd_tarballs[i])==0)
		{
			tab[i](liste_argument,nb_arg_cmd,tsh);
			return 0;
		}
	}
	return 1;
}
/*
Traite la commande ls
*/
int ls(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	int option = 0;
	//Recherche de l'option '-l'
	for(int i = 1;i < nb_arg_cmd;i++)
	{
		//Si on trouve une option, on verifie si c'est -l
		if (liste_argument[i][0] == '-')
		{
			//Si ce n'est pas -l, on arrete la fonction
			if (strcmp(liste_argument[i],"-l"))
			{
				printf("Option %s non traitee\n",liste_argument[i]);
				return 1;
			}
			//On stocke la position de -l
			option = i;
		}
	}
	for(int i = 1; i < nb_arg_cmd;i++)
	{
		if (i==option)
		{
			continue;
		}
		//Avec l'option -l
		if (option)
		{
			printf("A faire\n");
		}
		//Sans l'option -l
		else
		{
			char *fichier = malloc(255);
			pwd(liste_argument,nb_arg_cmd,tsh);
			strcpy(fichier,tsh->repertoire_courant);
			strcat(fichier,"/");
			strcat(fichier,liste_argument[i]);
			struct stat file;
			stat(fichier,&file);
			if (contexteTarball(fichier))
			{
				printf("Tarball %s\n",fichier);
			}
			//Si c'est un fichier regulier, on affiche juste le nom
			if (S_ISREG(file.st_mode))
				printf("%s\n",liste_argument[i]);
			//Si c'est un repertoire, on le parcourt et on affiche les fichiers contenus dedans
			if (S_ISDIR(file.st_mode))
			{
				DIR * repr = opendir(fichier);
				struct dirent *dir = readdir(repr);
				while (dir)
				{
					if (dir->d_name[0] != '.')
					{
						printf("%s\n",dir->d_name);
					}
					dir = readdir(repr);
				}
				closedir(repr);
			}
		}
	}
	return 0;
}
int cd(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	if (nb_arg_cmd == 2)
	{
		char * nv_repr_courant = malloc(1024);
		if (liste_argument[1][0] == '/')
		{
			strcpy(nv_repr_courant,liste_argument[1]);
		}
		else
		{
			strcpy(nv_repr_courant,tsh->repertoire_courant);
			if(tsh->repertoire_courant[strlen(tsh->repertoire_courant)-1] != '/')
				strcat(nv_repr_courant,"/");
			strcat(nv_repr_courant,liste_argument[1]);
		}
		//Si on est dans un tarball ou qu'on y va
		if (contexteTarball(nv_repr_courant) || tsh->tarball == 1)
		{
			if(tsh->tarball)
			{
				printf("A faire\n");
			}
			struct stat test;
			if (stat(nv_repr_courant,&test) != -1)
			{
				strcpy(tsh->repertoire_courant,nv_repr_courant);
				tsh->tarball = 1;
			}
			else
			{
				perror("");
			}
		}
		//Sinon, on change directement avec chdir
		else
		{
			if (nv_repr_courant[strlen(nv_repr_courant) - 1] != '/')
			{
				int i = strlen(nv_repr_courant);
				nv_repr_courant[i] = '/';
				nv_repr_courant[i+1] = '\0';
			}
			strcpy(nv_repr_courant,simplifie_chemin(nv_repr_courant));
			//Si simplifie_chemin donne le mot vide, on est à la racine
			if(strlen(nv_repr_courant)==0)
				strcpy(nv_repr_courant,"/");
			if (chdir(nv_repr_courant)==-1)
			{
				perror("cd impossible");
			}
			else
			{
				printf("%s\n",simplifie_chemin(nv_repr_courant));
				strcpy(tsh->repertoire_courant,nv_repr_courant);
				tsh->tarball = 0;
			}
		}
		free(nv_repr_courant);
	}
	else
	{
		if (nb_arg_cmd > 2)
			printf("cd : Trop d'arguments\n");
		else
			printf("cd : Pas assez d'arguments\n");
	}
	return 0;
}
int pwd(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	/*if (tsh->tarball)*/
	printf("%s\n",tsh->repertoire_courant);
	return 0;
}
int cp(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	printf("cp en construction\n");
	return 0;
}
int rm(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	printf("rm en construction\n");
	return 0;
}
int mkdir_tar(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	printf("mdir en construction\n");
	return 0;
}
int rmdir_tar(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	printf("rmdir en construction\n");
	return 0;
}
int mv(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	printf("mv en construction\n");
	return 0;
}
int cat(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	printf("cat en construction\n");
	return 0;
}
