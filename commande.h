#ifndef COMMANDE_H
#define COMMANDE_H
#include "shell.h"
int estTarball(char *nom_fichier);
int contexteTarball(char*chemin);
int estCommandeTar(char *mot_commande);
int recherche_fich_tar(char *chemin);
char **recherche_option(char **liste_argument,int nb_arg_cmd);
int ls(char **liste_argument,int nb_arg_cmd,shell *tsh);
int cd(char **liste_argument,int nb_arg_cmd,shell *tsh);
int pwd(char **liste_argument,int nb_arg_cmd,shell *tsh);
int cp(char **liste_argument,int nb_arg_cmd,shell *tsh);
int rm(char **liste_argument,int nb_arg_cmd,shell *tsh);
int mkdir_tar(char **liste_argument,int nb_arg_cmd,shell *tsh);
int rmdir_tar(char **liste_argument,int nb_arg_cmd,shell *tsh);
int mv(char **liste_argument,int nb_arg_cmd,shell *tsh);
int cat(char **liste_argument,int nb_arg_cmd,shell *tsh);
#endif
