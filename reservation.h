#ifndef RESERVATION_H
#define RESERVATION_H

/* reservation.h - Module « Reservation » (le module porte le nom de la structure).
 * Les fonctions travaillent sur un fichier texte : on vérifie le résultat
 * directement dans le fichier. AUCUN printf / scanf / affichage ici. */

#include "date.h"

#define MAX_RESERVATIONS 300


typedef struct {
    char id[12];             /* identifiant unique, ex : R001              */
    char passager[30];       /* nom du passager                            */
    char cin[9];             /* CIN : 8 chiffres                           */
    char vol[10];            /* numéro du vol, ex : CA101                  */
    Date date;               /* date de la réservation                     */
    char classe[16];         /* Économique / Affaires / Première           */
    int  places;             /* nombre de places réservées (1 à 9)         */
    int  prix;               /* prix d'une place en TND                    */
    char statut[16];         /* Confirmée / En attente / Annulée           */
} Reservation;

/* ---- fonctions de gestion du fichier -------------------------------- */
int  ajouter_reservation(char *fichier, Reservation r);                /* 1 = ok, 0 = identifiant déjà utilisé, -1 = erreur fichier */
int  modifier_reservation(char *fichier, char *id, Reservation r);     /* 1 = ok, 0 = introuvable, -1 = erreur fichier */
int  supprimer_reservation(char *fichier, char *id);                   /* 1 = ok, 0 = introuvable, -1 = erreur fichier */
Reservation chercher_reservation(char *fichier, char *id);             /* r.id vide si la réservation est introuvable */

int  charger_reservations(char *fichier, Reservation tab[], int max);  /* nombre lu, -1 si fichier absent */
int  sauvegarder_reservations(char *fichier, Reservation tab[], int n);/* 1 = ok, -1 = erreur */

int  changer_statut_reservation(char *fichier, char *id, char *statut);   /* 1 = ok, 0 = introuvable, -1 = erreur fichier */
int  exporter_reservations(char *fichier_export, Reservation tab[], int n); /* écrit un fichier .csv lisible dans Excel */

/* ---- recherche, tri, statistiques ----------------------------------- */
/* critere : 0 = tous les champs, 1 = identifiant, 2 = passager, 3 = CIN, 4 = vol
 * classe / statut : chaîne vide = pas de filtre */
int  filtrer_reservations(Reservation tab[], int n, Reservation res[], char *texte, int critere, char *classe, char *statut);
void trier_reservations(Reservation tab[], int n, int croissant);      /* tri sur le nom du passager */
int  compter_statut(Reservation tab[], int n, char *statut);           /* nombre de réservations dans ce statut */
int  montant_total(Reservation r);                                     /* places x prix d'une place */

/* ---- contrôle de saisie --------------------------------------------- */
int   valider_reservation(Reservation r);   /* 0 = valide, sinon numéro de l'erreur */
char *message_erreur_reservation(int code);

#endif
