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
	if (list_file == NULL)
	{
		return 0;
	}
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
		char * path_bis = malloc(strlen(path));
		char * fich = decoup_nom_fich(path,&index_path);
		//On part a la recherche des ..
		while (fich != NULL)
		{
			//On s'arrete des qu'on rencontre ..
			if (strcmp(fich,"..")==0)
				break;
			sprintf(path_bis,"%s%s/",path_bis,fich);
			fich = decoup_nom_fich(path,&index_path);
		}
		//On enleve le '/' pour verifier son existence
		path_bis[strlen(path_bis)-1] = '\0';
		if (contexteTarball(path_bis))
		{
			int index_tar = recherche_fich_tar(path_bis);
			char * tar = malloc(index_tar);
			strncpy(tar,path_bis,index_tar);
			struct stat st;
			//On verifie si le .tar existe
			if (stat(tar,&st)==-1)
			{
				return 0;
			}
			//On verifie si la suite du chemin est present dans le tar
			char * file = malloc(strlen(path_bis)-index_tar);
			sprintf(file,"%s",&path_bis[index_tar]);

			//Si la suite n'est pas present dans le tar, le chemin n'est pas valide
			if (strcmp(file,"") != 0 && presentDansTar(tar,file)==0)
			{
				return 0;
			}
		}
		else
		{
			//N'etant pas dans un tar, on appelle directement cheminValide
			int retour = cheminValide(path_bis,cmd);
			if (retour <= 0)
			{
				return retour;
			}
		}
		//On arrive au bout du chemin
		if (fich == NULL)
		{
			return 1;
		}
		//Le sous chemin path_bis est donc valide
		sprintf(path_bis,"%s/..",path_bis);
		fich = decoup_nom_fich(path,&index_path);
		while (fich != NULL)
		{
			if (strcmp(fich,".."))
				break;
			sprintf(path_bis,"%s/..",path_bis);
			fich = decoup_nom_fich(path,&index_path);
		}
		if (fich == NULL)
			return 1;
		sprintf(path_bis,"%s%s",simplifie_chemin(path_bis),&path[index_path-strlen(fich)]);
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
	char *nom_commande = malloc(strlen(liste_argument[0]));
	sprintf(nom_commande,"%s",liste_argument[0]);
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
		int dest_contexteTar = 0;
	}
	for (int i = 1; i < nb_arg_cmd; i++)
	{
		//Si l'argument est une option, on passe au prochain
		if(liste_argument[i][0] == '-')
			continue;

		char * argument_courant = malloc(strlen(liste_argument[i])+
			strlen(tsh->repertoire_courant)+4);
		sprintf(argument_courant,"%s/%s",tsh->repertoire_courant,liste_argument[i]);
		//Si la commande n'est pas mv ou cp, on verifie si l'argument est dans un tar
		if (strcmp(nom_commande,"mv") && strcmp(nom_commande,"cp"))
		{
			//Si ce n'est pas le cas, on execute la commande via exec
			if (contexteTarball(liste_argument[i])==0)
			{
				if (strcmp(nom_commande,"mkdir") != 0 &&
							cheminValide(liste_argument[i],nom_commande)==0)
				{
					erreur_chemin_non_valide(liste_argument[i],nom_commande);
					continue;
				}
				//Construction de la liste d'arguments a passer a exec
				char **args = malloc((nb_options + 2)*sizeof(char *));
				args[0] = malloc(strlen(nom_commande));
				sprintf(args[0],"%s",nom_commande);
				//Ajout des options
				for (int j = 1; j <= nb_options; j++)
				{
					args[j] = malloc(strlen(options[j-1]));
					sprintf(args[j],"%s",options[j-1]);
				}
				//Ajout de l'argument
				args[nb_options+1] = malloc(strlen(argument_courant));
				sprintf(args[nb_options+1],"%s",argument_courant);
				args[nb_options+2] = NULL;
				int fils = fork();
				switch (fils) {
					case -1:
						perror("fork traitement_commandeTar");
						break;
					case 0:
						//Construction de la liste d'arguments a passer a exec
						execvp(nom_commande,args);
						exit(0);
						break;
					default :
						wait(NULL);
						/*for (int j = 0; j < nb_options + 2; j++)
							free(args[j]);*/
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
						erreur_chemin_non_valide(liste_argument[i],nom_commande);
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
								execlp(nom_commande,nom_commande,argument_courant,options,NULL);
								exit(0);
								break;
							default :
								wait(NULL);
						}
						continue;
					}
					//Sinon on appelle la fonction auxiliaire correspondante
				}
			}
		}
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
