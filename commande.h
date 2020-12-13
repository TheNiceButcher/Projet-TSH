#ifndef COMMANDE_H
#define COMMANDE_H
#include "shell.h"
int estTarball(char *nom_fichier);
int contexteTarball(char*chemin);
int estCommandeTar(char *mot_commande, shell * tsh);
int cheminValide(char *path,char * cmd);
int recherche_fich_tar(char *chemin);
char **recherche_option(char **liste_argument,int nb_arg_cmd);
int traitement_commandeTar(char **liste_argument,int nb_arg,shell *tsh);
#endif
