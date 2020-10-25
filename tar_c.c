#ifndef TAR_C_C
#define TAR_C_C
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include "shell.h"
#include "commande.h"
#include "tar_c.h"
#include "tar.h"
/*
Fichier qui fait les actions demandé sur les tar
*/
/*
REnvoie le nom des fichiers contenus dans un tar
*/
char **list_fich(char *tar)
{
	int taille_archive_max = 20;
	int taille_archive = 0;
	char **liste_fichier = malloc(taille_archive_max*sizeof(char*));
	for(int i = 0;i < taille_archive_max;i++)
	{
		liste_fichier[i] = malloc(1024);
	}
	int fd,lus;
	struct posix_header entete;
	fd = open(tar,O_RDONLY);
	if (fd==-1)
	{
		char *error = malloc(1024);
		sprintf(error,"Erreur list_fich %s",tar);
		perror(error);
		free(error);
		return NULL;
	}
	//Tant qu'on peut lire dans le fichier tar, on le fait et on le stocke dans une variable posix_header
	while ((lus=read(fd,&entete,BLOCKSIZE)) > 0)
	{
		//Gestion de la memoire dynamique
		if (taille_archive == taille_archive_max)
		{
			taille_archive_max *= 2;
			liste_fichier = realloc(liste_fichier,taille_archive_max*sizeof(char*));
			for (int f = taille_archive; f < taille_archive_max;f++)
			{
				liste_fichier[f] = malloc(1024);
			}
		}
		//Si nous sommes dans une entete et que nous sommes pas dans un entete de fin de fichier
		if (entete.name[0] != '\0' && check_checksum(&entete))
		{
			//On rajoute le nom dans la liste
			strcpy(liste_fichier[taille_archive],entete.name);
			if (entete.typeflag != '5')
			{
				unsigned long taille;
				sscanf(entete.size,"%lo",&taille);
				lseek(fd,((taille + 512 -1) / 512)*512,SEEK_CUR);
			}
			taille_archive++;
		}
		//Sinon, on verifie pourquoi ce n'est pas le cas
		else
		{
			//Entete de fin de fichier => fin de parcours
			/*if(entete.name[0] == '\0')
			{
				break;
			}*/
			//Bloc qui n'est pas une entete => on continue le parcours
		}
	}
	close(fd);
	for(int i = taille_archive;i < taille_archive_max;i++)
		liste_fichier[i] = NULL;
	return liste_fichier;

}
/*
Affiche le contenu du fichier file et retourne 1 si file est dans tar. Sinon,renvoie
0 et affiche une erreur
*/
int affiche_fichier_tar(char *tar,char*file)
{
	int fd,lus;
	struct posix_header entete;
	fd = open(tar,O_RDONLY);
	if (fd==-1)
	{
		char *error = malloc(1024);
		sprintf(error,"Erreur affiche_fichier_tar %s",tar);
		perror(error);
		free(error);
		return 0;
	}
	while ((lus=read(fd,&entete,BLOCKSIZE)) > 0)
	{

		//Si nous sommes dans une entete et que nous sommes pas dans un entete de fin de fichier
		if (entete.name[0] != '\0' && check_checksum(&entete))
		{
			//On affiche le fichier
			if(strcmp(file,entete.name)==0)
			{
				char buffer[512];
				unsigned long taille;
				sscanf(entete.size,"%lo",&taille);
				int i = 0;
				while (taille > i)
				{
					lus=read(fd,buffer,BLOCKSIZE);
					write(1,buffer,lus);
					i += lus;
				}
				return 1;
			}
			unsigned long taille;
			sscanf(entete.size,"%lo",&taille);
			lseek(fd,((taille + 512 - 1) / 512)*512,SEEK_CUR);


		}

	}
	printf("Fichier %s introuvable\n",file);
	return 0;
}
/*
Supprime le fichier en argument du fichier .tar en argument.
REnvoie 0 en cas d'echec, 1 sinon
*/
int supprimer_fichier_tar(char *tar,char *file,int option)
{
	int fd,fd_copie,lus;
	char *file2 = malloc(strlen(file)+3);
	strcpy(file2,file);
	strcat(file2,"/");
	struct posix_header entete;
	fd = open(tar,O_RDWR);
	//Utilisation d'un fichier auxiliaire
	fd_copie = open(".supprimer_fichier_tar",O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd==-1)
	{
		char *error = malloc(1024);
		sprintf(error,"Erreur supprimer_fichier_tar %s",tar);
		perror(error);
		free(error);
		return 0;
	}
	if (fd_copie == -1)
	{
		printf("Erreur supprimer_fichier_tar\n");
		return 0;
	}
	int trouvee = 0;
	unsigned long taille;
	while ((lus=read(fd,&entete,BLOCKSIZE)) > 0)
	{
		if (trouvee == 0)
		{
			//Si nous sommes dans une entete et que nous sommes pas dans un entete de fin de fichier
			if (entete.name[0] != '\0' && check_checksum(&entete))
			{
				//Si c'est le nom du fichier que l'on veut supprimer, on ne l'écrit pas le fichier auxiliaire
				if(strcmp(file,entete.name)==0||strcmp(file2,entete.name)==0)
				{
					//Option -r absente
					if (entete.typeflag == '5' && !option)
					{
						printf("rm %s : est un dossier\n",file);
						return 0;
					}
					sscanf(entete.size,"%lo",&taille);
					lseek(fd,((taille + 512 - 1) / 512)*512,SEEK_CUR);
					trouvee = 1;
				}
				//Sinon on continue de l'ecrire
				else
				{
					write(fd_copie,&entete,lus);
				}

			}
			else
			{
				write(fd_copie,&entete,lus);
			}
		}
		//On a trouve le fichier a supprimer
		else
		{
			//OPtion -r présente
			if (option)
			{
				//Non ecriture des fichiers du dossier a supprimer
				if (file[strlen(file)-1]=='/')
				{
					if (memmem(entete.name,strlen(file),file,strlen(file)))
					{
						sscanf(entete.size,"%lo",&taille);
						lseek(fd,((taille + 512 - 1) / 512)*512,SEEK_CUR);
					}
					else
					{
						write(fd_copie,&entete,lus);
					}
				}
				else
				{
					if (memmem(entete.name,strlen(file2),file2,strlen(file2)))
					{
						sscanf(entete.size,"%lo",&taille);
						lseek(fd,((taille + 512 - 1) / 512)*512,SEEK_CUR);
					}
					else
						write(fd_copie,&entete,lus);
				}
			}
			else
			{
				write(fd_copie,&entete,lus);
			}
		}

	}
	//Copie du fichier auxiliaire dans le fichier en argument
	close(fd_copie);
	fd_copie = open(".supprimer_fichier_tar",O_RDONLY);
	lseek(fd,0,SEEK_SET);
	while ((lus=read(fd_copie,&entete,BLOCKSIZE)) > 0)
	{
		write(fd,&entete,lus);

	}
	if (!trouvee)
		printf("rm %s : fichier ou dossier introuvable\n",file);
	close(fd);
	close(fd_copie);
	//unlink(".supprimer_fichier_tar");
	return 0;
}
#endif
