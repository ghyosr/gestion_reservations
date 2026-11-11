/* reservation.c - Module « Reservation » : gestion des réservations dans un fichier texte.
 * Format d'une ligne :
 *   id;passager;cin;vol;jj/mm/aaaa;classe;places;prix;statut
 * Aucun printf / scanf / affichage dans ce fichier. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "reservation.h"

#define LIGNE_MAX 512

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

/* Découpe "a;b;c" en champs (remplace les ';' par '\0'). Retourne le nombre de champs. */
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

/* Le séparateur du fichier est ';' : on l'interdit à l'intérieur des champs. */
static void nettoyer(char *s)
{
    for (; *s; s++)
        if (*s == ';' || *s == '\n' || *s == '\r') *s = ',';
}

static int contient(char *texte, char *motif)       /* recherche sans tenir compte des majuscules */
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

/* "12/03/2026" -> Date (0/0/0 si le texte est incorrect) */
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

/* Une ligne du fichier -> structure Reservation (retourne 1 si la ligne est correcte) */
static int ligne_vers_reservation(char *ligne, Reservation *r)
{
    char *c[12];
    ligne[strcspn(ligne, "\r\n")] = '\0';
    if (decouper(ligne, c, 12) < 9) return 0;
    copier(r->id, sizeof r->id, c[0]);
    copier(r->passager, sizeof r->passager, c[1]);
    copier(r->cin, sizeof r->cin, c[2]);
    copier(r->vol, sizeof r->vol, c[3]);
    r->date = texte_vers_date(c[4]);
    copier(r->classe, sizeof r->classe, c[5]);
    r->places = atoi(c[6]);
    r->prix = atoi(c[7]);
    copier(r->statut, sizeof r->statut, c[8]);
    return 1;
}

/* Structure Reservation -> une ligne du fichier */
static void reservation_vers_ligne(Reservation r, char *ligne, int taille)
{
    nettoyer(r.id); nettoyer(r.passager); nettoyer(r.cin);
    nettoyer(r.vol); nettoyer(r.classe); nettoyer(r.statut);
    snprintf(ligne, taille, "%s;%s;%s;%s;%02d/%02d/%04d;%s;%d;%d;%s\n",
             r.id, r.passager, r.cin, r.vol,
             r.date.jour, r.date.mois, r.date.annee,
             r.classe, r.places, r.prix, r.statut);
}

/* ------------------------------------------------------------------ */
/*  Lecture / écriture du fichier                                      */
/* ------------------------------------------------------------------ */
int charger_reservations(char *fichier, Reservation tab[], int max)
{
    FILE *f = fopen(fichier, "r");
    char ligne[LIGNE_MAX];
    int n = 0;

    if (f == NULL) return -1;
    while (n < max && fgets(ligne, sizeof ligne, f) != NULL)
        if (ligne_vers_reservation(ligne, &tab[n])) n++;
    fclose(f);
    return n;
}

int sauvegarder_reservations(char *fichier, Reservation tab[], int n)
{
    FILE *f = fopen(fichier, "w");
    char ligne[LIGNE_MAX];
    int i;

    if (f == NULL) return -1;
    for (i = 0; i < n; i++) {
        reservation_vers_ligne(tab[i], ligne, sizeof ligne);
        fputs(ligne, f);
    }
    fclose(f);
    return 1;
}

/* ------------------------------------------------------------------ */
/*  Ajouter / modifier / supprimer / chercher                          */
/* ------------------------------------------------------------------ */
int ajouter_reservation(char *fichier, Reservation r)
{
    Reservation tab[MAX_RESERVATIONS];
    char ligne[LIGNE_MAX];
    int n, i;
    FILE *f;

    n = charger_reservations(fichier, tab, MAX_RESERVATIONS);
    if (n < 0) n = 0;                          /* fichier pas encore créé : il sera créé */
    for (i = 0; i < n; i++)
        if (strcmp(tab[i].id, r.id) == 0) return 0;           /* déjà existant */

    f = fopen(fichier, "a");                   /* "a" : ajout à la fin du fichier */
    if (f == NULL) return -1;
    reservation_vers_ligne(r, ligne, sizeof ligne);
    fputs(ligne, f);
    fclose(f);
    return 1;
}

int modifier_reservation(char *fichier, char *id, Reservation r)
{
    Reservation tab[MAX_RESERVATIONS];
    int n, i, trouve = 0;

    n = charger_reservations(fichier, tab, MAX_RESERVATIONS);
    if (n < 0) return -1;
    for (i = 0; i < n; i++)
        if (strcmp(tab[i].id, id) == 0) {
            copier(r.id, sizeof r.id, id);         /* l'identifiant ne change pas */
            tab[i] = r;
            trouve = 1;
        }
    if (!trouve) return 0;
    return sauvegarder_reservations(fichier, tab, n);
}

int supprimer_reservation(char *fichier, char *id)
{
    Reservation tab[MAX_RESERVATIONS];
    int n, i, k = 0;

    n = charger_reservations(fichier, tab, MAX_RESERVATIONS);
    if (n < 0) return -1;
    for (i = 0; i < n; i++)
        if (strcmp(tab[i].id, id) != 0) tab[k++] = tab[i];    /* on garde tous les autres */
    if (k == n) return 0;                                      /* rien supprimé */
    return sauvegarder_reservations(fichier, tab, k);
}

Reservation chercher_reservation(char *fichier, char *id)
{
    Reservation tab[MAX_RESERVATIONS];
    Reservation resultat;
    int n, i;

    memset(&resultat, 0, sizeof resultat);          /* resultat.id = "" : introuvable */
    n = charger_reservations(fichier, tab, MAX_RESERVATIONS);
    for (i = 0; i < n; i++)
        if (strcmp(tab[i].id, id) == 0) {
            resultat = tab[i];
            break;
        }
    return resultat;
}

/* Change seulement le statut d'une réservation (utilisé quand elle est entièrement payée). */
int changer_statut_reservation(char *fichier, char *id, char *statut)
{
    Reservation tab[MAX_RESERVATIONS];
    int n, i, trouve = 0;

    n = charger_reservations(fichier, tab, MAX_RESERVATIONS);
    if (n < 0) return -1;
    for (i = 0; i < n; i++)
        if (strcmp(tab[i].id, id) == 0) {
            copier(tab[i].statut, sizeof tab[i].statut, statut);
            trouve = 1;
        }
    if (!trouve) return 0;
    return sauvegarder_reservations(fichier, tab, n);
}

/* Export : une ligne d'en-tête puis une réservation par ligne (colonnes séparées par ';'). */
int exporter_reservations(char *fichier_export, Reservation tab[], int n)
{
    FILE *f = fopen(fichier_export, "w");
    int i;

    if (f == NULL) return -1;
    fputs("Identifiant;Passager;CIN;Vol;Date;Classe;Places;Prix place;Montant;Statut\n", f);
    for (i = 0; i < n; i++)
        fprintf(f, "%s;%s;%s;%s;%02d/%02d/%04d;%s;%d;%d;%d;%s\n", tab[i].id, tab[i].passager, tab[i].cin, tab[i].vol,
                tab[i].date.jour, tab[i].date.mois, tab[i].date.annee, tab[i].classe, tab[i].places,
                tab[i].prix, montant_total(tab[i]), tab[i].statut);
    fclose(f);
    return 1;
}

/* ------------------------------------------------------------------ */
/*  Recherche, tri, statistiques                                       */
/* ------------------------------------------------------------------ */
int filtrer_reservations(Reservation tab[], int n, Reservation res[], char *texte, int critere, char *classe, char *statut)
{
    int i, k = 0, ok;

    for (i = 0; i < n; i++) {
        if (*classe && strcmp(tab[i].classe, classe) != 0) continue;
        if (*statut && strcmp(tab[i].statut, statut) != 0) continue;

        ok = 0;
        if ((critere == 0 || critere == 1) && contient(tab[i].id, texte)) ok = 1;
        if ((critere == 0 || critere == 2) && contient(tab[i].passager, texte)) ok = 1;
        if ((critere == 0 || critere == 3) && contient(tab[i].cin, texte)) ok = 1;
        if ((critere == 0 || critere == 4) && contient(tab[i].vol, texte)) ok = 1;
        if (ok) res[k++] = tab[i];
    }
    return k;
}

static int comparer_passagers(const void *a, const void *b)
{
    return strcasecmp(((const Reservation *)a)->passager, ((const Reservation *)b)->passager);
}

static int comparer_passagers_inverse(const void *a, const void *b)
{
    return comparer_passagers(b, a);
}

void trier_reservations(Reservation tab[], int n, int croissant)
{
    if (croissant) qsort(tab, n, sizeof(Reservation), comparer_passagers);
    else           qsort(tab, n, sizeof(Reservation), comparer_passagers_inverse);
}

int compter_statut(Reservation tab[], int n, char *statut)
{
    int i, c = 0;
    for (i = 0; i < n; i++)
        if (strcmp(tab[i].statut, statut) == 0) c++;
    return c;
}

int montant_total(Reservation r)
{
    return r.places * r.prix;
}

/* ------------------------------------------------------------------ */
/*  Contrôle de saisie                                                 */
/* ------------------------------------------------------------------ */
static int chiffres_seulement(char *s, int longueur)
{
    int i;
    if ((int)strlen(s) != longueur) return 0;
    for (i = 0; i < longueur; i++)
        if (!isdigit((unsigned char)s[i])) return 0;
    return 1;
}

static int code_valide(char *s, int min)            /* lettres, chiffres, tiret */
{
    int i;
    if ((int)strlen(s) < min) return 0;
    for (i = 0; s[i]; i++)
        if (!isalnum((unsigned char)s[i]) && s[i] != '-') return 0;
    return 1;
}

int valider_reservation(Reservation r)
{
    int i;

    if (!code_valide(r.id, 2)) return 1;
    if (est_vide(r.passager)) return 2;
    for (i = 0; r.passager[i]; i++)                 /* lettres, espaces, tiret, apostrophe (accents acceptés) */
        if (!isalpha((unsigned char)r.passager[i]) && (unsigned char)r.passager[i] < 128 &&
            r.passager[i] != ' ' && r.passager[i] != '-' && r.passager[i] != '\'') return 2;
    if (!chiffres_seulement(r.cin, 8)) return 3;
    if (!code_valide(r.vol, 3)) return 4;
    if (est_vide(r.classe)) return 5;
    if (r.places < 1 || r.places > 9) return 6;
    if (r.prix < 0) return 7;
    if (!date_valide(r.date)) return 8;
    if (strcmp(r.statut, "Confirmée") != 0 && strcmp(r.statut, "En attente") != 0 &&
        strcmp(r.statut, "Annulée") != 0) return 9;
    return 0;
}

char *message_erreur_reservation(int code)
{
    switch (code) {
        case 1: return "Identifiant invalide (2 caractères minimum : lettres, chiffres ou tiret).";
        case 2: return "Le nom du passager est obligatoire (lettres uniquement).";
        case 3: return "Le CIN doit contenir exactement 8 chiffres.";
        case 4: return "Le numéro de vol est invalide (ex : CA101).";
        case 5: return "Choisissez la classe.";
        case 6: return "Le nombre de places doit être compris entre 1 et 9.";
        case 7: return "Le prix d'une place ne peut pas être négatif.";
        case 8: return "La date de réservation est invalide.";
        case 9: return "Choisissez le statut de la réservation.";
    }
    return "";
}
