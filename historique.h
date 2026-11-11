#ifndef HISTORIQUE_H
#define HISTORIQUE_H

/* historique.h - Module « Historique » : journal de toutes les actions faites
 * sur les réservations (ajout, modification, suppression, paiement...).
 * Une ligne par action dans un fichier texte. AUCUN printf / scanf ici. */

#define MAX_HIST 1000

typedef struct {
    char date[20];           /* "18/11/2026 14:35:02"                    */
    char utilisateur[20];    /* qui a fait l'action                      */
    char reference[12];       /* réservation concernée                           */
    char action[16];         /* Ajout / Modification / Suppression / Paiement / Export */
    char details[80];        /* explication courte                       */
} Historique;

/* Ajoute une ligne datée d'AUJOURD'HUI : 1 = ok, -1 = erreur fichier */
int  ajouter_historique(char *fichier, char *utilisateur, char *reference, char *action, char *details);
/* Même chose avec une structure complète (date choisie) - utile pour les tests */
int  enregistrer_historique(char *fichier, Historique h);

int  charger_historique(char *fichier, Historique tab[], int max);   /* nombre lu, -1 si fichier absent */
int  vider_historique(char *fichier);                                /* efface tout le journal : 1 = ok, -1 = erreur */
int  exporter_historique(char *fichier_export, Historique tab[], int n);

/* texte cherché dans la date, l'utilisateur, la réservation et les détails ; action vide = toutes */
int  filtrer_historique(Historique tab[], int n, Historique res[], char *texte, char *action);
int  compter_action(Historique tab[], int n, char *action);

#endif
