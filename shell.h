#ifndef SHELL_H
#define SHELL_H
/*
	Fichier qui gere la recuperation de la commande du shell
*/
/*
Structure qui stocke le repertoire_courant, la variable pour quitter le shell
et la variable pour savoir si on est pas dans un tarball ou non
*/
typedef struct shell_s{
	char *repertoire_courant;
	int quit;
	int tarball;
} shell;
char *decoup_mot(char *commande,int *index);
char *decoup_nom_fich(char *chemin,int *index);
char *simplifie_chemin(char *chemin);
char **recuperer_commande(int * taille_commande);
int traitement_commande(char**liste_argument,int nb_arg_cmd,shell*tsh);
int traitement_commandeTar(char **liste_argument,int nb_arg,shell *tsh);
#endif
