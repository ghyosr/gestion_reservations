/* paiement.c - Module « Paiement » : versements des réservations dans un fichier texte.
 * Format d'une ligne :
 *   id;reservation;jj/mm/aaaa;montant;mode;remarque
 * Aucun printf / scanf / affichage dans ce fichier. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "paiement.h"

#define LIGNE_MAX 400

/* ------------------------------------------------------------------ */
/*  Petites fonctions internes                                         */
/* ------------------------------------------------------------------ */
static void copier(char *dest, int taille, char *source)
{
    strncpy(dest, source, taille - 1);
    dest[taille - 1] = '\0';
}

static int est_vide(char *s)
{
    for (; *s; s++)
        if (!isspace((unsigned char)*s)) return 0;
    return 1;
}

static int decouper(char *ligne, char *champ[], int max)
{
    int n = 0;
    char *p;
    champ[n++] = ligne;
    for (p = ligne; *p && n < max; p++)
        if (*p == ';') {
            *p = '\0';
            champ[n++] = p + 1;
        }
    return n;
}

static void nettoyer(char *s)
{
    for (; *s; s++)
        if (*s == ';' || *s == '\n' || *s == '\r') *s = ',';
}

static int contient(char *texte, char *motif)
{
    int i, j, lt = strlen(texte), lm = strlen(motif);
    if (lm == 0) return 1;
    for (i = 0; i + lm <= lt; i++) {
        for (j = 0; j < lm; j++)
            if (tolower((unsigned char)texte[i + j]) != tolower((unsigned char)motif[j])) break;
        if (j == lm) return 1;
    }
    return 0;
}

static int date_valide(Date d)
{
    int jours[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int max;
    if (d.annee < 1900 || d.annee > 2100 || d.mois < 1 || d.mois > 12) return 0;
    max = jours[d.mois - 1];
    if (d.mois == 2 && ((d.annee % 4 == 0 && d.annee % 100 != 0) || d.annee % 400 == 0)) max = 29;
    return d.jour >= 1 && d.jour <= max;
}

static Date texte_vers_date(char *s)
{
    Date d = {0, 0, 0};
    if (strlen(s) == 10 && s[2] == '/' && s[5] == '/') {
        d.jour = atoi(s);
        d.mois = atoi(s + 3);
        d.annee = atoi(s + 6);
    }
    return d;
}

static int date_num(Date d)                 /* 18/11/2026 -> 20261118 : permet de comparer deux dates */
{
    return d.annee * 10000 + d.mois * 100 + d.jour;
}

static int ligne_vers_paiement(char *ligne, Paiement *p)
{
    char *c[10];
    ligne[strcspn(ligne, "\r\n")] = '\0';
    if (decouper(ligne, c, 10) < 6) return 0;
    copier(p->id, sizeof p->id, c[0]);
    copier(p->reservation, sizeof p->reservation, c[1]);
    p->date = texte_vers_date(c[2]);
    p->montant = atoi(c[3]);
    copier(p->mode, sizeof p->mode, c[4]);
    copier(p->remarque, sizeof p->remarque, c[5]);
    return 1;
}

static void paiement_vers_ligne(Paiement p, char *ligne, int taille)
{
    nettoyer(p.id); nettoyer(p.reservation); nettoyer(p.mode); nettoyer(p.remarque);
    snprintf(ligne, taille, "%s;%s;%02d/%02d/%04d;%d;%s;%s\n",
             p.id, p.reservation, p.date.jour, p.date.mois, p.date.annee, p.montant, p.mode, p.remarque);
}

/* ------------------------------------------------------------------ */
/*  Lecture / écriture du fichier                                      */
/* ------------------------------------------------------------------ */
int charger_paiements(char *fichier, Paiement tab[], int max)
{
    FILE *f = fopen(fichier, "r");
    char ligne[LIGNE_MAX];
    int n = 0;

    if (f == NULL) return -1;
    while (n < max && fgets(ligne, sizeof ligne, f) != NULL)
        if (ligne_vers_paiement(ligne, &tab[n])) n++;
    fclose(f);
    return n;
}

int sauvegarder_paiements(char *fichier, Paiement tab[], int n)
{
    FILE *f = fopen(fichier, "w");
    char ligne[LIGNE_MAX];
    int i;

    if (f == NULL) return -1;
    for (i = 0; i < n; i++) {
        paiement_vers_ligne(tab[i], ligne, sizeof ligne);
        fputs(ligne, f);
    }
    fclose(f);
    return 1;
}

int exporter_paiements(char *fichier_export, Paiement tab[], int n)
{
    FILE *f = fopen(fichier_export, "w");
    int i;

    if (f == NULL) return -1;
    fputs("Identifiant;Reservation;Date;Montant (TND);Mode;Remarque\n", f);
    for (i = 0; i < n; i++)
        fprintf(f, "%s;%s;%02d/%02d/%04d;%d;%s;%s\n", tab[i].id, tab[i].reservation, tab[i].date.jour,
                tab[i].date.mois, tab[i].date.annee, tab[i].montant, tab[i].mode, tab[i].remarque);
    fclose(f);
    return 1;
}

/* ------------------------------------------------------------------ */
/*  Ajouter / modifier / supprimer / chercher                          */
/* ------------------------------------------------------------------ */
int ajouter_paiement(char *fichier, Paiement p)
{
    Paiement tab[MAX_PAIEMENTS];
    char ligne[LIGNE_MAX];
    int n, i;
    FILE *f;

    n = charger_paiements(fichier, tab, MAX_PAIEMENTS);
    if (n < 0) n = 0;
    for (i = 0; i < n; i++)
        if (strcmp(tab[i].id, p.id) == 0) return 0;

    f = fopen(fichier, "a");
    if (f == NULL) return -1;
    paiement_vers_ligne(p, ligne, sizeof ligne);
    fputs(ligne, f);
    fclose(f);
    return 1;
}

int modifier_paiement(char *fichier, char *id, Paiement p)
{
    Paiement tab[MAX_PAIEMENTS];
    int n, i, trouve = 0;

    n = charger_paiements(fichier, tab, MAX_PAIEMENTS);
    if (n < 0) return -1;
    for (i = 0; i < n; i++)
        if (strcmp(tab[i].id, id) == 0) {
            copier(p.id, sizeof p.id, id);              /* l'identifiant ne change pas */
            tab[i] = p;
            trouve = 1;
        }
    if (!trouve) return 0;
    return sauvegarder_paiements(fichier, tab, n);
}

int supprimer_paiement(char *fichier, char *id)
{
    Paiement tab[MAX_PAIEMENTS];
    int n, i, k = 0;

    n = charger_paiements(fichier, tab, MAX_PAIEMENTS);
    if (n < 0) return -1;
    for (i = 0; i < n; i++)
        if (strcmp(tab[i].id, id) != 0) tab[k++] = tab[i];
    if (k == n) return 0;
    return sauvegarder_paiements(fichier, tab, k);
}

int supprimer_paiements_reservation(char *fichier, char *reservation)
{
    Paiement tab[MAX_PAIEMENTS];
    int n, i, k = 0;

    n = charger_paiements(fichier, tab, MAX_PAIEMENTS);
    if (n <= 0) return 0;
    for (i = 0; i < n; i++)
        if (strcmp(tab[i].reservation, reservation) != 0) tab[k++] = tab[i];
    if (k == n) return 0;
    sauvegarder_paiements(fichier, tab, k);
    return n - k;
}

Paiement chercher_paiement(char *fichier, char *id)
{
    Paiement tab[MAX_PAIEMENTS];
    Paiement resultat;
    int n, i;

    memset(&resultat, 0, sizeof resultat);
    n = charger_paiements(fichier, tab, MAX_PAIEMENTS);
    for (i = 0; i < n; i++)
        if (strcmp(tab[i].id, id) == 0) {
            resultat = tab[i];
            break;
        }
    return resultat;
}

/* ------------------------------------------------------------------ */
/*  Recherche, tri, statistiques                                       */
/* ------------------------------------------------------------------ */
int filtrer_paiements(Paiement tab[], int n, Paiement res[], char *texte, int critere, char *mode)
{
    int i, k = 0, ok;

    for (i = 0; i < n; i++) {
        if (*mode && strcmp(tab[i].mode, mode) != 0) continue;

        ok = 0;
        if ((critere == 0 || critere == 1) && contient(tab[i].id, texte)) ok = 1;
        if ((critere == 0 || critere == 2) && contient(tab[i].reservation, texte)) ok = 1;
        if ((critere == 0 || critere == 3) && contient(tab[i].remarque, texte)) ok = 1;
        if (ok) res[k++] = tab[i];
    }
    return k;
}

static int comparer_dates(const void *a, const void *b)
{
    return date_num(((const Paiement *)a)->date) - date_num(((const Paiement *)b)->date);
}

static int comparer_dates_inverse(const void *a, const void *b)
{
    return comparer_dates(b, a);
}

void trier_paiements(Paiement tab[], int n, int croissant)
{
    if (croissant) qsort(tab, n, sizeof(Paiement), comparer_dates);
    else           qsort(tab, n, sizeof(Paiement), comparer_dates_inverse);
}

int total_paye(Paiement tab[], int n, char *reservation)
{
    int i, somme = 0;
    for (i = 0; i < n; i++)
        if (strcmp(tab[i].reservation, reservation) == 0) somme += tab[i].montant;
    return somme;
}

int somme_paiements(Paiement tab[], int n)
{
    int i, somme = 0;
    for (i = 0; i < n; i++) somme += tab[i].montant;
    return somme;
}

int compter_mode(Paiement tab[], int n, char *mode)
{
    int i, c = 0;
    for (i = 0; i < n; i++)
        if (strcmp(tab[i].mode, mode) == 0) c++;
    return c;
}

/* ------------------------------------------------------------------ */
/*  Liens avec les réservations                                        */
/* ------------------------------------------------------------------ */
int reste_a_payer(char *fic_reservations, char *fic_paiements, char *reservation)
{
    Paiement tab[MAX_PAIEMENTS];
    Reservation r = chercher_reservation(fic_reservations, reservation);
    int n;

    if (r.id[0] == '\0') return -1;
    n = charger_paiements(fic_paiements, tab, MAX_PAIEMENTS);
    if (n < 0) n = 0;
    return montant_total(r) - total_paye(tab, n, reservation);
}

int controler_paiement(char *fic_reservations, char *fic_paiements, Paiement p, char *exclu)
{
    Paiement tab[MAX_PAIEMENTS];
    Reservation r = chercher_reservation(fic_reservations, p.reservation);
    int n, i, deja = 0;

    if (r.id[0] == '\0') return 11;
    if (strcmp(r.statut, "Annulée") == 0) return 12;
    n = charger_paiements(fic_paiements, tab, MAX_PAIEMENTS);
    if (n < 0) n = 0;
    for (i = 0; i < n; i++)                         /* on ignore le paiement qu'on est en train de modifier */
        if (strcmp(tab[i].reservation, p.reservation) == 0 && strcmp(tab[i].id, exclu) != 0)
            deja += tab[i].montant;
    if (deja + p.montant > montant_total(r)) return 13;
    return 0;
}

int synchroniser_statut_reservation(char *fic_reservations, char *fic_paiements, char *reservation)
{
    Reservation r = chercher_reservation(fic_reservations, reservation);

    if (r.id[0] == '\0') return 0;
    if (strcmp(r.statut, "En attente") == 0 && montant_total(r) > 0 &&
        reste_a_payer(fic_reservations, fic_paiements, reservation) == 0)
        return changer_statut_reservation(fic_reservations, reservation, "Confirmée") == 1;
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Contrôle de saisie                                                 */
/* ------------------------------------------------------------------ */
int valider_paiement(Paiement p)
{
    int i;

    if (strlen(p.id) < 3) return 1;
    for (i = 0; p.id[i]; i++)
        if (!isalnum((unsigned char)p.id[i]) && p.id[i] != '-') return 1;
    if (est_vide(p.reservation)) return 2;
    if (!date_valide(p.date)) return 3;
    if (p.montant < 1) return 4;
    if (strcmp(p.mode, "Espèces") != 0 && strcmp(p.mode, "Carte bancaire") != 0 &&
        strcmp(p.mode, "Virement") != 0 && strcmp(p.mode, "Chèque") != 0) return 5;
    return 0;
}

char *message_erreur_paiement(int code)
{
    switch (code) {
        case 1: return "Identifiant invalide (3 caractères minimum : lettres, chiffres ou tiret).";
        case 2: return "Choisissez la réservation payée.";
        case 3: return "La date du paiement est invalide.";
        case 4: return "Le montant doit être d'au moins 1 TND.";
        case 5: return "Choisissez le mode de paiement.";
        case 11: return "Réservation introuvable.";
        case 12: return "Cette réservation est annulée : aucun paiement possible.";
        case 13: return "Le montant dépasse le reste à payer de la réservation.";
    }
    return "";
}
