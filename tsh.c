#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "commande.h"
#include <glib.h>

int main(int argc, char const *argv[]) {
	int quit = 0; //Variable qui enregistre si on execute la commande "exit" qui permet de sortir du shell
	while (quit == 0)
	{
		int nb_arg_cmd; //Stocke le nombre d'arguments de la commande
		char **liste_argument =  recuperer_commande(&nb_arg_cmd);
		char *nom_commande = liste_argument[0];
		//les variable utilsable pour gerer les pipe
		int num_cmds_pipe = 0;//nombre de commande entre pipe
		char *commandes[256];//pour stocker les commande des pipe
        int fd[2];
        int fd2[2];
        int i_c = 0;
        int j = 0;
        int fin = 0;
        pid_t pid;
		//On verifie si la commande est nulle / vide
		//Si oui on revient a la ligne
		if (nom_commande == NULL)
			printf("\n");
		//Sinon on traite la commande
		else
		{
			//Execution de la commande "exit" et depart du shell
			if(memmem(nom_commande,sizeof("exit"),"exit",sizeof("exit")))
			{
				printf("Au revoir\n");
				quit = 1;
			}
			//Execution de la commande "cd" apres avoir sauvegarder le répertoire courant
			gchar *g_get_current_dir (void);
			else if(memmem(nom_commande,sizeof("cd"),"cd",sizeof("cd")))
           {
             gdir =nom_commande ;
             dir = strcat(gdir, "/");
             to = strcat(dir,liste_argument[1]);
             chdir(to);
           }
			
			
			//Sinon execution de la commande voulue si possible
			
			else
			{
			     //calculer le nombre de commande avec pipe
                 for(int i = 0; i < sizeof(liste_argument); i++)
                 {
                  if (strcmp(liste_argument[i], "|") == 0){
                        num_cmds_pipe++;
                  }
                 }
              num_cmds_pipe++;
           
            //s'il n y a pas des pipe
             if(num_cmds_pipe <= 1)
             {
         
				int pid = fork();
				if (pid==0)
				{
					if(execvp(nom_commande,liste_argument)==-1) //Si execvp renvoie -1, la commande n'existe pas
						printf("Commande introuvable\n");
					exit(0);
				}
				wait(NULL);
				
             }
             else{
                   while(liste_argument[j] != NULL && fin != 1){
          int c = 0;

          while (strcmp(liste_argument[j],"|") != 0){
             commandes[c] = liste_argument[j];
              j++;
              if (liste_argument[j] == NULL){
                  c++;
                  // la variable fin pour savoir quil y a plus de commande dans la liste des arguments
                  fin = 1;
                  break;
              }
              c++;
          }
          // pour marquer la dernnier possition dans commandes
         
         commandes[c] = NULL;
          j++;

      
      // si la commandes est dans une position impaire
      if (i % 2 != 0){
           pipe(fd); 
       } 
      // sinon si la commandes est dans une position paire
        else{
           pipe(fd2); 
       }
       pid_p=fork();

      if(pid_p==-1){
           if (i != num_cmds_pipe - 1){
      //si la commandes est dans une position impaire
               if (i % 2 != 0){
                   close(fd[1]);
               }
      // sinon si la commandes est dans une position paire 
               else{
                   close(fd2[1]); 
               }
           }
          
           return;
       }
       if(pid_p==0){
           // si on est dans la premiere commande
           if (i == 0){
               dup2(fd2[1], STDOUT_FILENO);
           }
           //si on est dans la derniere commande 
           //on doit verrifier si on est dans la derniere commande 
           //avant de verifier si on n'est au millieu  
           else if (i == num_cmds_pipe - 1){
              //si la commandes est dans une position impaire
               if (num_cmds_pipe % 2 != 0){ 
                   dup2(fd[0],STDIN_FILENO);
                  }
              //si la commandes est dans une position paire
                else{
                   dup2(fd2[0],STDIN_FILENO);
               }

          
           } 
           // si on est dans une commande au millieu on doit utiliser 
           // on doit utiliser deux pipe
           else{ 
           //si la commandes est dans une position impaire
               if (i % 2 != 0){
                   dup2(fd2[0],STDIN_FILENO);
                   dup2(fd[1],STDOUT_FILENO);
               }
                 //si la commandes est dans une position paire
                else{
                   dup2(fd[0],STDIN_FILENO);
                   dup2(fd2[1],STDOUT_FILENO);
               }
           }
           execvp(commandes[0],commandes);
       }

       // fermer les descripteurs 
       if (i == 0){
           close(fd2[1]);
       }
       else if (i == num_cmds_pipe - 1){
           if (num_cmds_pipe % 2 != 0){
               close(fd[0]);
           }else{
               close(fd2[0]);
           }
       }else{
           if (i % 2 != 0){
               close(fd2[0]);
               close(fd[1]);
           }else{
               close(fd[0]);
               close(fd2[1]);
           }
       }

       waitpid(pid_p,NULL,0);

       i++;
     }
             }
			}
		}
		//Liberation de la memoire
		for(int i = 0; i < nb_arg_cmd;i++)
			free(liste_argument[i]);
		free(liste_argument);
	}
	return 0;
}
