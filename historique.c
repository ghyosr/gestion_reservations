/* historique.c - Module « Historique » : journal des actions dans un fichier texte.
 * Format d'une ligne :  jj/mm/aaaa hh:mm:ss;utilisateur;reference;action;details
 * Aucun printf / scanf / affichage dans ce fichier. */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "historique.h"

#define LIGNE_MAX 300

static void copier(char *dest, int taille, char *source)
{
    strncpy(dest, source, taille - 1);
    dest[taille - 1] = '\0';
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

int enregistrer_historique(char *fichier, Historique h)
{
    FILE *f = fopen(fichier, "a");               /* "a" : on ajoute à la fin du journal */

    if (f == NULL) return -1;
    nettoyer(h.date); nettoyer(h.utilisateur); nettoyer(h.reference); nettoyer(h.action); nettoyer(h.details);
    fprintf(f, "%s;%s;%s;%s;%s\n", h.date, h.utilisateur, h.reference, h.action, h.details);
    fclose(f);
    return 1;
}

int ajouter_historique(char *fichier, char *utilisateur, char *reference, char *action, char *details)
{
    Historique h;
    time_t maintenant = time(NULL);
    struct tm *t = localtime(&maintenant);

    memset(&h, 0, sizeof h);
    strftime(h.date, sizeof h.date, "%d/%m/%Y %H:%M:%S", t);
    copier(h.utilisateur, sizeof h.utilisateur, utilisateur);
    copier(h.reference, sizeof h.reference, reference);
    copier(h.action, sizeof h.action, action);
    copier(h.details, sizeof h.details, details);
    return enregistrer_historique(fichier, h);
}

int charger_historique(char *fichier, Historique tab[], int max)
{
    FILE *f = fopen(fichier, "r");
    char ligne[LIGNE_MAX], *c[8];
    int n = 0;

    if (f == NULL) return -1;
    while (n < max && fgets(ligne, sizeof ligne, f) != NULL) {
        ligne[strcspn(ligne, "\r\n")] = '\0';
        if (decouper(ligne, c, 8) < 5) continue;
        copier(tab[n].date, sizeof tab[n].date, c[0]);
        copier(tab[n].utilisateur, sizeof tab[n].utilisateur, c[1]);
        copier(tab[n].reference, sizeof tab[n].reference, c[2]);
        copier(tab[n].action, sizeof tab[n].action, c[3]);
        copier(tab[n].details, sizeof tab[n].details, c[4]);
        n++;
    }
    fclose(f);
    return n;
}

int vider_historique(char *fichier)
{
    FILE *f = fopen(fichier, "w");          /* "w" : le fichier est vidé */
    if (f == NULL) return -1;
    fclose(f);
    return 1;
}

int exporter_historique(char *fichier_export, Historique tab[], int n)
{
    FILE *f = fopen(fichier_export, "w");
    int i;

    if (f == NULL) return -1;
    fputs("Date et heure;Utilisateur;Reservation;Action;Details\n", f);
    for (i = 0; i < n; i++)
        fprintf(f, "%s;%s;%s;%s;%s\n", tab[i].date, tab[i].utilisateur, tab[i].reference, tab[i].action, tab[i].details);
    fclose(f);
    return 1;
}

int filtrer_historique(Historique tab[], int n, Historique res[], char *texte, char *action)
{
    int i, k = 0;

    for (i = 0; i < n; i++) {
        if (*action && strcmp(tab[i].action, action) != 0) continue;
        if (contient(tab[i].date, texte) || contient(tab[i].utilisateur, texte) ||
            contient(tab[i].reference, texte) || contient(tab[i].details, texte))
            res[k++] = tab[i];
    }
    return k;
}

int compter_action(Historique tab[], int n, char *action)
{
    int i, c = 0;
    for (i = 0; i < n; i++)
        if (strcmp(tab[i].action, action) == 0) c++;
    return c;
}
