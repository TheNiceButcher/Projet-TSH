#include "tar_cmd.h"
/*
Effectue la commande ls sur le fichier present dans un tarball
*/
int ls(char *file, char **options,shell *tsh)
{
	char * simplified_file = malloc(strlen(file) + strlen(tsh->repertoire_courant)+3);
	//Si file est le repertoire courant, pas besoin de simplifier le chemin
	if (strcmp(file,tsh->repertoire_courant)==0)
	{
		sprintf(simplified_file,"%s",tsh->repertoire_courant);
		if (simplified_file[strlen(file)-1]=='/')
			simplified_file[strlen(file)-1] = '\0';
	}
	//SInon, on le simplifie
	else
	{
		sprintf(simplified_file,"%s%s",tsh->repertoire_courant,file);
		sprintf(simplified_file,"%s",simplifie_chemin(simplified_file));
	}
	if (simplified_file[strlen(simplified_file)-1] == '/')
	{
		simplified_file[(strlen(simplified_file))-1] = '\0';
	}
	int s = recherche_fich_tar(simplified_file);
	//Tableau qui stocke les noms des fichiers a afficher
	char ** to_print;
	//Son index
	int index_to_print = 0;
	//fichier tar
	char * tar = calloc(strlen(simplified_file)+2,sizeof(char));
	char **list;
	//ls sur un Fichier .tar
	if (s==strlen(simplified_file))
	{
		list = list_fich(simplified_file);
		sprintf(tar,"%s",simplified_file);
		if (list == NULL)
		{
			char *error = malloc(strlen(file)+strlen("ls \n"));
			sprintf(error,"ls %s\n",file);
			write(STDERR_FILENO,error,strlen(error));
			free(error);
			free(simplified_file);
			return 1;
		}
		else
		{
			//Calcul du nombre de fichier dans le tar
			int nb_fich_list = 0;
			while (list[nb_fich_list]!=NULL)
				nb_fich_list++;
			to_print = calloc((nb_fich_list + 1),sizeof(char *));
			int k = 0;
			while (list[k]!=NULL)
			{
				int d = 0;
				//On n'affiche pas les fichiers non presents "a la racine" du tar
				for(;d < index_to_print;d++)
				{
					if (strncmp(to_print[d],list[k],strlen(to_print[d])) == 0)
					{
						break;
					}
				}
				if (d == index_to_print)
				{
					init_chemin_explorer(list[k]);
					decoup_fich("");
					char * mot = malloc(strlen(list[k])+1);
					strncpy(mot,chemin_a_explorer,index_chemin_a_explorer);
					mot[index_chemin_a_explorer] = '\0';

					to_print[index_to_print] = malloc(strlen(mot)+1);
					sprintf(to_print[index_to_print],"%s",mot);
					index_to_print++;
					free(mot);
					free_chemin_explorer();
				}
				k++;
			}
		}
	}
		//ls sur un fichier dans un fichier .tar
	else
	{
		//Recherche fichier .tar contenant le fichier
		int index = recherche_fich_tar(simplified_file);
		strncpy(tar,simplified_file,index);
		tar[index - 1] = '\0';
		list = list_fich(tar);
		char *file_to_find = malloc(strlen(simplified_file)+1);
		strncpy(file_to_find,&simplified_file[index],strlen(simplified_file)-index + 1);
		if (list == NULL)
		{
			char *error = malloc(strlen(tar)+strlen("ls \n"));
			sprintf(error,"ls %s\n",tar);
			perror(error);
			free(error);
			free(tar);
			free(simplified_file);
			return 1;
		}
		else
		{
			int nb_fich_list = 0;
			while (list[nb_fich_list]!=NULL)
				nb_fich_list++;
			//Recherche du fichier dans le fichier .tar
			int k = 0;
			to_print = calloc((nb_fich_list + 1),sizeof(char*));
			//Parcours des fichiers du tar
			while (list[k]!=NULL)
			{
				//On verifie si le fichier courant commence de la meme facon
				if (strncmp(list[k],file_to_find,strlen(file_to_find))==0)
				{
					/*On verifie si le fichier courant est bien le fichier sur lequel on appelle
					ls ou encore s'il appartient bien a ce fichier.
					On fait ca pour eviter de compter un fichier qui commence de la meme
					facon mais qui reste different*/
					if (list[k][strlen(simplified_file) - index] == '\0'||list[k][strlen(simplified_file) - index] == '/')
					{
						//On recupere alors soit un fichier soit un repertoire
						int jpp = strlen(file_to_find) + 1;
						init_chemin_explorer(&list[k][jpp]);
						decoup_fich("");
						int index_path = 0;
						char * fich_to_print = calloc (index_chemin_a_explorer - index_path +4,sizeof(char));
						strncpy(fich_to_print,&chemin_a_explorer[index_path], index_chemin_a_explorer - index_path);
						fich_to_print[index_chemin_a_explorer-index_path + 1] = '\0';
						int d = 0;
						//On verifie si le fichier est deja present dans la liste des elements a afficher
						for (; d < index_to_print;d++)
						{
							if (index_chemin_a_explorer==index_path) continue;
							if (strcmp(fich_to_print,to_print[d])==0)
								break;
						}
						//On est donc sur que le fichier n'est pas deja dans la iste
						if (d == index_to_print)
						{
							//On n'affiche pas le nom du dossier sur lequel on appelle ls
							if (strncmp(list[k],file_to_find,strlen(file_to_find))==0 && list[k][strlen(list[k])-1] == '/')
							{
								//Dossier
								if(index_chemin_a_explorer==index_path)
								{
									k++;
									free(fich_to_print);
									free_chemin_explorer();
									continue;
								}
							}
							//ls sur un fichier != repertoire
							if (index_chemin_a_explorer==index_path)
							{
								sprintf(fich_to_print,"%s",file_to_find);
							}
							to_print[index_to_print] = malloc(strlen(fich_to_print)+2);

							sprintf(to_print[index_to_print],"%s",fich_to_print);

							index_to_print++;
						}
						free(fich_to_print);
						free_chemin_explorer();
					}
				}
				k++;
			}
		}

	}
	//Option -l presente
	if (options)
	{
		char **list_ls = affichage_ls_l(to_print,simplified_file,index_to_print,list);
		//Si ls est sur un seul fichier, on modifie la ligne avant de l'afficher pour y mettre le chemin
		int index_tar = strlen(tar)+1;
		//S'il y a un seul fichier et qu'il a le meme nom, on a donc appele ls sur ce fichier
		if (index_to_print == 1 && strcmp(&simplified_file[index_tar],to_print[0])==0)
		{
		 char * true_line = malloc(strlen(list_ls[0])+strlen(file)+3);
		 sprintf(true_line, "%s", list_ls[0]);
		 int i = strlen(list_ls[0]) - 1;
		 while (true_line[i] != ' ')
		 {
			 i--;
		 }
		 i++;
		 strcpy(&true_line[i],file);
		 strcat(true_line,"\n");
		 write(STDOUT_FILENO,true_line,strlen(true_line));
		 free(true_line);
		}
		else
		{
			for (int i = 0; i < index_to_print; i++)
			{
				write(STDOUT_FILENO,list_ls[i],strlen(list_ls[i]));
			}
			for(int i = 0; i < index_to_print; i++)
			{
				free(list_ls[i]);
			}
			free(list_ls);
		}
	}
	//Sans option -l
	else
	{
		//ls sur un fichier ordinaire
		if (index_to_print == 1)
		{
			if (to_print[0][strlen(to_print[0])-1] != '/')
			{
				char *full_name = malloc(strlen(file)+3);
				sprintf(full_name,"%s\n",file);
				write(STDOUT_FILENO,full_name,strlen(full_name)+1);
				free(full_name);
			}
		}
		//ls sur un repertoire dans un tar
		write(STDOUT_FILENO,file,strlen(file));
		write(STDOUT_FILENO,":\n\n",strlen(":\n\n")+1);
		for (int i = 0; i < index_to_print; i++)
		{
			char * full_name = malloc(strlen(to_print[i]) + 4);
			sprintf(full_name,"%s\n", to_print[i]);
			write(STDOUT_FILENO,full_name,strlen(full_name));
			free(full_name);
		}
	}
	for (int i = 0; i < index_to_print; i++)
		free(to_print[i]);
	free(to_print);
	return 0;
}
/*

*/
int cd(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	if (nb_arg_cmd == 2)
	{
		char * nv_repr_courant = malloc(strlen(liste_argument[1])+
		strlen(tsh->repertoire_courant)+4);
		//Prise en charge des chemins absolus depuis la racine
		if (liste_argument[1][0] == '/')
		{
			strcpy(nv_repr_courant,liste_argument[1]);
		}
		else
		{
			sprintf(nv_repr_courant,"%s",tsh->repertoire_courant);
			if(tsh->repertoire_courant[strlen(tsh->repertoire_courant)-1] != '/')
				strcat(nv_repr_courant,"/");
			strcat(nv_repr_courant,liste_argument[1]);
		}
		//Ajout du / au chemin s'il n'en y a pas
		if (nv_repr_courant[strlen(nv_repr_courant)-1] != '/')
		{
			strcat(nv_repr_courant,"/");
			nv_repr_courant[strlen(nv_repr_courant)] = '\0';
		}
		//On verifie si le chemin est valide (c-a-d si le repertoire existe)
		int valide = cheminValide(nv_repr_courant,"cd");
		if (valide==0)
		{
			erreur_chemin_non_valide(liste_argument[1],"cd");
			free(nv_repr_courant);
			return 0;
		}
		else
		{
			//L'argument n'est pas repertoire -> on affiche une erreur
			if (valide == -1 && errno == ENOTDIR)
			{
				char *error = malloc(strlen(liste_argument[1])+30);
				sprintf(error,"cd %s : n'est pas un dossier\n",liste_argument[1]);
				write(STDERR_FILENO,error,strlen(error));
				free(error);
				free(nv_repr_courant);
				return 1;
			}
			sprintf(nv_repr_courant,"%s",simplifie_chemin(nv_repr_courant));
			//Apres la simplication, il faut bien verifier si le nouveau repertoire est dans un tarball ou non
			if (contexteTarball(nv_repr_courant))
			{
				int index_tar = recherche_fich_tar(nv_repr_courant);
				char *tar = calloc(strlen(nv_repr_courant)+1,sizeof(char));
				strncpy(tar,nv_repr_courant,index_tar);
				//
				if (tar[strlen(tar)-1]=='/')
				{
					tar[strlen(tar)-1] = '\0';
				}
				char *file = malloc(strlen(nv_repr_courant)+5);
				sprintf(file,"%s",&nv_repr_courant[index_tar]);
				//
				if (file[strlen(file)-1]=='/')
				{
					file[strlen(file)-1] = '\0';
				}
				if (index_tar==strlen(nv_repr_courant) || estRepertoire(file,tar)==1)
				{
					sprintf(tsh->repertoire_courant,"%s",nv_repr_courant);
					tsh->tarball = 1;
					free(nv_repr_courant);
					return 1;
				}
				else
				{
					char *error = malloc(strlen(liste_argument[1])+30);
					sprintf(error,"cd %s : n'est pas un dossier\n",liste_argument[1]);
					write(STDERR_FILENO,error,strlen(error));
					free(error);
					free(nv_repr_courant);
					return 1;
				}
			}
			else
			{
				if (chdir(nv_repr_courant)==-1)
				{
					char error[strlen(liste_argument[1])+strlen("cd impossible") + 3];
					sprintf(error,"cd %s impossible",liste_argument[1]);
					perror(error);
					free(nv_repr_courant);
				}
				else
				{
					strcpy(nv_repr_courant,simplifie_chemin(nv_repr_courant));
					strcpy(tsh->repertoire_courant,nv_repr_courant);
					tsh->tarball = 0;
					free(nv_repr_courant);
				}
			}
		}
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
	char * repr = malloc(strlen(tsh->repertoire_courant)+ 2);
	sprintf(repr, "%s\n",tsh->repertoire_courant);
	write(STDOUT_FILENO,repr,strlen(repr));
	free(repr);
	return 0;
}
int cp(char *file,char * destination,char ** options,shell *tsh)
{
	//on a pas a verifier si file et destination existe deja car on la deja verifié a traitement_commandeTar
	//par contre on fais le test s'il sont des .tar ou des fichier dans des .tar
	if (contexteTarball(file))
	{
		int index = recherche_fich_tar(file);
		//Fichier source est un .tar
		if (index == strlen(file))
		{
			printf("en contruction");
		}
		//Fichier source est dans un .tar
		else
		{
			if(contexteTarball(destination))
			{
				int index_d = recherche_fich_tar(destination);
				//fichier destination est un .tar
				if(index_d == strlen(destination))
				{
					char *tar_file = malloc(strlen(file));
					strncpy(tar_file,file,index);
					char *file_to_cp = malloc(strlen(file));
					strcpy(file_to_cp,&file[index]);
					int fd_tar_file = open(tar_file,O_RDWR);
					int fd_dest = open(destination,O_RDWR);
					struct posix_header entete;
					struct posix_header header;
					unsigned int lus,taille = 0;
					int nb_blocs = 0;
					//cherche le fichier source en le fichier .tar puis construire une entete identique a son entete
					while ((lus = read(fd_tar_file,&entete,512))>0)
					{
						if (entete.name[0] != '\0')
						{
							//si on trouve le fichier voulu
							if (strcmp(entete.name,file_to_cp) == 0)
							{
								struct posix_header entete2;
								memset(&entete2,0,sizeof(struct posix_header));

								//remplire l'entete du fichier source
								memcpy(&entete2,&entete,BLOCKSIZE);
								set_checksum(&entete2);
								if (!check_checksum(&entete2))
									perror("Checksum impossible");

								//compter le nombre de block dans le .tar en destinaton
								int nb_blocs_dest=0;
								while ((read(fd_dest,&header,512))>0)
								{
									if (header.name[0] != '\0')
									{
										sscanf(header.size,"%o",&taille);
										nb_blocs_dest += 1 + ((taille + 512 - 1) / 512);
										lseek(fd_dest,((taille + 512 - 1) / 512)*512,SEEK_CUR);
									}
									else
									{
										break;
									}
								}


								//copier le contenue dans la nouvelle entete puis faire un write dans le fichier en destination
								write(fd_dest,&entete2,512);
								nb_blocs_dest += 1;
								int reads=0;
								while ((lus=read(fd_tar_file,&entete2,BLOCKSIZE)) > 0)
								{
									reads+=lus;
									if(reads <= taille)
									{
										lseek(fd_dest,nb_blocs_dest*512,SEEK_CUR);
										write(fd_dest,&entete2,lus);
										nb_blocs_dest += 1 ;
									}
									else
									{
										break;
									}
								}
								close(fd_dest);
								close(fd_tar_file);
								return 0;
							}
							else
							{
								sscanf(entete.size,"%o",&taille);
								nb_blocs += 1 + ((taille + 512 - 1) / 512);
								lseek(fd_tar_file,((taille + 512 - 1) / 512)*512,SEEK_CUR);
							}

						}
					}
					close(fd_dest);
					close(fd_tar_file);

				}
				//fichier destination est dans un .tar
				else
				{
					char *tar_file = malloc(strlen(file));
					strncpy(tar_file,file,index);
					char *file_to_cp = malloc(strlen(file));
					strcpy(file_to_cp,&file[index]);
					int fd_tar_file = open(tar_file,O_RDWR);
					char *tar_dest = malloc(strlen(destination));
					strncpy(tar_dest,destination,index_d);
					int fd_dest = open(tar_dest,O_RDWR);
					struct posix_header entete;
					struct posix_header header;
					unsigned int lus,taille = 0;
					int nb_blocs = 0;
					//cherche le fichier source en le fichier .tar puis remplire son entete
					while ((lus = read(fd_tar_file,&entete,512))>0)
					{

						if (entete.name[0] != '\0')
						{

							//si on trouve le fichier voulu
							if (strcmp(entete.name,file_to_cp) == 0)
							{
								struct posix_header entete2;
								memset(&entete2,0,BLOCKSIZE);
								memcpy(&entete2,&entete,BLOCKSIZE);
								if (!check_checksum(&entete2))
									perror("Checksum impossible");

								//chercher le block du ficher destinaton dans le .tar
								int nb_blocs_dest=0;
								while ((read(fd_dest,&header,512))>0)
								{
									if (header.name[0] != '\0')
									{
										if (strcmp(header.name,destination) != 0)
										{
											sscanf(header.size,"%o",&taille);
											nb_blocs_dest += 1 + ((taille + 512 - 1) / 512);
											lseek(fd_dest,((taille + 512 - 1) / 512)*512,SEEK_CUR);
										}
										else
										{
											sscanf(header.size,"%o",&taille);
											nb_blocs_dest += 1 + ((taille + 512 - 1) / 512);
											lseek(fd_dest,((taille + 512 - 1) / 512)*512,SEEK_CUR);

											write(fd_dest,&entete2,512);
											nb_blocs_dest += 1;
											int reads=0;
											while ((lus=read(fd_tar_file,&entete2,BLOCKSIZE)) > 0)
											{
												reads=+lus;
												if(reads <= taille)
												{
													lseek(fd_dest,nb_blocs_dest*512,SEEK_CUR);
													write(fd_dest,&entete2,lus);
													nb_blocs_dest += 1 ;
												}
												else
												{
													break;
												}

											}
											//header.size+=entete2.size+512;
											header.typeflag= '5'    ;
											close(fd_dest);
											close(fd_tar_file);
											return 0;
										}
									}
									else
									{
										break;
									}
								}
							}
							else
							{
								sscanf(entete.size,"%o",&taille);
								nb_blocs += 1 + ((taille + 512 - 1) / 512);
								lseek(fd_tar_file,((taille + 512 - 1) / 512)*512,SEEK_CUR);
							}
						}

					}
					close(fd_dest);
					close(fd_tar_file);
					return 0;
				}
			}
			//fichier destination n'est pas un contexte tar
			else
			{
				char *tar_file = malloc(strlen(file));
				strncpy(tar_file,file,index);
				char *file_to_cp = malloc(strlen(file));
				strcpy(file_to_cp,&file[index]);
				int fd_tar_file = open(tar_file,O_RDWR);
				int fd_dest = open(destination,O_RDWR | O_APPEND);
				struct posix_header entete;
				struct posix_header header;
				int nb_blocs = 0;
				unsigned int lus,taille = 0;
				//cherche le fichier source dans le fichier .tar
				while ((lus = read(fd_tar_file,&entete,512))>0)
				{
					if (entete.name[0] != '\0')
					{
						//si on trouve le fichier voulu
						if (strcmp(entete.name,file_to_cp)==0)
						{
							sscanf(entete.size,"%o",&taille);
							nb_blocs += 1 + ((taille + 512 - 1) / 512);
							lseek(fd_tar_file,((taille + 512 - 1) / 512)*512,SEEK_CUR);
							int reads=0;
							while(reads<=taille)
							{
								(lus=read(fd_tar_file,&entete,512));
								write(fd_dest,&entete,lus);
								reads+=lus;
							}
							close(fd_dest);
							close(fd_tar_file);
							return 0;
						}
						else
						{
							sscanf(entete.size,"%o",&taille);
							nb_blocs += 1 + ((taille + 512 - 1) / 512);
							lseek(fd_tar_file,((taille + 512 - 1) / 512)*512,SEEK_CUR);
						}
					}
				}
				close(fd_dest);
				close(fd_tar_file);
				return 0;
			}
		}
	}
	//ficher source n'est pas un context tarball
	else
	{
		if(contexteTarball(destination))
		{
			int index_d = recherche_fich_tar(destination);
			//fichier destination est un .tar
			if(index_d == strlen(destination))
			{
				int fd_file = open(file,O_RDONLY);
				struct posix_header entete;
				char *buf_file = malloc(sizeof(struct stat));
				stat(fd_file, buf_file);
				memset(&entete,0,sizeof(struct posix_header));
				//construire le header
				sprintf(entete.name,"%s",file);
				sprintf(entete.mode,buf_file.st_mode);
				//entete.typeflag = mystat.st_mode & ~S_IFMT;
				switch (buf_file.st_mode & S_IFMT)
				{
					case S_IFBLK:  entete.typeflag=S_IFBLK    ;   break;
					case S_IFCHR:  entete.typeflag=S_IFCHR    ;   break;
					case S_IFDIR:  entete.typeflag=S_IFDIR    ;   break;
					case S_IFIFO:  entete.typeflag=S_IFIFO    ;   break;
					case S_IFLNK:  entete.typeflag=S_IFLNK    ;   break;
					case S_IFREG:  entete.typeflag=S_IFREG    ;   break;
					case S_IFSOCK: entete.typeflag=S_IFSOCK   ;   break;
					default:        break;
				}


				sprintf(entete.mtime,"%011lo",time(NULL));
				sprintf(entete.uid,"%d",buf_file.st_uid);
				sprintf(entete.gid,"%d",buf_file.st_gid);
				sprintf(entete.uname,"%s",getpwuid(buf_file.st_gid)->pw_name);
				sprintf(entete.gname,"%s",getgrgid(buf_file.st_gid)->gr_name);
				sprintf(entete.size,"%011o",buf_file.st_size);
				strcpy(entete.magic,"ustar");
				set_checksum(&entete);
				if (!check_checksum(&entete))
					perror("Checksum impossible");
				fd_dest = open(destination,O_RDWR);

				//compter le nombre de block dans le .tar en destinato
				int nb_blocs_dest=0;
				while ((read(fd_dest,&header,512))>0)
				{
					if (header.name[0] != '\0')
					{
						sscanf(header.size,"%o",&taille);
						nb_blocs_dest += 1 + ((taille + 512 - 1) / 512);
	                 	lseek(fd_dest,((taille + 512 - 1) / 512)*512,SEEK_CUR);
					}
					else
					{
						break;
					}
				}
				//copier le contenue dans la nouvelle entete puis faire un write dans le fichier en destination
				write(fd_dest,&entete,512);
				nb_blocs_dest += 1;
				int reads=0;
				sscanf(entete.size,"%o",&taille);
				while ((lus=read(fd_file,&entete,BLOCKSIZE)) > 0)
				{
					reads+=lus;
					if(reads <= taille)
					{
						lseek(fd_dest,nb_blocs_dest*512,SEEK_CUR);
						write(fd_dest,&entete,lus);
						nb_blocs_dest += 1 ;
					}
					else
					{
						break;
					}
				}
				close(fd_dest);
				close(fd_tar_file);
				return 0;



			}
			//fichier destination est dans un tar
			else
			{
				int fd_file = open(file,O_RDONLY);
				char *tar_dest = malloc(strlen(destination));
				char *dest=malloc(strlen(destination));
				strncpy(tar_dest,destination,index_d);
				int fd_dest = open(tar_dest,O_RDWR);
				struct posix_header entete;
				struct posix_header entete2;
				//chercher le block du ficher destinaton dans le .tar
				int nb_blocs_dest=0;
				while ((read(fd_dest,&enteter,512))>0)
				{
					if (entete.name[0] != '\0')
					{
						if (entete.name[0] != dest)
						{
							sscanf(entete.size,"%o",&taille);
							nb_blocs_dest += 1 + ((taille + 512 - 1) / 512);
							lseek(fd_dest,((taille + 512 - 1) / 512)*512,SEEK_CUR);
						}
						else
						{
							char *buf_file = malloc(sizeof(struct stat));
							stat(fd_file, buf_file);
							sscanf(buf_file.st_size,"%o",&taille);
							nb_blocs_dest += 1 + ((taille + 512 - 1) / 512);
							lseek(fd_dest,((taille + 512 - 1) / 512)*512,SEEK_CUR);
							int reads=0;
							while ((lus=read(fd_file,&entete2,BLOCKSIZE)) > 0)
							{
								reads=+lus;
								if(reads <= taille)
								{
									lseek(fd_dest,nb_blocs_dest*512,SEEK_CUR);
									write(fd_dest,&entete2,lus);
									nb_blocs_dest += 1 ;
								}
								else
								{
									break;
								}
							}
							//   entete.size+=buf_file.st_size;
							entete.typeflag=S_IFDIR    ;
							close(fd_dest);
							close(fd_file);
							return 0;
						}
					}
					else
					{
						break;
					}
				}
			}

		}


	}
	printf("cp en construction\n");
	return 0;
}
int supprimer_fichier(char *file, int option, shell *tsh)
{

	int index = recherche_fich_tar(file);
	char *tar = malloc(strlen(file));
	strncpy(tar,file,index);
	//On enleve le / du tar
	if (tar[index-1]=='/')
	{
		tar[index-1] = '\0';
	}
	//Si on doit supprimer un fichier  .tar
	if (index == strlen(file))
	{
		//Considerant les .tar comme des dossiers, on attend l'option pour le supprimer
		if (option == RM_R)
		{
			file[strlen(file)] = '\0';
			if(unlink(file)==-1)
			{
				char *error = malloc(strlen(file)+strlen("rm  :"));
				sprintf(error,"rm %s :",file);
				perror(error);
				free(error);
			}
		}
		else
		{
			if (option == RM_DIR)
			{
				char ** list = list_fich(tar);
				if (list[0]==NULL)
				{
					supprimer_fichier(file, RM_R,tsh);
				}
				else
				{
					char *error= malloc(strlen("rmdir  : Tar non vide\n")
					+ strlen(file)+1);
					sprintf(error,"rmdir %s: Tar non vide\n",file);
					write(STDERR_FILENO,error,strlen(error));
					free(error);
				}
			}
			else
			{

				char *error= malloc(strlen("rm  : Veuillez utiliser l'option -r pour supprimer les .tar\n")
				+ strlen(file)+1);
				sprintf(error,"rm %s: Veuillez utiliser l'option -r pour supprimer les .tar\n",file);
				write(STDERR_FILENO,error,strlen(error));
				free(error);
			}
		}
	}
		//Suppression dans un .tar
	else
	{
		char *file_to_rm = malloc(strlen(file)-index + 3);
		strcpy(file_to_rm,&file[index]);
		if(tar[strlen(tar)-1]=='/')
			tar[strlen(tar)-1] = '\0';
		supprimer_fichier_tar(tar,file_to_rm,option);
		free(file_to_rm);
	}
	free(tar);
	return 0;
}
int mkdir_tar(char *file, char **options,shell *tsh)
{
	char * fichier= malloc(strlen(file)+3+strlen(tsh->repertoire_courant));
	sprintf(fichier,"%s%s",tsh->repertoire_courant,file);
	if (file[strlen(file)-1]!='/')
	{
		strcat(fichier,"/");
		file[strlen(file)-1] = '\0';
	}
	if (cheminValide(fichier,"mkdir")==1)
	{
		char error[strlen(file)+strlen("mkdir  impossible : deja existant\n") + 6];
		sprintf(error,"mkdir %s impossible : deja existant\n",file);
		write(STDERR_FILENO,error,strlen(error));
		free(fichier);
		return 1;
	}
	int index1 = strlen(fichier) - 1;
	if (fichier[index1] == '/')
	{
		index1--;
	}
	while (fichier[index1] != '/')
	{
		index1--;
	}
	index1++;
	char * name_repr = malloc(strlen(fichier));
	sprintf(name_repr,"%s",&fichier[index1]);
	//Dossier parent du repertoire que l'on veut creer
	char *parent = malloc(strlen(fichier)+1);
	strncpy(parent,fichier,index1);
	parent[index1-1] = '\0';
	if (cheminValide(parent,"mkdir")==0)
	{
		erreur_chemin_non_valide(file,"mkdir");
		free(parent);
		free(name_repr);
		free(fichier);
		return 1;
	}
	sprintf(fichier,"%s",simplifie_chemin(fichier));
	int index = recherche_fich_tar(fichier);
	//-1 signifie que le chemin ne contient pas de tarball
	if (index == -1)
	{
		if (mkdir(fichier, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
		{
			char * error = malloc(strlen(file) + 20);
			sprintf(error,"mkdir %s:",file);
			perror(error);
			free(error);
			free(fichier);
			free(parent);
			free(name_repr);
			return 1;
		}
		return 0;
	}
	//Creation d'un tarball
	if (index == strlen(fichier))
	{
		printf("Création de .tar %s\n",fichier);
		fichier[index-1] = '\0';
		int fd = creat(fichier, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
		if (fd == -1)
		{
			char * error = malloc(strlen(file) + 15);
			sprintf(error,"mkdir %s", file);
			perror(error);
			free(error);
			return 1;
		}
		char buffer[512];
		memset(buffer,0,512);
		for(int i = 0; i < 20; i++)
		{
			read(fd,buffer,512);
		}
		close(fd);
	}
	else
	{
		char *tar = malloc(strlen(fichier));
		strncpy(tar,fichier,index);
		tar[index-1] = '\0';
		char *repr_to_create = malloc (strlen(fichier)-index+3);
		strncpy(repr_to_create,&fichier[index],strlen(fichier)-index+1);
		repr_to_create[strlen(fichier)-index+1] = '\0';
		if (repr_to_create[strlen(repr_to_create)-1] != '/')
		{
			strcat(repr_to_create,"/");
			repr_to_create[strlen(fichier)-index] = '\0';
		}
		char * parent = malloc(strlen(file)+1);
		sprintf(parent,"%s",repr_to_create);
		creation_repertoire_tar(tar,repr_to_create);
		free(fichier);
		free(tar);
		free(repr_to_create);

	}
	return 0;
}
int mv(char *file,char *destination,char **options,shell *tsh)
{
	   /* struct stat stat_src;
	     char *src_final, *dest_final;
	    if (contexteTarball(simple_src)){
	    	   int index = recherche_fich_tar(simple_src);
			//Fichier source est un .tar
				if (index == strlen(simple_src))
				{
				    	//Fichier destination un contexte tar
				    if (contexteTarball(simple_dest)){

				         int index1 = recherche_fich_tar(simple_dest);
	 	               	//Fichie destination est un  .tar
			           	if (index1 == strlen(simple_dest))
			           	{
			           	    printf("en construction\n");
		                     return 0;
			           	}
			           	//Fichie destination est dans un fichier tar
			           	else {
			           	    printf("en construction\n");
		                     return 0;
			           	}

				    }
				    	//Fichie destination est un fichier simple
				    else{
				        printf("en construction\n");
		                 return 0;
				    }

				}
				//fichier source est dans un fichier .tar
				else
				{
				     if (contexteTarball(simple_dest)){

				         int index2 = recherche_fich_tar(simple_dest);
	 	               	//Fichie destination est un  .tar
			           	if (index2 == strlen(simple_dest))
			           	{
			           	    	char *tar_file = malloc(strlen(simple_src));
				               	strncpy(tar_file,simple2,index);
			           	    	char *file_to_mv = malloc(strlen(simple_src));
					            strcpy(file_to_mv,&simple_src[index]);
					            fd_tar = open(tar_file,O_RDWR);
					            fd_dest = open(simple_dest,O_RDWR);
					            struct posix_header entete;
					            unsigned int lus,taille = 0;
		                        int nb_blocs = 0;
		                        //cherche le fichier source en le fichier .tar puis remplire son entete
		                        while ((lus = read(fd_tar,&entete,512))>0)
		                          {
			                        if (entete.name[0] != '\0')
			                           {
			                               //si on trouve le fichier voulu
			                               if (entete.name[0] ==file_to_mv){
			                                    struct posix_header entete2;
			                                    memset(&entete2,0,sizeof(struct posix_header));

			                              //remplire l'entete du fichier source
			                                    entete2.name=entete.name;
			                                    entete2.mode=entete.mode;
			                                    entete2.typeflag=entete.typeflag;
			                                    entete2.mtime=entete.mtime;
			                                    entete2.uid=entete.uid;
			                                    entete2.gid=entete.gid;
			                                    entete2.uname=entete.uname;
			                                    entete2.gname=entete.gname;
			                                    entete2.size=entete.size;
			                                    entete2.magic=entete.magic;
			                                    set_checksum(&hd);

	                                           if (!check_checksum(&hd))
		                             	          perror("Checksum impossible");
		                             	          nb_blocs=0;
		                             	          //compter le nombre de block dans le .tar en destinaton
		                             	             while ((read(fd_dest,&header,512))>0)
		                                              {
		                                                     	if (header.name[0] != '\0')
			                                                        {
			                                                        	sscanf(header.size,"%o",&taille);
			                                                         	nb_blocs += 1 + ((taille + 512 - 1) / 512);
		                                                          	}
		                         	                         else
		                            	                         {
		                                                    		break;
		                             	                          }
	                       	                        }


			                                        fd_mv= open(file_to_mv,O_RDWR);

			                                    	while (read(fd_mv,&entete,BLOCKSIZE) > 0)
	                                                     {
	                                                             lseek(fd_dest,nb_blocs*512,SEEK_CUR);
		                                                         write(fd_dest,&entete,lus);

		                                                   }
		                                                   close(fd_dest);
		                                                   close(fd_mv);
		                                                   close(fd_tar);

			                                    return 0;
			                               }
			                               else{
				                            sscanf(entete.size,"%o",&taille);
				                            nb_blocs += 1 + ((taille + 512 - 1) / 512);
			                              	lseek(fd,((taille + 512 - 1) / 512)*512,SEEK_CUR);
			                               }
			                           }

			                       else
			                          {
				                            break;
			                           }
		                            }

		                     return 0;



			           	}
			           	//Fichie destination est dans un fichier tar
			           	else {
			           	    printf("en construction\n");
		                     return 0;
			           	}

				    }
				    	//Fichie destination est un fichier simple
				    else{
				                char *tar_file = malloc(strlen(simple_src));
				               	strncpy(tar_file,simple2,index);
			           	    	char *file_to_mv = malloc(strlen(simple_src));
					            strcpy(file_to_mv,&simple_src[index]);




				        printf("en construction\n");
		                 return 0;
				    }

				}


	    	}
	    	else if(contexteTarball(simple_dest)){


			 int index = recherche_fich_tar(simple_dest);
			//Fichier .tar
				if (index == strlen(simple_dest))
				{
				      int fd,lus,l,nb_blocs;
		              struct posix_header entete;
		              struct posix_header header;
		              fd_src = open(src,O_RDONLY);
		              fd_dest = open(dest,O_CREAT| O_RDONLY | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
		                if (fd_src==-1)
		                  {
		                 	return 0;
		                  }
		                  if (fd_dest==-1)
		                  {
		                 	return 0;
		                  }

		                //on compte le nombre de bloc dans le fichier dest
		                  nb_blocs=0;

		                   while ((l = read(fd_dest,&header,512))>0)
		                     {
		                          	if (header.name[0] != '\0')
			                         {
			                               	sscanf(header.size,"%o",&taille);
			                               	nb_blocs += 1 + ((taille + 512 - 1) / 512);

		                           	}
		                         	else
		                            	{
		                            		break;
		                             	}
	                       	}

	                       	//preparer l'entete
	                       	        buf_src = malloc(sizeof(struct stat));
	                       	        stat(fd_src, buf_src);

	                                memset(&entete,0,sizeof(struct posix_header));

	                              	sprintf(entete.name,"%s",src);
	                             	sprintf(entete.mode,buf_src.st_mode);

	                                //entete.typeflag = mystat.st_mode & ~S_IFMT;


	                               switch (buf_src.st_mode & S_IFMT) {
	                                case S_IFBLK:  entete.typeflag=S_IFBLK    ;   break;
	                                case S_IFCHR:  entete.typeflag=S_IFCHR    ;   break;
	                                case S_IFDIR:  entete.typeflag=S_IFDIR    ;   break;
	                                case S_IFIFO:  entete.typeflag=S_IFIFO    ;   break;
	                                case S_IFLNK:  entete.typeflag=S_IFLNK    ;   break;
	                                case S_IFREG:  entete.typeflag=S_IFREG    ;   break;
	                                case S_IFSOCK: entete.typeflag=S_IFSOCK   ;   break;
	                                default:        break;
	                                }


	                            	sprintf(entete.mtime,"%011lo",time(NULL));
	                              	sprintf(entete.uid,"%d",buf_src.st_uid);
	                             	sprintf(entete.gid,"%d",buf_src.st_gid);
	                            	sprintf(entete.uname,"%s",getpwuid(buf_src.st_gid)->pw_name);
	                              	sprintf(entete.gname,"%s",getgrgid(buf_src.st_gid)->gr_name);
	                            	sprintf(entete.size,"%011o",buf_src.st_size);
	                                strcpy(entete.magic,"ustar");
	                             	set_checksum(&hd);
	                                if (!check_checksum(&hd))
		                             	perror("Checksum impossible");


	                	while ((lus=read(fd_src,&entete,BLOCKSIZE)) > 0)
	                     {
	                          lseek(fd_dest,nb_blocs*512,SEEK_CUR);
		                       write(fd_dest,&entete,lus);

		                 }
		                 close(fd_src);
		                 close(fd_dest);
		                 return 0;
				}
				else
				//fichier destination est dans un .tar
				{
				    	printf("en construction\n");
		        return 0;
				}





	    	}*/


		printf("mv en construction\n");
		return 0;
}
/*
Affiche le contenu du fichier file en argument si cela est possible et renvoie 0.
Sinon, il renvoie 1 quand l'affichage n'est pas possible
*/
int cat(char *file, char **options,shell *tsh)
{
	char * fich = malloc(strlen(file)+strlen(tsh->repertoire_courant)+3);
	sprintf(fich,"%s%s",tsh->repertoire_courant,file);
	sprintf(fich,"%s",simplifie_chemin(fich));
	if (estTarball(fich))
	{
		char *error = malloc(strlen(file)+
						strlen("cat %s : Pas de cat sur un .tar\n"));
		sprintf(error,"cat %s : Pas de cat sur un .tar\n",file);
		write(STDERR_FILENO,error,strlen(error));
		free(fich);
		return 1;
	}
	//Fichier dans un .tar
	else
	{
		//Recherche du .tar contenant l'argument
		char *tar = calloc(strlen(fich)+3,sizeof(char));
		int index = recherche_fich_tar(fich);
		strncpy(tar,fich,index);
		tar[strlen(tar)-1] = '\0';
		char *file_to_find = malloc(1024);
		strcpy(file_to_find,&fich[index]);
		if (file[strlen(file)-1]=='/')
			file_to_find[strlen(file_to_find)-1] = '\0';
		else
			file_to_find[strlen(file_to_find)] = '\0';
		//On appelle affiche_fichier_tar qui affichera le contenu du fichier ou une erreur
		if (affiche_fichier_tar(tar,file_to_find)==0)
		{
			char *error = malloc(strlen(file)+strlen("cat \n"));
			sprintf(error,"cat %s : est un dossier\n",file);
			write(STDERR_FILENO,error,strlen(error));
			free(error);
			free(tar);
			free(file_to_find);
			return 1;
		}
		free(tar);
		free(file_to_find);
	}

	return 0;
}
