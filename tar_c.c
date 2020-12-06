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
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include "shell.h"
#include "commande.h"
#include "tar_c.h"
#include "tar.h"
/*
Fichier qui fait les actions demandé sur les tar
*/
/*
REnvoie le nom des fichiers contenus dans un tar
Si le tar n'existe pas, on renvoie NULL
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
			break;
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
				if(entete.typeflag == '5')
				{
					char *error = malloc(strlen(file)+strlen("cat  : est un dossier\n")+2);
					sprintf(error,"cat %s : est un dossier\n", file);
					write(STDERR_FILENO,error,strlen(error));
					free(error);
					return 0;
				}
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
			//Cas ou l'on a tape un nom de dossier sans le /
			if((strncmp(file,entete.name,strlen(entete.name)-1)==0) && (entete.typeflag=='5'))
			{
				char *error = malloc(strlen(file)+strlen("cat  : est un dossier\n")+2);
				sprintf(error,"cat %s : est un dossier\n", file);
				write(STDERR_FILENO,error,strlen(error));
				free(error);
				return 0;
			}
			unsigned long taille;
			sscanf(entete.size,"%lo",&taille);
			lseek(fd,((taille + 512 - 1) / 512)*512,SEEK_CUR);


		}

	}
	char *error = malloc(strlen(file)+strlen("cat  : est un dossier\n")+2);
	sprintf(error,"cat : Fichier %s introuvable\n",file);
	write(STDERR_FILENO,error,strlen(error));
	free(error);
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
		write(STDERR_FILENO,"Erreur supprimer_fichier_tar\n",strlen("Erreur supprimer_fichier_tar\n"));
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
						char *error = malloc(strlen(file)+strlen("rm  : est un dossier\n")+1);
						sprintf(error,"rm %s : est un dossier\n",file);
						write(STDERR_FILENO,error,strlen(error));
						free(error);
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
/*
Crée un repertoire au nom de repr dans le fichier tar en argument
REnvoie une erreur si le dossier est deja present dans le fichier tar
*/
int creation_repertoire_tar(char*tar,char*repr)
{
	//On verifie d'abord si on veut creer un repertoire a la racine du tar ou dans l'un des ses sous repertoire
	int t = 0;
	int nb_ss_dossier = 0;
	while(repr[t]!='\0')
	{
		if (repr[t]=='/')
			nb_ss_dossier++;
		t++;

	}
	char **fichiers_tar = list_fich(tar);
	if (fichiers_tar == NULL)
	{
		printf("%s vide ou inexistant\n",tar);
		return 1;
	}
	int index = 0;
	int trouve = 0;
	while (fichiers_tar[index]!=NULL)
	{
		//Recherche dossier dans le fichier tar
		if (strncmp(fichiers_tar[index],repr,strlen(fichiers_tar[index]))==0)
		{
			if (nb_ss_dossier > 1)
			{
				trouve++;
				index++;
				continue;
			}
			char error[strlen(repr)+strlen(tar)+strlen("mkdir  impossible : deja present dans \n") + 6];
			sprintf(error,"mkdir %s impossible : deja present dans %s\n",repr,tar);
			write(STDERR_FILENO,error,strlen(error));
			return 1;
		}
		index++;
	}

	free(fichiers_tar);
	//Si le repertoire a creer est dans un sous repertoire de la racine mais que ce sous repertoire n'existe pas,on renvoie une erreur
	if (nb_ss_dossier > 1 && trouve == 0)
	{
		char error[strlen(repr)+strlen(tar)+strlen("mkdir  impossible :  N'est pas un dossier\n") + 6];
		sprintf(error,"mkdir %s impossible : N'est pas un dossier\n",repr);
		write(STDERR_FILENO,error,strlen(error));
		return 1;
	}
	int fd = open(tar,O_RDONLY);
	if(fd == -1)
	{
		char *error = malloc(strlen(tar)+strlen("Erreur creation_repertoire_tar ")+2);
		sprintf(error,"Erreur creation_repertoire_tar %s",tar);
		perror(error);
		free(error);
		return 0;
	}
	//Création entete du dossier à creer
	struct posix_header hd,hd2;
	memset(&hd,0,sizeof(struct posix_header));
	sprintf(hd.name,"%s",repr);
	sprintf(hd.mode,"0000777");
    hd.typeflag = '5';
	sprintf(hd.mtime,"%011lo",time(NULL));
	sprintf(hd.uid,"%d",getuid());
	sprintf(hd.gid,"%d",getgid());
	sprintf(hd.uname,"%s",getpwuid(getuid())->pw_name);
	sprintf(hd.gname,"%s",getgrgid(getgid())->gr_name);
	sprintf(hd.size,"%011o",0);
    strcpy(hd.magic,"ustar");
	set_checksum(&hd);
	if (!check_checksum(&hd))
		perror("Checksum impossible");
	unsigned int lus,taille = 0;
	int nb_blocs = 0;
	while ((lus = read(fd,&hd2,512))>0)
	{
		if (hd2.name[0] != '\0')
		{
			sscanf(hd2.size,"%o",&taille);
			nb_blocs += 1 + ((taille + 512 - 1) / 512);
			lseek(fd,((taille + 512 - 1) / 512)*512,SEEK_CUR);
		}
		else
		{
			break;
		}
	}
	close(fd);
	//Ajout du repertoire a la fin du tarball
	fd = open(tar,O_WRONLY);
	lseek(fd,nb_blocs*512,SEEK_CUR);
	write(fd,&hd,512);
	close(fd);
	return 0;
}
#endif
