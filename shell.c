#ifndef SHELL_C
#define SHELL_C
#define _GNU_SOURCE
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "commande.h"
#include "shell.h"
#include "tar_c.h"
/*
	Fonctions gerant le fontionnement du shell
*/
/*
Renvoie le shell qui gere les commandes et les options en argument sur les
tarballs
*/
shell creation_shell(char **cmd_tarballs,char **option)
{
	shell tsh;
	memset(&tsh, 0, sizeof(shell));
	tsh.repertoire_courant = malloc(1024);
	strcpy(tsh.repertoire_courant,getcwd(NULL,1024));
	tsh.quit = 0;
	tsh.tarball = 0;
	tsh.cmd_tarballs = cmd_tarballs;
	tsh.option = option;
	tsh.nb_cmds = 0;
	while(cmd_tarballs[tsh.nb_cmds] != NULL)
	{
		tsh.nb_cmds++;
	}
	if (tsh.nb_cmds > 0)
		tsh.nb_cmds--;
	return tsh;
}
/*
Fonction qui prend en argument la commande en entier et une adresse de son index
courant.
Elle parcourt la commande et renvoie le premier mot qu'elle trouve.
Elle ne recupere pas les espaces et renvoie NULL si on a fini de lire la commande
*/
char *decoup_mot(char *commande,int *index)
{
	char *mot = malloc(strlen(commande)*sizeof(char));
	int i = 0;
	int i_commande = *index;
	if (commande[i_commande] == '\n' || commande[i_commande] == '\0')
		return NULL;
	while (commande[i_commande] == ' ') //Pour ne pas compter les espaces entre les mots
		i_commande++;
	//On parcourt la commande jusqu'a la fin de la ligne ou au prochain espace
	while (commande[i_commande] != ' ' && commande[i_commande] != '\n' && commande[i_commande] != '\0')
	{
		mot[i] = commande[i_commande];
		i++;
		i_commande++;
	}
	mot[i] = '\0';
	//On n'est pas arrive en fin de ligne. On continue de parcourir la commande
	if ((commande[i_commande] != '\n'))
		*index = i_commande;
	//On arrive a la fin de la ligne
	if(i==0)
		return NULL;
	return mot;
}
/*
Renvoie le nom du dossier enfant du chemin à l'index dont l'adresse est donnee en argument
Si on est à la fin du chemin ou que le chemin est NULL, on renvoie NULL
*/
char *decoup_nom_fich(char *chemin,int *index)
{
	if (chemin == NULL || chemin[*index]=='\0')
		return NULL;
	int i = *index;
	int debut_mot = i;
	int lg_mot = 0;
	while (chemin[i] != '/')
	{
		if (chemin[i] == '\0' || chemin[i] < 33)
			break;
		lg_mot++;
		i++;
	}
	if (chemin[i] != '\0')
		i++;
	*index = i;
	char *fichier = malloc(lg_mot);
	strncpy(fichier,&chemin[debut_mot],lg_mot);
	fichier[lg_mot] = '\0';
	return fichier;
}
/*
Simplifie le chemin absolu en enlevant les .. et . contenu dans le chemin en argument
*/
char *simplifie_chemin(char *chemin)
{
	if (chemin == NULL)
		return NULL;
	else
	{
		char *nvx_chemin = malloc(1024);
		int i = 0;
		int index = 0;
		char *name = decoup_nom_fich(chemin,&index);
		while(name != NULL)
		{
			//Présence .
			if (strcmp(name,".") == 0)
			{
				name = decoup_nom_fich(chemin,&index);
				continue;
			}
			//Présence ..
			if ((strcmp(name,"..") == 0))
			{
				i--;
				i--;
				//.. signifiant dossier parent en bash, il suffit de 'supprimer' le dossier précédant le .. du chemin
				while(nvx_chemin[i] != '/')
				{
					i--;
				}
				i++;
				name = decoup_nom_fich(chemin,&index);
				if (name == NULL)
				{
					nvx_chemin[i] = '\0';
					break;
				}
				continue;
			}
			//Dossier autre
			else
			{
				strcpy(&nvx_chemin[i],name);
				i += strlen(name) + 1;
				strcat(nvx_chemin,"/");
				name = decoup_nom_fich(chemin,&index);
			}
		}
		free(name);
		nvx_chemin[index] = '\0';
		if(nvx_chemin[i-1]=='/')
		{
			if (i >= 1)
			{
				i--;
				while(nvx_chemin[i] != '/')
				{
					i--;
				}
				i += 2;
				nvx_chemin[i] = '\0';
			}
		}
		return nvx_chemin;
	}
}
/*
Recupere la commande de l'utilisateur, renvoie la liste des arguments (mot != " ")
et Stocke son nombre dans l'adresse en argument
*/
char **recuperer_commande(int * taille_commande)
{
	char * commande = NULL;
	commande = readline(">");
	commande[strlen(commande)] = '\0';
	int index_commande = 0; //Stocke l'index courant de commande
	int taille_commande_max = 10;
	int taille_argument_max = 250;
	char **liste_argument = (char **)malloc(taille_commande_max*sizeof(char*));
	for (int l = 0;l < taille_commande_max;l++)
		liste_argument[l] = malloc(taille_argument_max * sizeof(char));
	int j = 1;
	char *nom_commande = decoup_mot(commande,&index_commande);
	liste_argument[0] = nom_commande;
	char *mot;
	//Tant qu'il y a des mots a recuperer(mot !=NULL), on les recupere
	while ((mot = decoup_mot(commande,&index_commande)))
	{
		if(j == taille_commande_max)
		{
			taille_commande_max *= 2;
			liste_argument = (char **)realloc(liste_argument,taille_commande_max*sizeof(char*));
		}
		int len = strlen(mot);
		if (len > taille_argument_max)
		{
			taille_argument_max = len*2;
			liste_argument[j] = realloc(liste_argument[j],taille_argument_max*sizeof(char));
		}
		liste_argument[j] = mot;
		j++;
	}
	//On stocke le nombre d'arguments dans l'adresse donne en parametre
	*taille_commande = j;
	for (;j < taille_commande_max;j++)
	{
		liste_argument[j] = NULL;
	}
	free(commande);
	return liste_argument;
}
/*
Analyse la ligne de commande et traite la commande
*/
int traitement_commande(char **liste_argument,int nb_arg_cmd,shell *tsh)
{
	int cmds=0;//nombre de commande entre pipe
	char *nom_commande = malloc(1024);
	//On verifie si la commande est nulle / vide
	//Si oui on revient a la ligne
	if (liste_argument[0] == NULL)
		printf("\n");
	//Sinon on traite la commande
	else
	{
		strcpy(nom_commande,liste_argument[0]);
		//Execution de la commande "exit" et depart du shell
		if(memmem(nom_commande,sizeof("exit"),"exit",sizeof("exit")))
		{
			printf("Au revoir\n");
			tsh->quit = 1;
		}
		else
		{
            //compter le nombre de commande entre pipe
		    for(int i = 0; i <nb_arg_cmd; i++){
               if (strcmp(liste_argument[i], "|") == 0){
                 cmds++;
              }
            }
            cmds++;//sauvgarde le nombre de commande entre  pipe

		    //s'il notre comamande n'a pas de pipe
		    if(cmds==1)
			{
				//On verifie que la commande peut s'effectuer sur les tar ou non
				if (estCommandeTar(nom_commande,tsh))
				{
					traitement_commandeTar(liste_argument,nb_arg_cmd,tsh);
				}
				//Sinon execution de la commande voulue si possible
				else
				{
					int pid = fork();
					if (pid == -1)
						perror("Fils non cree");
					if (pid==0)
					{
						if(execvp(nom_commande,liste_argument)==-1) //Si execvp renvoie -1, la commande n'existe pas
						{
							char error[strlen(nom_commande) + strlen("Commande   introuvable") + 1];
							sprintf(error,"Commande %s introuvable\n",nom_commande);
							write(STDERR_FILENO,error,strlen(error));
						}
						exit(0);
					}
					wait(NULL);
				}
			}

			//si notre commande contient des pipe
			else
			{
				  int i = 0;
	              int cmds=0;
	              int fd[2];
	              int fd2[2];

	              //compter le nombre de commande entre pipe
	              for(int i = 0; i < nb_arg_cmd; i++){
	                 if (strcmp(liste_argument[i], "|") == 0){
	                   cmds++;
	                 }
	              }
	              cmds++;


	             int j = 0;
	             char ** commandes = malloc(20*sizeof(char*));

	             int fin= 0;
	             pid_t pid;
	             while(liste_argument[j] != NULL && fin!= 1){

		             int k = 0;
		             while (strcmp(liste_argument[j],"|") != 0){
			              commandes[k] = liste_argument[j];
			              j++;

		                  if (liste_argument[j] == NULL){
		                     //la variable fin va nous indiquer si on a lis tout les commandes
		                     fin = 1;
		                     k++;
		                     break;
		                  }
						  k++;
	            	}
		            commandes[k] = NULL;
		            j++;

	           		if (i % 2 != 0){
					// si i est impaire

	           			pipe(fd);

	            	}

	          		else
					{
						pipe(fd2);
					}

	          		pid=fork();

					if(pid==0){

		           		if (i == 0){ //si on est dans la premier commande

		               		dup2(fd2[1], STDOUT_FILENO);
		           		}


		           		else
							if (i == cmds - 1){ // si on nest dans la dernier commande
			               		if (cmds % 2 != 0){
			                   		dup2(fd[0],STDIN_FILENO);
			               		}

			               		else{
			                   		dup2(fd2[0],STDIN_FILENO);
			                   }
		           			}


	          //si on est dans une commande qui est au millieu on doit utiliser 2 pipe un pour recuper
	          //sa sortie et lautre pour ecrire dans son entrer
		              else
					  {
			               if (i % 2 != 0){
			                   dup2(fd2[0],STDIN_FILENO);
			                   dup2(fd[1],STDOUT_FILENO);
			               }
						   else{
			                   dup2(fd[0],STDIN_FILENO);
			                   dup2(fd2[1],STDOUT_FILENO);
			               }
				   	  }
				  	  traitement_commande(commandes,k,tsh);
				  	  exit(0);
	       		}
		        if(i==0){
		     		close(fd2[1]);
		     	}
		     	else if(i==cmds -1){
		         	if(cmds % 2 !=0){ close(fd[0]); }

		          	else{ close(fd2[0]); }

		            }
		     		else{
		         		if (i%2 != 0){
		          			close(fd2[0]);
		          			close(fd[1]);
		         		}
		        		else{
		         			close(fd[0]);
		         			close(fd2[1]);
		        		}

	       			}

		   			waitpid(pid,NULL,0);
		   			i++;
	   		}
		}
	}
	}
	free(nom_commande);
	return 0;
}
#endif
