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
int estCommandeTar(char *mot_commande,shell * tsh)
{
	for (int i = 0; i < 9;i++)
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
			result[j] = malloc(25);
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
Recherche un fichier .tar dans un chemin et retourne l'index de la fin du fichier.tar
dans le chemin
*/
int recherche_fich_tar(char *chemin)
{
	char *tar = malloc(strlen(chemin)+1);
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
Renvoie si le fichier en argument est bien present dans le fichier tar.
1 -> present
0 -> absent
*/
int presentDansTar(char *tar,char *file)
{
	char ** list_file = list_fich(tar);
	//On verifie si le fichier est dans le tar mais n'a pas d'entete a lui
	char *file2 = malloc(strlen(file) + 1);
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
Verifie si le chemin en argument est valide, c'est à dire si le fichier existe
pour un chemin sans tar et si le fichier tar le plus "profond" dans lechemin existe.
Renvoie 1 s'il existe, 0 sinon.
*/
int cheminValide(char *path,char * cmd)
{
	struct stat st;
	char * file = malloc(strlen(path)+1);
	sprintf(file,"%s/",path);
	file[strlen(path)] = '\0';
	//Si le chemin est dans un tar
	if(contexteTarball(path))
	{
		int i = recherche_fich_tar(path);
		strncpy(file,path,i);
		file[i] = '\0';
		if(file[i-1]=='/')
			file[i-1] = '\0';
		int b_f = 0;

		char * lo = decoup_nom_fich(path, &b_f);
		while (lo != NULL)
		{
			while (lo != NULL && strcmp(lo,".."))
			{
				lo = decoup_nom_fich(path,&b_f);
			}
			if (lo == NULL)
				break;
			//Les .. se situent apres le fichier tar
			if(b_f > i)
			{
				//On retire 4 à l'index, pour retrouver le dernier fichier du chemin
				char *maman = malloc((b_f-4) - i + 1);
				strncpy(maman,&path[i],(b_f-4) - i + 1);
				maman[(b_f-4) - i + 1] = '\0';
				//Si le fichier n'est pas present dans le tar, on renvoie 0
				if(presentDansTar(file,maman)==0)
				{
					char *error = malloc(strlen(path)+strlen(cmd)+3 +strlen("Aucun fichier de ce nom\n"));
					sprintf(error,"%s %s : Aucun fichier de ce nom\n",cmd,path);
					write(STDERR_FILENO,error,strlen(error));
					free(maman);
					free(error);
					free(file);
					return 0;
				}
				//Sinon, on verifie la suite du chemin
				while (b_f < strlen(path))
				{
					lo = decoup_nom_fich(path,&b_f);
					if (strcmp(lo,".."))
						break;
				}
				if (b_f == strlen(path))
					return 1;
				char *bat = malloc(strlen(path));
				strncpy(bat,path,b_f);
				sprintf(bat,"%s",simplifie_chemin(bat));
				if (cheminValide(bat,cmd))
				{

				}
			}
			//Les .. se situent avant le fichier tar
			else
			{
				//On verifie si le chemin est valide
				char * new_path = malloc(strlen(path));
				strncpy(new_path,path,b_f);
				if (cheminValide(new_path,cmd))
				{

				}
				else
				{
					char *error = malloc(strlen(path)+strlen(cmd)+3 +strlen("Aucun fichier de ce nom\n"));
					sprintf(error,"%s %s : Aucun fichier de ce nom\n",cmd,path);
					write(STDERR_FILENO,error,strlen(error));
					free(error);
					free(file);
					return 0;
				}
			}


		}
	}
	if (stat(file,&st)==-1)
	{
		char *error = malloc(strlen(path)+strlen(cmd)+3);
		sprintf(error,"%s %s",cmd,path);
		perror(error);
		free(error);
		free(file);
		return 0;
	}
	free(file);
	return 1;
}
int traitement_commandeTar(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	char *nom_commande = malloc(10);
	strcpy(nom_commande,liste_argument[0]);
	char *destination = NULL;
	/*On verifie si la commande est cd ou pwd
	Si c'est le cas, on la traite directement, vu que nous avons besoin
	d'au plus un argument */
	if (strcmp(nom_commande,"cd")==0)
	{
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
	char **options = recherche_option(liste_argument,nb_arg_cmd);
	int nb_options = 0;
	if (options)
	{
		while(options[nb_options] != NULL)
			nb_options++;
	}
	/*Si la commande est ls sans argument, on traite directement ls sur le
	repertoire courant*/
	if ((nb_arg_cmd - nb_options == 1) && strcmp(nom_commande,"ls")==0)
	{
		ls(tsh->repertoire_courant,NULL,tsh);
		return 0;
	}
	if(nb_arg_cmd==1)
	{
		char * error = malloc(strlen(nom_commande)+strlen("Arguments manquants\n"));
		sprintf(error,"%s : Arguments manquants\n",nom_commande);
		write(STDERR_FILENO,error,strlen(error));
		free(error);
		free(nom_commande);
		return 1;
	}
	/*Sinon, si la commande est cp ou mv, on verifie si la destination est valide
	ou non*/
	if (strcmp(nom_commande,"cp")==0 || strcmp(nom_commande,"mv")==0)
	{
		//On trouve le fichier destination de la commande
		for (int i = nb_arg_cmd - 1; i > 0; i--)
		{
			if(liste_argument[i][0] != '-')
			{
				destination = malloc(strlen(liste_argument[i])+
				strlen(tsh->repertoire_courant)+3);
				sprintf(destination,"%s/%s",tsh->repertoire_courant,liste_argument[i]);
				break;
			}
		}
		//Si on n'en trouve pas, on affiche une erreur
		if (destination==NULL)
		{
			char *error = malloc(strlen(nom_commande)+strlen(" : Aucun argument\n"));
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
				char *error = malloc(strlen(nom_commande)+strlen(" : Cible manquante\n"));
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
					char *error = malloc(strlen(nom_commande)+strlen(" : Cible manquante\n"));
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
		if(liste_argument[i][0]=='-')
			continue;
		if (strcmp(nom_commande,"mv")==0)
			mv(liste_argument[i],destination,options,tsh);
		if (strcmp(nom_commande,"cp")==0)
			cp(liste_argument[i],destination,options,tsh);
		if (strcmp(nom_commande,"mkdir")==0)
			mkdir_tar(liste_argument[i],options,tsh);
		if (strcmp(nom_commande,"rm")==0)
			rm(liste_argument[i],options,tsh);
		if (strcmp(nom_commande,"cat")==0)
			cat(liste_argument[i],options,tsh);
		if (strcmp(nom_commande,"rmdir")==0)
			rmdir_tar(liste_argument[i],options,tsh);
		if (strcmp(nom_commande,"ls")==0)
			ls(liste_argument[i],options,tsh);

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
				cat(liste_argument, nb_arg_cmd - 2, tsh);
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
