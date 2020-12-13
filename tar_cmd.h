/*
	Fonctions qui retranscrivent le comportement des commandes sur les tar
*/
#ifndef TAR_CMD_H
#define TAR_CMD_H
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include "shell.h"
#include "commande.h"
#include "tar_c.h"
int ls(char *file, char **options,shell *tsh);
int cd(char **liste_argument,int nb_arg_cmd,shell *tsh);
int pwd(char **liste_argument,int nb_arg_cmd,shell *tsh);
int cp(char *file,char *destination,char ** options,shell *tsh);
int rm(char *file, char **options, shell *tsh);
int mkdir_tar(char *file, char **options,shell *tsh);
int rmdir_tar(char *file, char **options,shell *tsh);
int mv(char *file,char *destination,char ** options,shell *tsh);
int cat(char *file, char **options,shell *tsh);
#endif
