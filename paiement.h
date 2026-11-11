#ifndef PAIEMENT_H
#define PAIEMENT_H

/* paiement.h - Module « Paiement » (le module porte le nom de la structure).
 * Un paiement est un versement fait pour une réservation (on peut payer en plusieurs fois).
 * Les fonctions travaillent sur un fichier texte : AUCUN printf / scanf ici. */
#include "date.h"
#include "reservation.h"

#define MAX_PAIEMENTS 500

typedef struct {
    char id[12];             /* identifiant unique, ex : P001              */
    char reservation[12];    /* réservation payée (identifiant)            */
    Date date;               /* date du paiement                           */
    int  montant;            /* montant versé en TND                       */
    char mode[16];           /* Espèces / Carte bancaire / Virement / Chèque */
    char remarque[60];       /* texte libre (facultatif)                   */
} Paiement;

/* ---- fonctions de gestion du fichier -------------------------------- */
int      ajouter_paiement(char *fichier, Paiement p);                  /* 1 = ok, 0 = identifiant déjà utilisé, -1 = erreur fichier */
int      modifier_paiement(char *fichier, char *id, Paiement p);       /* 1 = ok, 0 = introuvable, -1 = erreur fichier */
int      supprimer_paiement(char *fichier, char *id);                  /* 1 = ok, 0 = introuvable, -1 = erreur fichier */
Paiement chercher_paiement(char *fichier, char *id);                   /* p.id vide si introuvable */
int      supprimer_paiements_reservation(char *fichier, char *reservation);   /* supprime tous les paiements d'une réservation : nombre supprimé */

int      charger_paiements(char *fichier, Paiement tab[], int max);    /* nombre lu, -1 si fichier absent */
int      sauvegarder_paiements(char *fichier, Paiement tab[], int n);  /* 1 = ok, -1 = erreur */
int      exporter_paiements(char *fichier_export, Paiement tab[], int n);

/* ---- recherche, tri, statistiques ----------------------------------- */
/* critere : 0 = tous les champs, 1 = identifiant, 2 = réservation, 3 = remarque
 * mode : chaîne vide = pas de filtre */
int      filtrer_paiements(Paiement tab[], int n, Paiement res[], char *texte, int critere, char *mode);
void     trier_paiements(Paiement tab[], int n, int croissant);        /* tri sur la date du paiement */
int      total_paye(Paiement tab[], int n, char *reservation);         /* somme déjà payée pour cette réservation */
int      somme_paiements(Paiement tab[], int n);                       /* somme de tous les paiements */
int      compter_mode(Paiement tab[], int n, char *mode);              /* nombre de paiements faits avec ce mode */

/* ---- liens avec les réservations ------------------------------------ */
/* montant de la réservation - déjà payé ; -1 si la réservation est introuvable */
int      reste_a_payer(char *fic_reservations, char *fic_paiements, char *reservation);

/* Vérifie qu'on peut enregistrer ce paiement (exclu = identifiant du paiement en cours de modification, ou "") :
 * 0 = possible, 11 = réservation introuvable, 12 = réservation annulée, 13 = montant supérieur au reste à payer */
int      controler_paiement(char *fic_reservations, char *fic_paiements, Paiement p, char *exclu);

/* Une réservation « En attente » entièrement payée devient « Confirmée ».
 * Retourne 1 si le statut a changé, 0 sinon. */
int      synchroniser_statut_reservation(char *fic_reservations, char *fic_paiements, char *reservation);

/* ---- contrôle de saisie --------------------------------------------- */
int      valider_paiement(Paiement p);          /* 0 = valide, sinon numéro de l'erreur (1 à 5) */
char    *message_erreur_paiement(int code);     /* messages des codes 1 à 5 et 11 à 13 */

#endif
