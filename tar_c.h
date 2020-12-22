#ifndef TAR_C_H
#define TAR_C_H

char **list_fich(char*tar);
char **affichage_ls_l(char**,char*,int,char**);
int affiche_fichier_tar(char *tar,char*file);
int supprimer_fichier_tar(char *tar,char *file,int option);
int creation_repertoire_tar(char *tar,char *repr);
#endif
