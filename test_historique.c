/* test_historique.c - Teste le module Historique SANS aucun affichage.
 * Résultats à lire dans : test_historique.txt, export_historique.csv, rapport_historique.txt.
 * Code retour 0 = tout est correct. */
#include <stdio.h>
#include <string.h>
#include "historique.h"

static int total = 0, echecs = 0;
static FILE *rapport;
#define VERIF(cond) do { total++; if (!(cond)) { echecs++; fprintf(rapport, "ECHEC ligne %d : %s\n", __LINE__, #cond); } } while (0)

int main(void)
{
    char *fic = "test_historique.txt";
    Historique tab[MAX_HIST], res[MAX_HIST], h;
    int n, k;

    rapport = fopen("rapport_historique.txt", "w");
    if (rapport == NULL) return 2;
    remove(fic);

    VERIF(charger_historique(fic, tab, MAX_HIST) == -1);                    /* pas encore de journal */
    VERIF(ajouter_historique(fic, "admin", "R001", "Ajout", "Réservation ajoutée") == 1);
    VERIF(ajouter_historique(fic, "admin", "R001", "Modification", "Places modifiées") == 1);
    VERIF(ajouter_historique(fic, "admin", "R002", "Paiement", "Paiement P001 reçu; solde à régler") == 1);
    memset(&h, 0, sizeof h);
    strcpy(h.date, "01/01/2026 10:00:00"); strcpy(h.utilisateur, "admin"); strcpy(h.reference, "R003");
    strcpy(h.action, "Suppression"); strcpy(h.details, "Réservation supprimée");
    VERIF(enregistrer_historique(fic, h) == 1);

    n = charger_historique(fic, tab, MAX_HIST);
    VERIF(n == 4);
    VERIF(strlen(tab[0].date) == 19 && tab[0].date[2] == '/' && tab[0].date[13] == ':');   /* date du jour bien formée */
    VERIF(strcmp(tab[3].date, "01/01/2026 10:00:00") == 0 && strcmp(tab[3].action, "Suppression") == 0);
    VERIF(strchr(tab[2].details, ';') == NULL);                              /* le ';' tapé est remplacé */

    k = filtrer_historique(tab, n, res, "", "");                     VERIF(k == 4);
    k = filtrer_historique(tab, n, res, "R001", "");               VERIF(k == 2);
    k = filtrer_historique(tab, n, res, "r001", "Ajout");          VERIF(k == 1);
    k = filtrer_historique(tab, n, res, "", "Paiement");          VERIF(k == 1);
    k = filtrer_historique(tab, n, res, "places", "");             VERIF(k == 1);
    k = filtrer_historique(tab, n, res, "introuvable", "");          VERIF(k == 0);
    VERIF(compter_action(tab, n, "Ajout") == 1);
    VERIF(compter_action(tab, n, "Export") == 0);
    VERIF(exporter_historique("export_historique.csv", tab, n) == 1);

    VERIF(vider_historique(fic) == 1);
    VERIF(charger_historique(fic, tab, MAX_HIST) == 0);

    fprintf(rapport, "%d verifications, %d echec(s)\n", total, echecs);
    fclose(rapport);
    return echecs == 0 ? 0 : 1;
}
