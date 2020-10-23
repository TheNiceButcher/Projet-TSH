#ifndef COMMANDE_C
#define COMMANDE_C
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "shell.h"
#include "commande.h"
#include "tar_c.h"
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
Retourne les differentes options d'une commande s'il y en a et NULL sinon
*/
char **recherche_option(char **liste_argument,int nb_arg_cmd)
{
	if (nb_arg_cmd == 1)
		return NULL;
	char **result = malloc(nb_arg_cmd*sizeof(char*));
	int j = 0;
	for (int i = 0; i < nb_arg_cmd-1;i++)
	{
		if (liste_argument[i+1][0] == '-')
		{
			result[j] = malloc(25);
			strcpy(result[j],liste_argument[i+1]);
			j++;
		}
	}
	if (j == 0)
	{
		free(result);
		return NULL;
	}
	for(;j < nb_arg_cmd-1;j++)
	{
		result[j] = NULL;
	}
	return result;
}
/*
Recherche un fichier .tar dans un chemin et retourne l'index de la fin du fichier.tar
dans le chemin
*/
int recherche_fich_tar(char *chemin)
{
	char *tar = malloc(256);
	int index = 0;
	tar = decoup_nom_fich(chemin,&index);
	while (tar != NULL)
	{
		if (strcmp(&tar[strlen(tar)-4],".tar")==0)
		{
			break;
		}
		tar = decoup_nom_fich(chemin,&index);
	}
	free(tar);
	return index;
}
/*
Gere le traitement de commandes réalisable sur les tar

*/
int traitement_commandeTar(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	char *nom_commande = malloc(10);
	strcpy(nom_commande,liste_argument[0]);
	int (*tab[9])(char**,int,shell*)= {&cd,&ls,&cat,&mkdir_tar,&rmdir_tar,&mv,&pwd,&cp,&rm};
	char *cmd_tarballs[] = {"cd","ls","cat","mkdir","rmdir","mv","pwd","cp","rm"};
	char *option[] = {NULL,"-l",NULL,NULL,NULL,NULL,NULL,"-r","-r"};
	for(int i = 0; i < 9;i++)
	{
		if(strcmp(nom_commande,cmd_tarballs[i])==0)
		{
			if (tsh->tarball == 0 && i != 0 && i != 6)
			{
				int j = 1;
				//Si oui, on verifie si les arguments ont un contexte tar ou non
				for(;j < nb_arg_cmd;j++)
				{
					//Si on est dans un contexte tar, on arrete la boucle
					if (contexteTarball(liste_argument[j]))
					{
						break;
					}
				}
				//Si la boucle est alle juste qu'au bout, on lance exec
				if (j==nb_arg_cmd)
				{
					int fils = fork();
					if (fils == -1)
					{
						perror("");
						return 1;
					}
					if (fils == 0)
					{
						execvp(nom_commande,liste_argument);
						exit(0);
					}
					wait(NULL);
					return 0;
				}

			}

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
	//Si on a pas d'argument pour ls, on considere le repertoire courant comme argument
	if (nb_arg_cmd == 1)
	{
		liste_argument[1] = malloc(2);
		strcpy(liste_argument[1],".");
		nb_arg_cmd++;
	}
	int option = (recherche_option(liste_argument,nb_arg_cmd)!=NULL);

	for(int i = 1; i < nb_arg_cmd;i++)
	{
		if (liste_argument[i][0] == '-')
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
			strcpy(fichier,tsh->repertoire_courant);
			strcat(fichier,"/");
			strcat(fichier,liste_argument[i]);
			char *simplified_path = malloc(1024);
			strcpy(simplified_path,simplifie_chemin(fichier));
			if (contexteTarball(simplified_path) || tsh->tarball == 1)
			{
				fichier[strlen(simplified_path)-1] = '\0';
				//Fichier .tar
				if (strcmp(&simplified_path[strlen(fichier)-4],".tar")==0)
				{
					char **list = list_fich(simplified_path);
					if (list == NULL)
					{
						printf("Erreur\n");
					}
					else
					{
						int i = 0;
						while (list[i]!=NULL)
						{
							printf("%s\n",list[i]);
							i++;
						}
					}
					continue;
				}
				else
				{
					char *tar = malloc(256);
					int index = 0;
					tar = decoup_nom_fich(simplified_path,&index);
					while (tar != NULL)
					{
						if (strcmp(&tar[strlen(tar)-4],".tar")==0)
						{
							break;
						}
						tar = decoup_nom_fich(simplified_path,&index);
					}
					if (tar == NULL)
					 	printf("impossible\n");
					else
					{
						char *new_tar = malloc(1024);
						strncpy(new_tar,simplified_path,index);
						new_tar[strlen(new_tar)-1] = '\0';
						char **list = list_fich(new_tar);
						char *file_to_find = malloc(1024);
						strcpy(file_to_find,&simplified_path[index]);
						if (list == NULL)
						{
							printf("Erreur\n");
						}
						else
						{
							int i = 0;
							while (list[i]!=NULL)
							{
								if (strncmp(list[i],file_to_find,strlen(simplified_path) - index - 1)==0)
									printf("%s\n",list[i]);
								i++;
							}
						}
						continue;
					}
				}

			}
			else
			{
				struct stat file;
				if (stat(simplified_path,&file) == -1)
				{
					perror("ls");
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
			free(simplified_path);
		}
	}
	return 0;
}
int cd(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	if (nb_arg_cmd == 2)
	{
		char * nv_repr_courant = malloc(1024);
		//Prise en charge des chemins absolus depuis la racine
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
		if (nv_repr_courant[strlen(nv_repr_courant)-1] != '/')
			strcat(nv_repr_courant,"/");
		//Si on est dans un tar a la fin
		if (contexteTarball(simplifie_chemin(nv_repr_courant)))
		{
			strcpy(nv_repr_courant,simplifie_chemin(nv_repr_courant));
			//On parcourt le chemin jusqu'a trouver le fichier tar
			int i = 0;
			char *file = decoup_nom_fich(nv_repr_courant,&i);
			while (file != NULL)
			{
				if (strcmp(&file[strlen(file)-4],".tar")==0)
					break;
				file = decoup_nom_fich(nv_repr_courant,&i);
			}
			free(file);
			//Si on est alle jusqu'au bout du nouveau repertoire, on sera a la "racine" d'un tar
			char *tar = malloc(strlen(nv_repr_courant));
			for(int f = 0; f < i;f++)
			{
				tar[f] = nv_repr_courant[f];
			}
			tar[i-1] = '\0';
			struct stat test;
			if (stat(tar,&test) != -1)
			{
				//Le repertoire se situe dans le tar
				if (i != strlen(nv_repr_courant))
				{
					char **list = list_fich(tar);
					if (list)
					{
						char *repr_dans_tar = malloc(strlen(nv_repr_courant));
						strcpy(repr_dans_tar,&nv_repr_courant[i]);
						if(repr_dans_tar[strlen(repr_dans_tar)-1] != '/')
						 	strcat(repr_dans_tar,"/");
						int j = 0;
						while(list[j]!=NULL)
						{
							if (strcmp(repr_dans_tar,list[j])==0)
							{
								strcpy(tsh->repertoire_courant,nv_repr_courant);
								tsh->tarball = 1;
								break;
							}
							j++;
						}
						if (list[j]==NULL)
						{
							printf("cd: Aucun dossier %s dans %s\n",liste_argument[1],tar);
						}

					}
					else
					{
						printf("Erreur\n");
					}
				}
				//Le repertoire se situe a la racine du tar
				else
				{
					strcpy(tsh->repertoire_courant,nv_repr_courant);
					tsh->tarball = 1;
				}
			}
			else
			{
				char * error = malloc(1024);
				sprintf(error,"cd %s",tar);
				perror(error);
				free(error);
			}
			free(tar);
		}
		//Sinon, on change directement avec chdir
		else
		{
			strcpy(nv_repr_courant,simplifie_chemin(nv_repr_courant));
			if (nv_repr_courant[strlen(nv_repr_courant) - 1] != '/')
			{
				int i = strlen(nv_repr_courant);
				nv_repr_courant[i] = '/';
				nv_repr_courant[i+1] = '\0';
			}
			//Si simplifie_chemin donne le mot vide, on est à la racine
			if(strlen(nv_repr_courant)==0)
				strcpy(nv_repr_courant,"/");
			if (chdir(nv_repr_courant)==-1)
			{
				perror("cd impossible");
			}
			else
			{
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
/*
Implemente cat
*/
int cat(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	//Parcours des arguments de cat
	for (int i = 1;i < nb_arg_cmd;i++)
	{
		int fd;
		char * file = malloc(1024);
		strcpy(file,tsh->repertoire_courant);
		strcat(file,"/");
		strcat(file,liste_argument[i]);
		char *simple = malloc(1024);
		strcpy(simple,simplifie_chemin(file));
		//Si l'argument est dans un .tar ou est en un
		if (contexteTarball(simple))
		{
			//Fichier .tar
			if (strcmp(&simple[strlen(simple)-5],".tar/")==0)
			{
				printf("cat %s : Pas de cat sur un .tar\n",simple);
				continue;
			}
			//Fichier dans un .tar
			else
			{
				char *tar = malloc(1024);
				int index = recherche_fich_tar(simple);
				strncpy(tar,simple,index);
				tar[strlen(tar)-1] = '\0';
				char **list = list_fich(tar);
				char *file_to_find = malloc(1024);
				strcpy(file_to_find,&simple[index]);
				if (list == NULL)
				{
					printf("Erreur\n");
				}
				else
				{
					file_to_find[strlen(file_to_find)-1] = '\0';
					affiche_fichier_tar(tar,file_to_find);
				}
				continue;

			}
		}
		//Fichier en dehors d'un tar
		else
		{
			simple[strlen(simple)-1] = '\0';
			fd = open(simple,O_RDONLY);
			if(fd==-1)
			{
				char * error = malloc(1024);
				sprintf(error,"cat %s",simple);
				perror(error);
				free(error);
				continue;
			}
			char buffer[1024];
			int lus = 0;
			while ((lus = read(fd,buffer,1024)) > 0)
			{
				write(1,buffer,lus);
			}
		}

	}
	return 0;
}
#endif
