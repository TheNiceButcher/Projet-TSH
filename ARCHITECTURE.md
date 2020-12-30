Projet TSH 2020-2021
# Stratégie pour répondre aux besoin du projet:
* Ce projet portant sur un shell traitant les fichiers `.tar` comme des répertoires, il faut répondre aux besoins suivants:
	* Récupérer une commande transmise par l'utilisateur
	* L'analyser pour savoir quoi faire
	* Exécuter les actions voulues par l'utilisateur via la commande


* Création d'un shell

Pour recevoir les commandes de l'utilisateur, il faut être prêt à récupérer les informations qu'il nous donne. Pour ce faire, nous avons créer une boucle qui ne se finit qu'une fois que l'utilisateur le veuille. On a alors défini la variable `quit` dans la structure `shell` qui contient d'autres informations comme le répertoire courant(`repertoire_courant`) ou le fait qu'on soit dans un tarball(`tarball`), qui vérifie cela. La boucle et le programme s'arrêtera une fois cet variable mise à 1. Maintenant que nous avons une boucle pour recevoir les commandes, il faut les récupérer.


* Récupération d'une commande

Pour récupérer la commande tapée par l'utilisateur, on utilise la fonction `recuperer_commande`, qui va via la fonction readline de la bibliothèque readline,va avoir la liste de caractères qui correspond à la commande de l'utilisateur. Elle fait alors appel à `decoup_mot` sur cette liste de caractères, qui parcourt la liste à un index donné et renvoie un mot contenu entre deux espaces, qui correspond à un des arguments de la commande de l'utilisateur. On l'appelle jusqu'à la fin de la commande et on stocke à chaque fois le résultat dans une liste. On renvoie alors cette liste d'arguments qui va servir à l'analyser.


* Analyse de la commande

On a alors la liste d'arguments donnée par `recuperer_commande` et on va devoir l'analyser pour savoir quoi faire. On fait alors appel à `traitement_commande`. Il vérifie d'abord que la liste d'arguments n'est pas vide. Si elle est vide, on fait juste un retour à la ligne. Sinon, on parcourt la liste d'arguments et on vérifie s'il y a des tubes ou des redirections à gérer (avec la présence de '|' ou de '<' et '>').

S'il n'y a pas de tubes ou de redirections, on regarde alors le premier mot de la liste d'arguments, qui donne le nom de la commande à effectuer et l'action à faire. Si ce mot est `exit`, cela signifie que l'on souhaite quitter le shell. On met alors la variable qui gère la boucle à 1. Sinon, on vérifie si cette commande peut être utilisé sur les tarballs ou non (c'est-à-dire si on doit étendre le champ d'action de la commande par rapport au bash). Pour le vérifier, nous utilisons la fonction `estCommandeTar` qui renvoie 1 si c'est le cas, ou 0 sinon. Avec ce résultat, on sait alors quoi faire et on peut passer à l'exécution.

S'il y en a, on parcourt alors la liste d'arguments, en s'arrêtant à chaque symbole indiquant un tube ou une redirection. Entre chaque symbole, on récupère les mots et on effectue la même vérification que dans le paragraphe d'au-dessus sur ces derniers. Sauf qu'avant de l'exécuter, on doit soit créer un tube, ou une redirection. Pour le tube, on crée deux tableaux de descripteurs pour les utiliser avec la fonction `pipe` de la bibliothèque standard. Ils permettront de donner comme entrée le résultat de la commande d'avant et comme sortie la commande d'après. Pour la redirection, on fera divers appels à `dup2` pour rediriger la commande vers la commande suivante. Une fois cela fait, la situation est la même que dans le précédent paragraphe: on peut passer à l'exécution de la commande.


* Exécution de la commande

* Avec ce résultat de `estCommandeTar`, on a alors deux possibilités:
	* On a une commande ne gérant pas les tarballs, on appelle alors `execlp`(bibliothèque standard) qui effectue la commande comme dans le bash,
	* Sinon, on appelle la fonction `traitement_commandeTar` avec la liste d'arguments, sa taille et un pointeur sur la structure `shell` définie avant la boucle du programme.

Dans le second cas, `traitement_commandeTar` vérifie tout d'abord si la commande peut gérer plusieurs arguments ou non. Si elle ne peut gérer qu'au plus un argument (comme `pwd` et `cd`), on appelle leur fonction homonyme. Sinon, on parcourt les différents arguments de la commande pour vérifier si la commande est applicable et le cas échéant, la traiter. Pour ce faire, on applique à chaque argument les fonctions `cheminValide`, attestant l'existence d'un fichier, et de `contexteTarball`, indiquant si l'argument est dans un tarball ou non,
afin de traiter au mieux la demande de l'utilisateur. Selon les résultats de ces dernières, on fait les actions suivantes:

	* Si l'existence ou la non-existence de l'argument pose problème à la commande, on renvoie une erreur et on passe à l'argument suivant,
	* Si l'argument est valide, et dans un contexte non Tarball, on appelle exec avec le nom de la commande, l'argument et les options s'il y en a,
	* Enfin, si l'argument est valide, et dans un contexte Tarball, on appelle la fonction présente dans `tar_cmd` correspondant à l'action voulue, si les options le permettent, et on passe à l'argument suivant.  


L'action voulue est donc effectuée. On repart alors pour un autre tour du boucle et ainsi de suite, jusqu'à la commande `exit` faisant quitter le shell.


# Architecture logicielle :

* Voici comment est décomposé le projet:
	* tsh.c contient la boucle principale du programme. Il initialise la structure `shell` utilisée dans la suite du programme et appelle les fonctions `recuperer_commande` et `traitement_commande`;
	* commande .c/.h définit les fonctions auxiliaires des commandes que l'on souhaite effectuer sur les tarballs, et d'autres fonctions utiles pour ces dernières (`contexteTarball`,`estTarball` pour savoir si on est dans un contexte .tar ou `recherche_option` pour connaître les options d'une commande étant dans `estCommandeTar`);
	* tar_cmd .c/.h sert de relai entre commande et tar_c. C'est ce module qui appelle les fonctions de tar_cmd, en fonction de la commande tapée par l'utilisateur, et qui vérifie une dernière fois la validité des arguments avant l'appel aux fonctions de tar_c. Prenons l'exemple de `supprimer_fichier` : elle est appelée quand les commandes `rm` et `rmdir` sont tapées et vérifie si la commande est applicable ou non. Si c'est le cas, on appelle `supprimer_fichier_tar`. Sinon, elle renvoie une erreur.
	* shell .c/.h gère la partie 'shell' du projet, avec la création de la structure `shell` et des fonctions `recuperer_commande`,`traitement_commande` ou encore des fonctions auxiliaires de ces dernières,
	* tar_c .c/.h effectue les différentes actions sur les tarballs voulues, comme `list_fich` qui renvoie la liste des fichiers dans un .tar pouvant être utile pour `ls` ou encore `supprimer_fichier_tar` pour `rm`;
	* tar.h/c, repris du TP1, permet d'utiliser la structure posix_header, indispensable pour parcourir les tarballs et effectuer des commandes dessus;
