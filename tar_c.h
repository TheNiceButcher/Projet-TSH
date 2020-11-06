#ifndef TAR_C_H
#define TAR_C_H

char **list_fich(char*tar);
int affiche_fichier_tar(char *tar,char*file);
int supprimer_fichier_tar(char *tar,char *file,int option);
int creation_repertoire_tar(char *tar,char *repr);
#endif
