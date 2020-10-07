#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <string.h>
#include "commande.h"
/*
	Fichier qui gere la recuperation de la commande 
*/
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
	return liste_argument;
}
