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
int contexteTarball(char*chemin)
{
	if (chemin == NULL)
		return 0;
	else
	{
		int i = 0;
		char *fichier = decoup_nom_fich(chemin,&i);
		while (fichier != NULL)
		{
			if (estTarball(fichier))
			{
				free(fichier);
				return 1;
			}
			fichier = decoup_nom_fich(chemin,&i);
		}
		free(fichier);
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
	//Aucune option
	if (j == 0)
	{
		free(result);
		return NULL;
	}
	for(int h = j ;h < nb_arg_cmd-1;h++)
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
			//Gestion des options
			char ** options = recherche_option(liste_argument,nb_arg_cmd);
			int a_bonnes_options = 1;
			if (options && option[i] == NULL)
			{
				a_bonnes_options = 0;
			}
			else
			{
				if(options)
				{
					int m = 0;
					while (options[m]!=NULL)
					{
						if (strcmp(options[m],option[i]))
							a_bonnes_options = 0;
						m++;
					}
				}
			}
			//Contexte non tarball
			if (tsh->tarball == 0)
			{
				int j = 1;
				//On vérifie alors si un des arguments est dans un tarball on non
				for(;j < nb_arg_cmd;j++)
				{
					char *absol_path = malloc(strlen(liste_argument[j])+strlen(tsh->repertoire_courant)+3);
					strcpy(absol_path,tsh->repertoire_courant);
					strcat(absol_path,"/");
					strcat(absol_path,liste_argument[j]);
					//Si on est dans un contexte tar et qu'on a les bonnes options, on arrete la boucle
					if (contexteTarball(absol_path) && a_bonnes_options)
					{
						free(absol_path);
						break;
					}
					free(absol_path);
				}
				//Si la boucle est alle juste qu'au bout et que la commande n'est ni cd ni pwd, on lance exec
				if (j==nb_arg_cmd && i != 0 && i != 6)
				{
					int fils = fork();
					if (fils == -1)
					{
						perror("fork traitement_commandeTar");
						free(nom_commande);
						return 1;
					}
					if (fils == 0)
					{
						execvp(nom_commande,liste_argument);
						exit(0);
					}
					wait(NULL);
					free(nom_commande);
					return 0;
				}

			}
			//Cas contexte tarball et bonnes options
			tab[i](liste_argument,nb_arg_cmd,tsh);
			return 0;
		}
	}
	free(nom_commande);
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
			//OPtion -l pour un fichier dans un tar (a faire)
			if (contexteTarball(liste_argument[i]))
			{
				printf("%s:\n",liste_argument[i]);
			}
			//Option -l pour un fichier n'etant pas dans un tar
			else
			{
				//N'etant pas dans un .tar, un appel à execlp suffit
				if (fork()==0)
				{
					printf("%s:\n",liste_argument[i]);
					execlp("ls","ls","-l",liste_argument[i],NULL);
					exit(0);
				}
				wait(NULL);
			}
		}
		//Sans l'option -l
		else
		{
			char *fichier = malloc(strlen(tsh->repertoire_courant) + strlen(liste_argument[i]) + 2);
			strcpy(fichier,tsh->repertoire_courant);
			strcat(fichier,"/");
			strcat(fichier,liste_argument[i]);
			char *simplified_path = malloc(1024);
			strcpy(simplified_path,simplifie_chemin(fichier));
			//printf("%s\n",simplified_path);
			//Fichier dans un tarball
			if (contexteTarball(simplified_path))
			{
				simplified_path[strlen(simplified_path)-1] = '\0';
				if (simplified_path[strlen(simplified_path)-1] == '/')
				{
					simplified_path[(strlen(simplified_path))-1] = '\0';
				}
				//ls sur un Fichier .tar
				if (strcmp(&simplified_path[strlen(simplified_path)-4],".tar")==0)
				{
					char **list = list_fich(simplified_path);
					if (list == NULL)
					{
						printf("Erreur\n");
					}
					else
					{
						char **to_print = malloc(25*sizeof(char *));
						for (int f = 0; f < 25;f++)
						{
							to_print[f] = NULL;
						}
						int index = 0;
						int k = 0;
						while (list[k]!=NULL)
						{
							int d = 0;
							for(;d < index;d++)
							{
								if (strncmp(to_print[d],list[k],strlen(to_print[d])) == 0)
								{
									//printf("%s\n",to_print[d]);
									break;
								}
							}
							if (d == index)
							{
								printf("%s\n",list[k]);
								to_print[index] = malloc(strlen(list[k]));
								strcpy(to_print[index],list[k]);
								index++;
							}
							k++;
						}
						for (int f = 0; f < index;f++)
						{
							to_print[f] = NULL;
						}
						free(to_print);
					}
					free(simplified_path);
					continue;
				}
				//ls sur un fichier dans un fichier .tar
				else
				{
					//Recherche fichier .tar contenant le fichier
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
							//Recherche du fichier dans le fichier .tar
							int k = 0;
							int trouve = 0;
							char ** deja_affiche = malloc(20*sizeof(char*));
							for(int h = 0; h < 20;h++)
							{
								deja_affiche[h] = NULL;
							}
							int i_deja_trouve = 0;
							while (list[k]!=NULL)
							{
								if (strncmp(list[k],file_to_find,strlen(simplified_path) - index - 1)==0)
								{
									if (list[k][strlen(simplified_path) - index] == '\0'||list[k][strlen(simplified_path) - index] == '/')
									{
										int jpp = strlen(simplified_path) - index + 1;
										char *fich_to_print = decoup_nom_fich(list[k],&jpp);
										int d = 0;
										for (; d < i_deja_trouve;d++)
										{
											if (strcmp(fich_to_print,deja_affiche[d])==0)
												break;
										}
										if (d == i_deja_trouve && fich_to_print != NULL)
										{
											printf("%s",fich_to_print);
											//Affichage d'un '/' en fin de ligne pour les repertoires
											if (list[k][jpp-1] == '/')
												printf("/\n");
											else
											{
												printf("\n");
											}
											deja_affiche[i_deja_trouve] = malloc(strlen(fich_to_print)+1);
											strcpy(deja_affiche[i_deja_trouve],fich_to_print);
											i_deja_trouve++;
											trouve++;
										}
										fich_to_print = NULL;
									}
								}
								k++;
							}
							for(int h = 0; h < 20;h++)
							{
								deja_affiche[h] = NULL;
							}
							if (!trouve)
							{
								char *error= malloc(1024);
								sprintf(error,"ls %s: Aucun dossier ni fichier de ce nom\n",liste_argument[i]);
								write(STDERR_FILENO,error,strlen(error));
								free(error);
							}
						}
						continue;
					}
				}

			}
			//Fichier n'etant pas dans un .tar
			else
			{
				int fils = fork();
				if (fils == -1)
				{

				}
				if(fils==0)
				{
					execlp("ls","ls",liste_argument[i],NULL);
					exit(0);
				}
				wait(NULL);
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
							char *error= malloc(1024);
							sprintf(error,"cd: Aucun dossier %s dans %s\n",liste_argument[1],tar);
							write(STDERR_FILENO,error,strlen(error));
							free(error);
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
				strcpy(nv_repr_courant,simplifie_chemin(nv_repr_courant));
				strcpy(tsh->repertoire_courant,nv_repr_courant);
				tsh->tarball = 0;
			}
		}
		free(nv_repr_courant);
	}
	else
	{
		if (nb_arg_cmd > 2)
			write(STDERR_FILENO,"cd : Trop d'arguments\n",strlen("cd : Trop d'arguments\n"));
		else
			write(STDERR_FILENO,"cd : Pas assez d'arguments\n",strlen("cd : Pas assez d'arguments\n"));
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
	int option = (recherche_option(liste_argument,nb_arg_cmd)!=NULL);
	for(int i = 1; i < nb_arg_cmd; i++)
	{
		if (liste_argument[i][0] == '-')
			continue;
		char * simple = malloc(strlen(liste_argument[i]) + strlen(tsh->repertoire_courant)+3);
		strcpy(simple,tsh->repertoire_courant);
		strcat(simple,"/");
		strcat(simple,liste_argument[i]);
		int dossier = 0;
		if (simple[strlen(simple)-1] != '/')
		{
			strcat(simple,"/");
		}
		else
			dossier = 1;
		char * simple2 = simplifie_chemin(simple);
		//Fichier dans un .tar
		if (contexteTarball(simple2))
		{
			int index = recherche_fich_tar(simple2);
			//Si on doit supprimer un fichier  .tar
			if (index == strlen(simple2))
			{
				//Considerant les .tar comme des dossiers, on attend l'option pour le supprimer
				if (option)
				{
					simple2[strlen(simple2)-1] = '\0';
					if(unlink(simple2)==-1)
					{
						char *error = malloc(1024);
						sprintf(error,"rm %s :",simple2);
						perror(error);
						free(error);
					}
				}
				else
				{
					char *error= malloc(1024);
					sprintf(error,"rm %s: Veuillez utiliser l'option -r pour supprimer les .tar\n",liste_argument[i]);
					write(STDERR_FILENO,error,strlen(error));
					free(error);
				}
			}
			//Suppression dans un .tar
			else
			{
				char *tar = malloc(strlen(simple2));
				strncpy(tar,simple2,index);
				char *file_to_rm = malloc(strlen(simple2));
				strcpy(file_to_rm,&simple2[index]);
				if (dossier == 0)
					file_to_rm[strlen(file_to_rm)-1] = '\0';
				tar[strlen(tar)-1] = '\0';
				supprimer_fichier_tar(tar,file_to_rm,option);
			}
		}
		else
		{
			//Suppression fichier en dehors d'un .tar
			if (option == 0)
			{
				simple2[strlen(simple2)-1] = '\0';
				if (unlink(simple2) == -1)
				{
					char * error = malloc(1024);
					sprintf(error,"rm %s ",simple2);
					perror(error);
					free(error);
				}
			}
			else
			{
				int fils = fork();
				if (fils == -1)
				{
					perror("fork rm");
					continue;
				}
				if (fils==0)
				{
					execlp("rm","rm","-r",liste_argument[i],NULL);
					exit(0);
				}
				else
					wait(NULL);
			}
		}
		simple = NULL;
		simple2 = NULL;
	}
	return 0;
}
int mkdir_tar(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	for(int i = 1; i < nb_arg_cmd; i++)
	{
		char *fichier = malloc(strlen(liste_argument[i])+strlen(tsh->repertoire_courant)+1);
		strcpy(fichier,tsh->repertoire_courant);
		strcat(fichier,"/");
		strcat(fichier,liste_argument[i]);
		fichier[strlen(liste_argument[i])+strlen(tsh->repertoire_courant)] = '\0';
		char * file = simplifie_chemin(fichier);
		file[strlen(file)] = '\0';
		//Contexte tar
		if (contexteTarball(fichier))
		{
			int index = recherche_fich_tar(file);
			if (index == strlen(file))
			{
				printf("Création de .tar %s\n",liste_argument[i]);
			}
			else
			{
				char *tar = malloc(strlen(file));
				strncpy(tar,file,index);
				tar[index-1] = '\0';
				char *repr_to_create = &file[index];
				repr_to_create[strlen(file)-index+1] = '\0';
				if (repr_to_create[strlen(repr_to_create)-1] != '/')
				{
					printf("Merde");
				}
				creation_repertoire_tar(tar,repr_to_create);
			}
		}
		//En dehors d'un tar
		else
		{
			int fils = fork();
			if (fils == -1)
			{
				perror("fork mkdir");
				continue;
			}
			if (fils == 0)
			{
				execlp("mkdir","mkdir",liste_argument[i],NULL);
				exit(0);
			}
			wait(NULL);
		}
	}
	return 0;
}
int rmdir_tar(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	printf("rmdir en construction\n");
	return 0;
}
int mv(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
     struct stat stat_src;
     char *src, *dest, *src_final, *dest_final;
     //controler le nombre d'arguments
    if (nb_arg_cmd!= 3) {
      printf("erreur nombre d'arguments incorrect\n");
      exit(EXIT_FAILURE);
    }

     //copier source dans la variable src
    src = malloc(strlen(liste_argument[1]) + 1);
    strcpy(src, liste_argument[1]);

     //copier destination dans la variable dest
    dest = malloc(strlen(liste_argument[2]) + 1);
    strcpy(dest, liste_argument[2]);

     //verifier si le fichier source existe
    if (stat(src, &stat_src)== -1) {
        printf("le fichier %s n'existe pas\n",src);
        exit(EXIT_SUCCESS);
    }
      //verifier si dest est un chemin
     if(dest[0]=='/')
        {
            strcat(dest,"/");
            strcat(dest,src);
              if(rename(src,dest)!=0)
                printf("Error:\nDirectory not found\n");
         }
     else {
              //construction de la variable src_final
                 src_final = malloc(strlen(src) + 1 + strlen(tsh->repertoire_courant) + 1);
                 strcpy(src_final,tsh->repertoire_courant);
                 strcat(src_final,"/");
                 strcat(src_final,src);

              //construction de la variable dest_final
                dest_final = malloc(strlen(dest) + 1 + strlen(tsh->repertoire_courant) + 1 + strlen(src) + 1);
                strcpy(dest_final,tsh->repertoire_courant);
                strcat(dest_final,"/");
                strcat(dest_final,dest);
                strcat(dest_final,"/");
                strcat(dest_final,src);

                    if(rename(src_final,dest_final) != 0){
                      printf("rename failed with error");
                    }

                free(src_final);
                free(dest_final);

         }

     free(src);
     free(dest);
     exit(EXIT_SUCCESS);



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
				//Recherche du .tar contenant l'argument
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
					if (liste_argument[i][strlen(liste_argument[i])-1]=='/')
						file_to_find[strlen(file_to_find)] = '\0';
					else
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
