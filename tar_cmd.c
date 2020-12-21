#include "tar_cmd.h"
/*
Effectue la commande ls sur le fichier present dans un tarball
*/
int ls(char *file, char **options,shell *tsh)
{
	if (options)
	{
		//Option -l pour un fichier dans un tar (a faire)
		printf("%s:\n",file);
	}
	//Sans l'option -l
	else
	{
		/*char *fichier = malloc(strlen(tsh->repertoire_courant) + strlen(file) + 2);
		sprintf(fichier,"%s/%s",tsh->repertoire_courant,file);
		int dossier = (file[strlen(file)-1]=='/');
		if (dossier==0)
		{
			strcat(fichier,"/");
		}*/
		//Fichier dans un tarball
		if (contexteTarball(file))
		{
			//file[strlen(file)-1] = '\0';
			if (file[strlen(file)-1] == '/')
			{
				file[(strlen(file))-1] = '\0';
			}
			int s = recherche_fich_tar(file);
			//ls sur un Fichier .tar
			if (s==strlen(file))
			{
				char **list = list_fich(file);
				if (list == NULL)
				{
					char *error = malloc(strlen(file)+strlen("ls \n"));
					sprintf(error,"ls %s\n",file);
					write(STDERR_FILENO,error,strlen(error));
					free(error);
					free(file);
					//free(fichier);
					return 1;
				}
				else
				{
					//printf("\n%s:\n",file);
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
							int fin_fich = 0;
							char * mot = decoup_nom_fich(list[k],&fin_fich);
							if (list[k][strlen(list[k])-1]=='/')
								strcat(mot,"/");
							printf("%s\n",mot);
							to_print[index] = malloc(strlen(mot));
							strcpy(to_print[index],mot);
							index++;
							free(mot);
						}
						k++;
					}
					for (int f = 0; f < index;f++)
					{
						to_print[f] = NULL;
					}
					free(to_print);
				}
				int i = 0;
				while (list[i]!=NULL) {
					free(list[i]);
					i++;
				}
				free(list);
				free(file);
			}
			//ls sur un fichier dans un fichier .tar
			else
			{
				//Recherche fichier .tar contenant le fichier
				char *tar = malloc(strlen(file));
				int index = s;
				strncpy(tar,file,index);
				if (tar == NULL)
				 	printf("impossible\n");
				else
				{
					tar[index - 1] = '\0';
					char **list = list_fich(tar);
					char *file_to_find = malloc(1024);
					strncpy(file_to_find,&file[index],strlen(file)-index + 1);
					if (list == NULL)
					{
						char *error = malloc(strlen(tar)+strlen("ls \n"));
						sprintf(error,"ls %s\n",tar);
						perror(error);
						free(error);
						free(tar);
						free(file_to_find);
						return 1;
					}
					else
					{
						printf("\n%s:\n",file);
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
							if (strncmp(list[k],file_to_find,strlen(file_to_find))==0)
							{
								if (list[k][strlen(file) - index] == '\0'||list[k][strlen(file) - index] == '/')
								{
									int jpp = strlen(file_to_find) + 1;
									char *fich_to_print = decoup_nom_fich(list[k],&jpp);
									int d = 0;
									for (; d < i_deja_trouve;d++)
									{
										if (fich_to_print==NULL) continue;
										if (strcmp(fich_to_print,deja_affiche[d])==0)
											break;
									}
									if (d == i_deja_trouve)
									{
										//On n'affiche pas le nom du dossier sur lequel on appelle ls
										if (strncmp(list[k],file_to_find,strlen(file_to_find))==0 && list[k][strlen(list[k])-1] == '/')
										{
											//Dossier
											if(fich_to_print == NULL)
											{
												k++;
												continue;
											}
										}
										//ls sur un fichier != repertoire
										if (fich_to_print == NULL)
											fich_to_print = list[k];
										//Affichage d'un '/' en fin de ligne pour les repertoires
										printf("%s",fich_to_print);
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
									free(fich_to_print);
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
							char *error= malloc(strlen("ls : Aucun dossier ni fichier de ce nom\n") + strlen(file) + 2);
							sprintf(error,"ls %s: Aucun dossier ni fichier de ce nom\n",file);
							write(STDERR_FILENO,error,strlen(error));
							free(error);
						}
						free(tar);
						for(int h = 0; h < k;h++)
							free(list[h]);
						free(list);
					}
				}

			}

		}
	}
	return 0;
}
/*

*/
int cd(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	if (nb_arg_cmd == 2)
	{
		char * nv_repr_courant = malloc(strlen(liste_argument[1])+
		strlen(tsh->repertoire_courant)+2);
		//memset(nv_repr_courant,0,)
		//Prise en charge des chemins absolus depuis la racine
		if (liste_argument[1][0] == '/')
		{
			strcpy(nv_repr_courant,liste_argument[1]);
		}
		else
		{
			sprintf(nv_repr_courant,"%s",tsh->repertoire_courant);
			//strcpy(nv_repr_courant,tsh->repertoire_courant);
			if(tsh->repertoire_courant[strlen(tsh->repertoire_courant)-1] != '/')
				sprintf(nv_repr_courant,"%s/",nv_repr_courant);
			sprintf(nv_repr_courant,"%s%s",nv_repr_courant,liste_argument[1]);
		}
		if (nv_repr_courant[strlen(nv_repr_courant)-1] != '/')
			strcat(nv_repr_courant,"/");
		//Si on passe par un tarball
		if (contexteTarball(nv_repr_courant))
		{
			strcpy(nv_repr_courant,simplifie_chemin(nv_repr_courant));
			int i = recherche_fich_tar(nv_repr_courant);
			char *tar = malloc(strlen(nv_repr_courant));
			strncpy(tar,nv_repr_courant,i);
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
								return 1;
							}
							j++;
						}
						char *error= malloc(strlen("cd: Aucun dossier dans \n")+strlen(tar)+strlen(tsh->repertoire_courant)+3);
						sprintf(error,"cd: Aucun dossier %s dans %s\n",liste_argument[1],tar);
						write(STDERR_FILENO,error,strlen(error));
						free(error);

					}
					else
					{
						char *error = malloc(strlen(tar)+strlen("cd \n"));
						sprintf(error,"cd %s\n",tar);
						perror(error);
						free(error);
					}
				}
				//Le repertoire se situe a la racine du tar ou dans un dossier au dessus
				else
				{
					strcpy(tsh->repertoire_courant,nv_repr_courant);
					//On verifie si on est toujours dans un tarball
					tsh->tarball = contexteTarball(nv_repr_courant);
				}
			}
			else
			{
				char * error = malloc(strlen("cd ")+strlen(tar)+2);
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
				char error[strlen(liste_argument[1])+strlen("cd impossible") + 3];
				sprintf(error,"cd %s impossible",liste_argument[1]);
				perror(error);
			}
			else
			{
				//sprintf(nv_repr_courant,"%s",simplifie_chemin(nv_repr_courant));
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
	char * repr = malloc(strlen(tsh->repertoire_courant)+ 2);
	sprintf(repr, "%s\n",tsh->repertoire_courant);
	write(STDOUT_FILENO,repr,strlen(repr));
	free(repr);
	return 0;
}
int cp(char *file,char * destination,char ** options,shell *tsh)
{
	printf("cp en construction\n");
	return 0;
}
int rm(char *file, char **options, shell *tsh)
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
		if (options)
		{
			file[strlen(file)-1] = '\0';
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
			char *error= malloc(strlen("rm  : Veuillez utiliser l'option -r pour supprimer les .tar\n")
			+ strlen(file));
			sprintf(error,"rm %s: Veuillez utiliser l'option -r pour supprimer les .tar\n",file);
			write(STDERR_FILENO,error,strlen(error));
			free(error);
		}
	}
		//Suppression dans un .tar
	else
	{
		char *file_to_rm = malloc(strlen(file)-index);
		strcpy(file_to_rm,&file[index]);
		if(tar[strlen(tar)-1]=='/')
			tar[strlen(tar)-1] = '\0';
		supprimer_fichier_tar(tar,file_to_rm,(options!=NULL));
		free(file_to_rm);
	}
	free(tar);
	return 0;
}
int mkdir_tar(char *file, char **options,shell *tsh)
{
	char * fichier= malloc(strlen(file)+1);
	sprintf(fichier,"%s",file);
	if (file[strlen(file)-1]!='/')
	{
		strcat(fichier,"/");
		file[strlen(file)] = '\0';
	}

	int index = recherche_fich_tar(fichier);
	if (index == strlen(fichier))
	{
		printf("Création de .tar %s\n",fichier);
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
		}
		creation_repertoire_tar(tar,repr_to_create);
		free(fichier);
		free(tar);
		free(repr_to_create);

	}
	return 0;
}
int rmdir_tar(char *file, char **options,shell *tsh)
{
	printf("rmdir en construction\n");
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
int cat(char *file, char **options,shell *tsh)
{
	char * fich = malloc(strlen(file)+strlen(tsh->repertoire_courant)+3);
	sprintf(fich,"%s/%s",tsh->repertoire_courant,file);
	char *simple = malloc(strlen(fich));
	strcpy(simple,simplifie_chemin(fich));
	//Si l'argument est dans un .tar ou est en un
	if (contexteTarball(simple))
	{
		//Fichier .tar
		if (estTarball(simple))
		{
			char *error = malloc(strlen(simple)+
							strlen("cat %s : Pas de cat sur un .tar\n"));
			sprintf(error,"cat %s : Pas de cat sur un .tar\n",simple);
			write(STDERR_FILENO,error,strlen(error));
			return 1;
		}
		//Fichier dans un .tar
		else
		{
			//Recherche du .tar contenant l'argument
			char *tar = malloc(strlen(simple));
			int index = recherche_fich_tar(simple);
			strncpy(tar,simple,index);
			tar[strlen(tar)-1] = '\0';
			char **list = list_fich(tar);
			char *file_to_find = malloc(1024);
			strcpy(file_to_find,&simple[index]);
			if (list == NULL)
			{
				char *error = malloc(strlen(tar)+strlen("cat \n"));
				sprintf(error,"cat %s\n",tar);
				perror(error);
				free(error);
			}
			else
			{
				if (file[strlen(file)-1]=='/')
					file_to_find[strlen(file_to_find)-1] = '\0';
				else
					file_to_find[strlen(file_to_find)] = '\0';
				affiche_fichier_tar(tar,file_to_find);
			}
		}
	}
	return 0;
}
