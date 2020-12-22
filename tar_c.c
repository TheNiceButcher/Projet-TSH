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
		}
		//Si nous sommes dans une entete et que nous sommes pas dans un entete de fin de fichier
		if (entete.name[0] != '\0' && check_checksum(&entete))
		{
			//On rajoute le nom dans la liste
			liste_fichier[taille_archive] = malloc(strlen(entete.name)+1);
			sprintf(liste_fichier[taille_archive],"%s",entete.name);
			if (entete.typeflag != '5')
			{
				unsigned long taille;
				sscanf(entete.size,"%lo",&taille);
				lseek(fd,((taille + 512 -1) / 512)*512,SEEK_CUR);
			}
			taille_archive++;
		}
		//Sinon, on arrive a la fin du fichier
		else
		{
			break;
		}
	}
	close(fd);
	for(int i = taille_archive;i < taille_archive_max;i++)
		liste_fichier[i] = NULL;
	return liste_fichier;

}
struct posix_header recuperer_entete(char *tar,char *file)
{
	struct posix_header entete;
	memset(&entete,0,BLOCKSIZE);
	int fd;
	fd = open(tar,O_RDONLY);
	while(read(fd,&entete,BLOCKSIZE)>= 0)
	{
		if (strcmp(entete.name,file) == 0)
			return entete;
		unsigned long taille;
		sscanf(entete.size,"%lo",&taille);
		lseek(fd,((taille + 512 -1) / 512)*512,SEEK_CUR);
	}
	memset(&entete,0,BLOCKSIZE);
	return entete;
}
char * from_mode_to_str_ls_l(char *mode)
{
	char * mode_ls = malloc(10);
	int perm[3];
	//On recupere la valeur des permissions
	//permissions proprietaire
	perm[0] = mode[4] - '0';
	//permissions groupe
	perm[1] = mode[5] - '0';
	//permissions autres
	perm[2] = mode[6] - '0';
	for (int i = 0; i < 3; i++)
	{
		//Droit de lecture
		if ((perm[i] - 4) >= 0)
		{
			mode_ls[0 + i*3] = 'r';
			perm[i] -= 4;
		}
		else
			mode_ls[0 + i*3] = '-';

		//Droit d'ecriture
		if ((perm[i] - 2) >= 0)
		{
			mode_ls[1 + i*3] = 'w';
			perm[i] -= 2;
		}
		else
			mode_ls[1 + i*3] = '-';

		//Droit d'execution
		if ((perm[i] - 1) >= 0)
		{
			mode_ls[2 + i*3] = 'x';
		}
		else
			mode_ls[2 + i*3] = '-';
	}
	mode_ls[9] = '\0';
	return mode_ls;
}
char **affichage_ls_l(char ** to_print,char *tar,int nb_files,char **list)
{
	char ** ls_l = malloc(nb_files*sizeof(char *));
	//Calcul du nombre
	int nb_ln[nb_files];
	for (int i = 0; i < nb_files;i++)
	{
		nb_ln[i] = 1;
		struct posix_header entete = recuperer_entete(tar,to_print[i]);
		ls_l[i] = malloc(1024);
		unsigned long taille;
		char * time_fich = malloc(1024);
		time_t date;
		sscanf(entete.size,"%lo",&taille);
		sscanf(entete.mtime,"%011lo",&date);
		//Type de fichier
		char type_file;
		switch (entete.typeflag - '0') {
			//FIchier ordinaire
			case 0:
				type_file = '-';
				break;
			//Lien
			case 1:
				type_file = 'l';
				break;
			case 2:
				type_file = '-';
				break;
			//Caractere special
			case 3:
				type_file = 'c';
				break;
			//Bloc
			case 4:
				type_file = 'b';
				break;
			//Repertoire
			case 5:
				type_file = 'd';
				break;
			//FIFO
			case 6:
				type_file = 'f';
				break;
		}
		//Calcul date
		struct tm * tm_t = gmtime(&date);
		int hour = tm_t->tm_hour;
		int min = tm_t->tm_min;
		int day = tm_t->tm_mday;
		int mois = tm_t->tm_mon;
		char month[5];
		switch(mois)
		{
			case 0:
				strcpy(month,"jan.");
				break;
			case 1:
				strcpy(month,"fev.");
				break;
			case 2:
				strcpy(month,"mar.");
				break;
			case 3:
				strcpy(month,"avr.");
				break;
			case 4:
				strcpy(month,"mai.");
				break;
			case 5:
				strcpy(month,"jui.");
				break;
			case 6:
				strcpy(month,"jul.");
				break;
			case 7:
				strcpy(month,"aou.");
				break;
			case 8:
				strcpy(month,"sep.");
				break;
			case 9:
				strcpy(month,"oct.");
				break;
			case 10:
				strcpy(month,"nov.");
				break;
			case 11:
				strcpy(month,"dec.");
				break;
		}
		if (hour < 10)
			sprintf(time_fich,"%s %d 0%d",month, day, hour);
		else
			sprintf(time_fich,"%s %d %d",month, day, hour);
		if (min < 10)
			sprintf(time_fich,"%s:0%d",time_fich,min);
		else
			sprintf(time_fich,"%s:%d",time_fich,min);
		sprintf(ls_l[i],"%c%s %d %s %s %ld %s %s\n",
		type_file,from_mode_to_str_ls_l(entete.mode),nb_ln[i],entete.uname,entete.gname,taille,time_fich,entete.name);
	}

	return ls_l;

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
	char *error = malloc(strlen(file)+strlen("cat  : Fichier  introuvable\n")+2);
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
	nb_blocs++;
	//On retablit le fait que le tar est constitue de blocs de 20 blocs de 512 octets
	while (nb_blocs % 20 != 0)
	{
		char buffer[BLOCKSIZE];
		memset(buffer,0,BLOCKSIZE);
		write(fd,buffer,BLOCKSIZE);
		nb_blocs++;
	}
	close(fd);
	return 0;
}
#endif
