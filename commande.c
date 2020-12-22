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
#include <errno.h>
#include "shell.h"
#include "commande.h"
#include "tar_c.h"
#include "tar_cmd.h"
/*
	Fichier qui gere les tarballs et les commandes associees
*/
/*
Verifie si le fichier est un tarball (fini par ".tar")
Renvoie 1 si le fichier est un Tarball, 0 sinon
*/
int estTarball(char *nom_fichier)
{
	if (strlen(nom_fichier) < 4)
		return 0;
	if(strncmp(&nom_fichier[strlen(nom_fichier)-4],".tar",4)==0)
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
		int index_prec = 0;
		init_chemin_explorer(chemin);
		decoup_fich("chemin,&i");
		while (index_prec < chemin_length)
		{
			char * fichier = calloc(index_chemin_a_explorer - index_prec +1,sizeof(char));
			strncpy(fichier,&chemin_a_explorer[index_prec],index_chemin_a_explorer - index_prec);
			if (fichier[strlen(fichier)-1]=='/')
			{
				fichier[strlen(fichier)-1] = '\0';
			}
			if (estTarball(fichier))
			{
				free(fichier);
				free_chemin_explorer();
				return 1;
			}
			index_prec = index_chemin_a_explorer;
			decoup_fich("chemin,&i");
			free(fichier);
		}
		return 0;
	}
}
/*
Verifie si la commande en argument est une commande fonctionnant dans un tarball
Renvoie 1 si c'est le cas, 0 sinon
*/
int estCommandeTar(char *mot_commande,shell * tsh)
{
	for (int i = 0; i < tsh->nb_cmds;i++)
	{
		if (strcmp(mot_commande,tsh -> cmd_tarballs[i])==0)
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
	for (int i = 1; i < nb_arg_cmd;i++)
	{
		if (liste_argument[i][0] == '-')
		{
			result[j] = malloc(strlen(liste_argument[i]));
			sprintf(result[j],"%s",liste_argument[i]);
			j++;
		}
	}
	//Aucune option
	if (j == 0)
	{
		free(result);
		return NULL;
	}
	for(int h = j ;h < nb_arg_cmd;h++)
	{
		result[j] = NULL;
	}
	return result;
}
/*
Verifie si l'option en argument est presente dans les options valides
Renvoie 1 si l'option est presente, 0 sinon
*/
int est_une_option(char *options,char option)
{
	int i = 1;
	while (options[i] != '\0')
	{
		if (options[i] == option)
		{
			return 1;
		}
		i++;
	}
	return 0;
}
/*
Verifie si les options en argument sont prises en charge pour la commande pour
les tar.
Renvoie le caractere null si toutes les options sont prises en charge, une option
non supportee presente dans la liste en argument sinon
*/
char a_bonnes_options(char *nom_commande,char **options,shell * tsh)
{
	if (options == NULL)
		return '\0';
	//On explore les commandes pour trouver celle qu'on veut
	for (int i = 0; i < tsh->nb_cmds; i++)
	{
		//On trouve la commande
		if (strcmp(nom_commande,tsh->cmd_tarballs[i])==0)
		{
			int j = 0;
			//On verifie pour chaque option si elle est presente
			while(options[j] != NULL)
			{
				int h = 1;
				while(options[j][h] != '\0')
				{
					if (est_une_option(tsh->option[i],options[j][h])==0)
					{
						return options[j][h];
					}
					h++;
				}
				j++;
			}
			return '\0';
		}
	}
	return '\0';
}
/*
Recherche un fichier .tar dans un chemin et retourne l'index de la fin du fichier.tar
dans le chemin
*/

int recherche_fich_tar(char *chemin)
{
	init_chemin_explorer(chemin);
	decoup_fich(chemin);
	int index_prec = 0;
	while (index_prec < chemin_length)
	{
		char * tar = malloc(index_chemin_a_explorer-index_prec+1);
		strncpy(tar,&chemin_a_explorer[index_prec],index_chemin_a_explorer-index_prec);
		tar[index_chemin_a_explorer-index_prec] = '\0';
		if (tar[strlen(tar)-1]=='/')
			tar[strlen(tar)-1]='\0';
		if(estTarball(tar))
		{
			int index = index_chemin_a_explorer;
			free(tar);
			free_chemin_explorer();
			return index;
		}
		index_prec = index_chemin_a_explorer;
		decoup_fich("");
	}
	free_chemin_explorer();
	return -1;
}
/*
Renvoie si le fichier en argument est bien present dans le fichier tar.
1 -> present
0 -> absent
*/
int presentDansTar(char *tar,char *file)
{
	char ** list_file = list_fich(tar);
	if (list_file == NULL)
	{
		return 0;
	}
	//On verifie si le fichier est dans le tar mais n'a pas d'entete a lui
	char *file2 = malloc(strlen(file) + 2);
	if (file[strlen(file)-1] != '/')
	{
		sprintf(file2,"%s/",file);
	}
	else
	{
		sprintf(file2,"%s",file);
	}
	int i = 0;
	while (list_file[i]!=NULL)
	{
		if((strcmp(file,list_file[i])==0) || (strncmp(file2,list_file[i],strlen(file2))==0))
			return 1;
		i++;
	}
	return 0;
}
/*
Affiche l'erreur correspondant a l'invalidite du chemin, à savoir qu'il y a
un fichier inexistant dans ce chemin
*/
void erreur_chemin_non_valide(char * path, char * cmd)
{
	char * error = malloc(strlen(path)+strlen(cmd)+
	strlen("Aucun fichier ou de repertoire de ce nom\n")+3);
	sprintf(error,"%s %s: Aucun fichier ou repertoire de ce nom\n",cmd,path);
	write(STDERR_FILENO,error,strlen(error));
	free(error);

}
/*
Verifie si le chemin en argument est valide, c'est à dire si le fichier existe
pour un chemin sans tar et si le fichier tar le plus "profond" dans lechemin existe.
Renvoie 1 s'il existe, 0 sinon. On renvoie -1 si stat renvoie une erreur autre
que la non existence du fichier
*/
int cheminValide(char *path,char * cmd)
{
	//Si le chemin est dans un tar
	if(contexteTarball(path))
	{
		int index_path = 0;
		init_chemin_explorer(path);
		char * path_bis = malloc(strlen(path)+3);
		memset(path_bis,0,strlen(path)+3);
		decoup_fich("path,&index_path");
		//On part a la recherche des ..
		while (index_path < chemin_length)
		{
			char * fich = malloc(index_chemin_a_explorer - index_path + 1);
			strncpy(fich,&chemin_a_explorer[index_path], index_chemin_a_explorer - index_path);
			fich[index_chemin_a_explorer - index_path] = '\0';
			//On s'arrete des qu'on rencontre ..
			if (strncmp(fich,"..",2)==0)
			{
				free(fich);
				break;
			}
			sprintf(path_bis,"%s%s",path_bis,fich);
			index_path = index_chemin_a_explorer;
			decoup_fich("path,&index_path");
			free(fich);
		}
		//On enleve le '/' pour verifier son existence
		if (path_bis[strlen(path_bis)-1] == '/')
			path_bis[strlen(path_bis)-1] = '\0';
		if (contexteTarball(path_bis))
		{
			int index_tar = recherche_fich_tar(path_bis);
			char * tar = malloc(index_tar);
			strncpy(tar,path_bis,index_tar);
			//tar[index_tar] = '\0';
			if (tar[index_tar-1]=='/')
				tar[index_tar-1] = '\0';
			struct stat st;
			//On verifie si le .tar existe
			if (stat(tar,&st)==-1)
			{
				free_chemin_explorer();
				free(path_bis);
				free(tar);
				return 0;
			}
			//On verifie si la suite du chemin est present dans le tar
			char * file = malloc(strlen(path_bis)-index_tar+1);
			sprintf(file,"%s",&path_bis[index_tar]);
			//Si la suite n'est pas present dans le tar, le chemin n'est pas valide
			if (strcmp(file,"") != 0 && presentDansTar(tar,file)==0)
			{
				free(file);
				free(path_bis);
				free(tar);
				free_chemin_explorer();
				return 0;
			}
			free(file);
		}
		else
		{
			//N'etant pas dans un tar, on appelle directement cheminValide
			int retour = cheminValide(path_bis,cmd);
			if (retour <= 0)
			{
				free_chemin_explorer();
				free(path_bis);
				return retour;
			}
		}
		//On arrive au bout du chemin
		if (index_chemin_a_explorer == chemin_length)
		{
			free_chemin_explorer();
			free(path_bis);
			return 1;
		}
		//Le sous chemin path_bis est donc valide
		index_path += 3;
		sprintf(path_bis,"%s/..",path_bis);
		decoup_fich("");
		while (index_path < chemin_length)
		{
			char * fich = malloc(index_chemin_a_explorer - index_path + 1);
			strncpy(fich,&chemin_a_explorer[index_path], index_chemin_a_explorer - index_path);
			fich[index_chemin_a_explorer - index_path] = '\0';
			if (strncmp(fich,"..",2))
			{
				free(fich);
				break;
			}
			sprintf(path_bis,"%s/..",path_bis);
			index_path = index_chemin_a_explorer;
			decoup_fich("");
			free(fich);
		}
		if (index_path == chemin_length)
		{
			free_chemin_explorer();
			free(path_bis);
			return 1;
		}
		sprintf(path_bis,"%s%s",simplifie_chemin(path_bis),&chemin_a_explorer[index_path]);
		free_chemin_explorer();
		return cheminValide(path_bis,cmd);
	}
	else
	{
		struct stat st;
		if (stat(path,&st)==-1)
		{
			//On verifie la valeur de errno
			if (errno != ENOENT)
			{
				perror("Stat cheminValide");
				return -1;
			}
			return 0;
		}
		return 1;
	}
}
/*
Gere le traitement d'une commande pouvant etre executee sur un tar
*/
int traitement_commandeTar(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	char *nom_commande = malloc(strlen(liste_argument[0])+2);
	sprintf(nom_commande,"%s",liste_argument[0]);
	char *destination = NULL;
	/*On verifie si la commande est cd ou pwd
	Si c'est le cas, on la traite directement, vu que nous avons besoin
	d'au plus un argument */
	if (strcmp(nom_commande,"cd")==0)
	{
		//free(nom_commande);
		cd(liste_argument,nb_arg_cmd,tsh);
		free(nom_commande);
		return 0;
	}
	if (strcmp(nom_commande,"pwd")==0)
	{
		pwd(liste_argument,nb_arg_cmd,tsh);
		free(nom_commande);
		return 0;
	}
	//Sinon on recherche les differentes options
	char **options = recherche_option(liste_argument,nb_arg_cmd);
	int nb_options = 0;
	//Calcul nombre d'options
	if (options)
	{
		while(options[nb_options] != NULL)
			nb_options++;
	}
	/*Si la commande est ls sans argument, on traite directement ls sur le
	repertoire courant*/
	if ((nb_arg_cmd - nb_options == 1) && strcmp(nom_commande,"ls")==0)
	{
		if (contexteTarball(tsh->repertoire_courant))
			ls(tsh->repertoire_courant,options,tsh);
		else
		{
			int fils = fork();
			switch (fils) {
				case -1:
					perror("fork traitement_commandeTar");
					break;
				case 0:
					execvp("ls",liste_argument);
					exit(0);
					break;
				default :
					wait(NULL);
			}

		}
		free(nom_commande);
		for (int j = 0; j < nb_options; j++)
		{
			free(options[j]);
		}
		free(options);
		return 0;
	}
	//Sinon, si la commande n'a pas d'argument, on renvoie une erreur
	if(nb_arg_cmd==1)
	{
		char * error = malloc(strlen(nom_commande)+strlen("Arguments manquants\n")+1);
		sprintf(error,"%s : Arguments manquants\n",nom_commande);
		write(STDERR_FILENO,error,strlen(error));
		free(error);
		free(nom_commande);
		return 1;
	}
	/*Sinon, si la commande est cp ou mv, on verifie si la destination est valide
	ou non*/
	int index_destination = 0;
	if (strcmp(nom_commande,"cp")==0 || strcmp(nom_commande,"mv")==0)
	{
		//On trouve le fichier destination de la commande
		for (int i = nb_arg_cmd - 1; i > 0; i--)
		{
			if(liste_argument[i][0] != '-')
			{
				destination = malloc(strlen(liste_argument[i])+
				strlen(tsh->repertoire_courant)+3);
				index_destination = i;
				sprintf(destination,"%s/%s",tsh->repertoire_courant,liste_argument[i]);
				break;
			}
		}
		//Si on n'en trouve pas, on affiche une erreur
		if (destination==NULL)
		{
			char *error = malloc(strlen(nom_commande)+strlen(" : Aucun argument\n")+1);
			sprintf(error,"%s : Aucun argument\n", nom_commande);
			write(STDERR_FILENO,error,strlen(error));
			free(error);
			free(nom_commande);
			return 0;
		}
		//On verifie si nous avons le bon nombre d'arguments
		if (options==NULL)
		{
			if (nb_arg_cmd < 3)
			{
				char *error = malloc(strlen(nom_commande)+strlen(" : Cible manquante\n")+1);
				sprintf(error,"%s : Cible manquante\n",nom_commande);
				write(STDERR_FILENO,error,strlen(error));
				free(error);
				free(nom_commande);
				return 0;
			}
		}
		else
		{
			if (nb_arg_cmd - nb_options < 3)
			{
					char *error = malloc(strlen(nom_commande)+strlen(" : Cible manquante\n")+1);
					sprintf(error,"%s : Cible manquante\n",nom_commande);
					write(STDERR_FILENO,error,strlen(error));
					free(error);
					free(nom_commande);
					return 0;
			}
		}
		//Verification existence de la destination
		//S'il y a plus de 2 arguments, la cible doit exister avant la commande
		if(nb_arg_cmd - nb_options != 3)
		{
			if (!cheminValide(destination,nom_commande))
			{
				char *error = malloc(strlen(nom_commande)+strlen(" : Cible inexistante\n")
					+ strlen(destination));
				sprintf(error,"%s %s : Cible inexistante\n",nom_commande,destination);
				write(STDERR_FILENO,error,strlen(error));
				free(error);
				free(nom_commande);
				return 0;
			}

		}
	}
	for (int i = 1; i < nb_arg_cmd; i++)
	{
		//Si l'argument est une option, on passe au prochain
		if(liste_argument[i][0] == '-')
			continue;

		char * argument_courant = malloc(strlen(liste_argument[i])+
			strlen(tsh->repertoire_courant)+4);
		sprintf(argument_courant,"%s/%s",tsh->repertoire_courant,liste_argument[i]);

		/*Verification existence et le contexte (dans un tarball ou non) de l'argument courant*/

		//Si la commande n'est pas mv ou cp, on verifie si l'argument est dans un tar
		if (strcmp(nom_commande,"mv") && strcmp(nom_commande,"cp"))
		{
			//Si ce n'est pas le cas, on execute la commande via exec
			if (contexteTarball(argument_courant)==0)
			{
				if (strcmp(nom_commande,"mkdir") != 0 &&
							cheminValide(argument_courant,nom_commande)==0)
				{
					erreur_chemin_non_valide(liste_argument[i],nom_commande);
					free(argument_courant);
					continue;
				}
				//Construction de la liste d'arguments a passer a exec
				char **args = malloc((nb_options + 2)*sizeof(char *));
				args[0] = malloc(strlen(nom_commande)+1);
				sprintf(args[0],"%s",nom_commande);
				//Ajout des options
				for (int j = 1; j <= nb_options; j++)
				{
					args[j] = malloc(strlen(options[j-1])+1);
					sprintf(args[j],"%s",options[j-1]);
				}
				//Ajout de l'argument
				args[nb_options+1] = malloc(strlen(liste_argument[i])+1);
				sprintf(args[nb_options+1],"%s",liste_argument[i]);
				args[nb_options+2] = NULL;
				int fils = fork();
				switch (fils) {
					case -1:
						perror("fork traitement_commandeTar");
						break;
					case 0:
					//Si la commande est ls, on affiche le nom avant si c'est un dossiers
						if (strcmp(nom_commande,"ls")==0)
						{
							struct stat st;
							stat(argument_courant,&st);
							if S_ISDIR(st.st_mode)
							{
								char * name = malloc(strlen(liste_argument[i])+5);
								sprintf(name,"%s :\n",liste_argument[i]);
								write(STDOUT_FILENO,name,strlen(name));
								free(name);
							}
						}
						execvp(nom_commande,args);
						exit(0);
						break;
					default :
						wait(NULL);
						free(args);
						continue;
				}
			}
			//Contexte Tar
			else
			{
				//On verifie l'existence du fichier tar si la commande n'est pas mkdir
				if (strcmp(nom_commande,"mkdir"))
				{
					//On verifie si le chemin est valide
					if (cheminValide(argument_courant,nom_commande)==0)
					{
						//Sinon on renvoie un message d'erreur
						erreur_chemin_non_valide(liste_argument[i],nom_commande);
						free(argument_courant);
						continue;
					}
					//Si c'est le cas, on simplifie le chemin
					sprintf(argument_courant,"%s",simplifie_chemin(argument_courant));
					//On enleve le '/', qui sert a simplifier le chemin
					if (argument_courant[strlen(argument_courant)-1]=='/')
						argument_courant[strlen(argument_courant)-1] = '\0';

					//On verifie si le chemin est encore dans un tar
					if (contexteTarball(argument_courant) == 0)
					{
						//S'il n'est pas dans un tar, on appelle exec
						int fils = fork();
						switch (fils) {
							case -1:
								perror("fork traitement_commande");
								break;
							case 0:
							//Si la commande est ls, on affiche le nom avant si c'est un dossier
								if (strcmp(nom_commande,"ls")==0)
								{
									struct stat st;
									stat(argument_courant,&st);
									if S_ISDIR(st.st_mode)
									{
										char * name = malloc(strlen(liste_argument[i])+5);
										sprintf(name,"%s :\n",liste_argument[i]);
										write(STDOUT_FILENO,name,strlen(name));
										free(name);
									}
								}
								execlp(nom_commande,nom_commande,liste_argument[i],options,NULL);
								exit(0);
								break;
							default :
								wait(NULL);
						}
						continue;
					}
				}
			}
		}

		/*Verification des options */

		//On verifie si les options sont bonnes
		if (nb_options != 0)
		{
			char opt_ok = a_bonnes_options(nom_commande,options,tsh);
			if (opt_ok != '\0')
			{

				char * error = malloc(strlen(nom_commande)+
				strlen(" option non supportee dans les tarballs\n")+4);
				sprintf(error,"%s -%c : option non supportee dans les tarballs\n",nom_commande,opt_ok);
				write(STDERR_FILENO,error,strlen(error));
				free(error);
				continue;
			}
		}

		/*Maintenant que vous avons les bonnes options, traitons les commandes
		sur les tarballs */
		if (strcmp(nom_commande,"mv")==0 || strcmp(nom_commande,"cp")==0)
		{
			//On saute la destination
			if (i == index_destination)
			{
				continue;
			}
			int dest_in_tar = contexteTarball(destination);
			//La destination est en dehors d'un tarball
			if (dest_in_tar == 0)
			{
				if (contexteTarball(argument_courant)==0)
				{
					//On appelle exec
					int fils = fork();
					switch (fils) {
						case -1:
							perror("fork traitement_commande");
							break;
						case 0:
							execlp(nom_commande,nom_commande,liste_argument[i],destination,options,NULL);
							exit(0);
							break;
						default :
							wait(NULL);
							continue;
					}
				}
				else
				{
					if (cheminValide(argument_courant,nom_commande)==0)
					{
						erreur_chemin_non_valide(argument_courant,nom_commande);
						free(argument_courant);
						continue;
					}
					sprintf(argument_courant,"%s",simplifie_chemin(argument_courant));
					if (contexteTarball(argument_courant)==0)
					{
						int fils = fork();
						switch (fils) {
							case -1:
								perror("fork traitement_commande");
								break;
							case 0:
								execlp(nom_commande,nom_commande,argument_courant,destination,options,NULL);
								exit(0);
								break;
							default :
								wait(NULL);
								free(argument_courant);
								continue;
						}
					}
					else
					{
						printf("Tar -> NonTar\n");
					}
				}
			}
			//Destination dans un tar
			else
			{
				printf("->Tar\n");
			}

		}
		if (strcmp(nom_commande,"mkdir")==0)
			mkdir_tar(liste_argument[i],options,tsh);
		if (strcmp(nom_commande,"rm")==0 || strcmp(nom_commande,"rmdir")==0)
		{
				if (options)
				{
					supprimer_fichier(argument_courant,RM_R,tsh);
				}
				else
				{
					if (strcmp(nom_commande,"rm")==0)
					{
						supprimer_fichier(argument_courant,RM,tsh);
					}
					else
					{
						supprimer_fichier(argument_courant,RM_DIR,tsh);
					}
				}
		}
		if (strcmp(nom_commande,"cat")==0)
			cat(liste_argument[i],options,tsh);
		if (strcmp(nom_commande,"ls")==0)
			ls(liste_argument[i],options,tsh);
		free(argument_courant);
	}
	return 0;
}

/*int cp(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	int option = (recherche_option(liste_argument,nb_arg_cmd));
	if (nb_arg_cmd <= 2)
	{
		write(STDERR_FILENO,"cp : Au moins 2 arguments\n",strlen("cp : Au moins 2 arguments\n"));
		return 1;
	}
	//Verification destination cp
	char *destination = malloc(strlen(liste_argument[nb_arg_cmd-1])+strlen(tsh->repertoire_courant)+3);
	strcpy(destination,tsh->repertoire_courant);
	strcat(destination,"/");
	strcat(destination,liste_argument[nb_arg_cmd-1]);
	int src_in_tar = 0;
	if (nb_arg_cmd != 3)
	{

		if (contexteTarball(destination))
		{
			printf("A faire\n");
		}
		else
		{
			struct stat st;
			if (stat(destination,&st)!=-1)
			{
				printf("A faire\n");
			}
			else
			{
				char * error = malloc(strlen(liste_argument[nb_arg_cmd-1])+strlen("cp: la cible n'est pas un dossier\n")+3);
				sprintf(error,"cp: la cible %s n'est pas un dossier\n",liste_argument[nb_arg_cmd-1]);
				write(STDERR_FILENO,error,strlen(error));
				free(destination);
				free(error);
				return 1;
			}
		}
	}
	for(int i = 1; i < nb_arg_cmd - 1;i++)
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

		if(contexteTarball(simple))
		{
			printf("Coucou\n");
		}
		else
		{
			if (dossier==0)
			{
				simple[strlen(simple)-1] = '\0';
			}
			struct stat st;
			if (stat(simple,&st)!=-1)
			{
				if((st.st_mode & S_IFMT) == S_IFDIR)
				{
					printf("Dossier %s\n",simple);
				}
				else
					printf("FIchier\n");
			}
			else
			{
				char * error = malloc(strlen("cp ")+strlen(simple)+2);
				sprintf(error,"cp %s",simple);
				perror(error);
				free(error);
			}
		}
		free(simple);
	}
	return 0;
}*/

int redirection(char **liste_argument, int nb_arg_cmd, shell *tsh, int redirect){
	if (redirect == 1)
	{
		redirection_input(liste_argument, nb_arg_cmd, tsh);
	}
	else if (redirect == 2)
	{
		redirection_error(liste_argument, nb_arg_cmd, tsh);
	}
	return 0;
}

int redirection_input(char **liste_argument, int nb_arg_cmd, shell *tsh)
{
	int in, out;
	char *file = malloc(1024);
	char *file_to = malloc(1024);
	strcat(file, tsh->repertoire_courant);
	strcat(file, "/");
	strcat(file, liste_argument[nb_arg_cmd-1]);
	if(strcmp(liste_argument[1], "/")){

	}else{
		strcat(file_to, tsh->repertoire_courant);
		strcat(file_to, "/");
	}
	out = open(file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);

	if (strcmp(liste_argument[0], "cat") == 0)
	{
		in = open(liste_argument[1], O_RDONLY);
		int fils = fork();
		if (fils == -1)
		{
			perror("fork redirection");
			return 1;
		}
		if (fils == 0)
		{
			dup2(in, 0);
			dup2(out, 1);
			if (contexteTarball(file_to))
			{
				//cat(liste_argument, nb_arg_cmd - 2, tsh);
			}
			else
			{
				execlp(liste_argument[0], liste_argument[0], liste_argument[1], NULL);
			}
			close(out);
			exit(0);
		}
		wait(NULL);
	}else if (strcmp(liste_argument[0], "echo") == 0){
		char *write = malloc(1024);
		for (int i = 1; i < nb_arg_cmd - 2; i++){
			strcat(write, liste_argument[i]);
			strcat(write, " ");
		}
		write[strlen(write) - 1] = '\0';
		int fils = fork();
		if (fils == -1){
			perror("fork redirection");
			return 1;
		}if (fils == 0)
		{
			dup2(out, 1);
			close(out);
			execlp(liste_argument[0], liste_argument[0], write, NULL);
			exit(0);
		}
		wait(NULL);
	}else if (strcmp(liste_argument[0], "ls") == 0){
		int fils = fork();
		if (fils == -1){
			perror("fork redirection");
			return 1;
		}
		if (fils == 0){
			dup2(out, 1);
			close(out);
			execlp(liste_argument[0], liste_argument[0], NULL);
			exit(0);
		}
		wait(NULL);
	}else{
		perror("A faire\n");
	}
	return 1;
}

int redirection_error(char **liste_argument, int nb_arg_cmd, shell *tsh)
{
	int in, out;

	in = open(liste_argument[1], O_RDONLY);

	if (in == -1){
		out = open(liste_argument[nb_arg_cmd - 1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
		int fils = fork();
		if (fils == -1)
		{
			perror("fork redirection");
			return 1;
		}
		if (fils == 0)
		{
			dup2(out, 1);
			close(in);
			close(out);
			printf("%s: %s: No such file or directory\n", liste_argument[0], liste_argument[1]);
			exit(0);
		}
		wait(NULL);
	}else{
		int fils = fork();
		if (fils == -1){
			perror("fork redirection");
			return 1;
		}if (fils == 0)
		{
			close(in);
			execlp(liste_argument[0], liste_argument[0], liste_argument[1], NULL);
			exit(0);
		}
		wait(NULL);

	}
	return 0;
}


#endif
