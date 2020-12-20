#ifndef SHELL_H
#define SHELL_H
#include "tar_c.h"
/*
	Fichier qui gere la recuperation de la commande du shell
*/
/*
Structure qui stocke le repertoire_courant, la variable pour quitter le shell,
la variable pour savoir si on est pas dans un tarball ou non, les commandes
à implémenter sur les tarballs et leurs options
*/
typedef struct shell_s{
	char *repertoire_courant;
	int quit;
	int tarball;
	char ** cmd_tarballs;
	char ** option;
	int nb_cmds;
} shell;
char * chemin_a_explorer;
int index_chemin_a_explorer;
int chemin_length;
shell creation_shell(char ** commandes,char **options);
void init_chemin_explorer(char *path);
void free_chemin_explorer();
char *decoup_mot(char *commande,int *index);
char *decoup_nom_fich(char *chemin,int *index);
int decoup_fich(char*);
char *simplifie_chemin(char *chemin);
char *simplifie_chemin_aux(char *chemin);
char **recuperer_commande(int * taille_commande);
int traitement_commande(char**liste_argument,int nb_arg_cmd,shell*tsh);
#endif
