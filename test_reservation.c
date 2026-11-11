/* test_reservation.c - Teste le module Reservation SANS aucun affichage.
 * Les résultats se lisent directement dans les fichiers texte :
 *   test_reservation.txt    : fichier de données après les opérations
 *   tri_reservation.txt     : fichier trié par nom de passager
 *   rapport_reservation.txt : liste des vérifications (ECHEC ... ou "0 echec")
 * Code retour 0 = tout est correct. */
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "reservation.h"

static int total = 0, echecs = 0;
static FILE *rapport;

#define VERIF(cond) do { total++; if (!(cond)) { echecs++; fprintf(rapport, "ECHEC ligne %d : %s\n", __LINE__, #cond); } } while (0)

static Reservation faire(char *id, char *passager, char *cin, char *vol, int j, int m, int a,
                         char *classe, int places, int prix, char *statut)
{
    Reservation x;
    memset(&x, 0, sizeof x);
    strcpy(x.id, id); strcpy(x.passager, passager); strcpy(x.cin, cin); strcpy(x.vol, vol);
    x.date.jour = j; x.date.mois = m; x.date.annee = a;
    strcpy(x.classe, classe); x.places = places; x.prix = prix; strcpy(x.statut, statut);
    return x;
}

int main(void)
{
    char *fic = "test_reservation.txt";
    Reservation tab[MAX_RESERVATIONS], res[MAX_RESERVATIONS], x;
    Reservation r1 = faire("R001", "Trabelsi Sami", "09123456", "CA101", 2, 11, 2026, "Économique", 2, 420, "Confirmée");
    Reservation r2 = faire("R002", "Gharbi Leila", "08234567", "CA101", 3, 11, 2026, "Affaires", 1, 900, "Confirmée");
    Reservation r3 = faire("R003", "Dupont Marc", "12345678", "CA102", 4, 11, 2026, "Économique", 3, 410, "En attente");
    int n, k, i;

    rapport = fopen("rapport_reservation.txt", "w");
    if (rapport == NULL) return 2;
    remove(fic);

    /* 1. validation */
    VERIF(valider_reservation(r1) == 0);
    x = r1; strcpy(x.id, "");                   VERIF(valider_reservation(x) == 1);
    x = r1; strcpy(x.passager, "");             VERIF(valider_reservation(x) == 2);
    x = r1; strcpy(x.passager, "Sami123");      VERIF(valider_reservation(x) == 2);
    x = r1; strcpy(x.cin, "1234");              VERIF(valider_reservation(x) == 3);
    x = r1; strcpy(x.vol, "C");                 VERIF(valider_reservation(x) == 4);
    x = r1; strcpy(x.classe, "");               VERIF(valider_reservation(x) == 5);
    x = r1; x.places = 0;                       VERIF(valider_reservation(x) == 6);
    x = r1; x.prix = -5;                        VERIF(valider_reservation(x) == 7);
    x = r1; x.date.jour = 31; x.date.mois = 4;  VERIF(valider_reservation(x) == 8);
    x = r1; strcpy(x.statut, "Inconnu");        VERIF(valider_reservation(x) == 9);
    VERIF(montant_total(r1) == 840);

    /* 2. ajouter (le fichier test_reservation.txt est créé) */
    VERIF(ajouter_reservation(fic, r1) == 1);
    VERIF(ajouter_reservation(fic, r2) == 1);
    VERIF(ajouter_reservation(fic, r3) == 1);
    VERIF(ajouter_reservation(fic, r1) == 0);              /* même identifiant */
    n = charger_reservations(fic, tab, MAX_RESERVATIONS);
    VERIF(n == 3);

    /* 3. chercher */
    x = chercher_reservation(fic, "R002");
    VERIF(strcmp(x.passager, "Gharbi Leila") == 0 && x.places == 1);
    x = chercher_reservation(fic, "XXXX");
    VERIF(x.id[0] == '\0');

    /* 4. modifier */
    x = r3; strcpy(x.statut, "Confirmée"); x.places = 2;
    VERIF(modifier_reservation(fic, "R003", x) == 1);
    x = chercher_reservation(fic, "R003");
    VERIF(strcmp(x.statut, "Confirmée") == 0 && x.places == 2);
    VERIF(modifier_reservation(fic, "XXXX", r1) == 0);

    /* 5. filtrer */
    n = charger_reservations(fic, tab, MAX_RESERVATIONS);
    k = filtrer_reservations(tab, n, res, "", 0, "", "");                  VERIF(k == 3);
    k = filtrer_reservations(tab, n, res, "ca101", 0, "", "");             VERIF(k == 2);
    k = filtrer_reservations(tab, n, res, "ca101", 4, "", "");             VERIF(k == 2);
    k = filtrer_reservations(tab, n, res, "ca101", 2, "", "");             VERIF(k == 0);
    k = filtrer_reservations(tab, n, res, "", 0, "Affaires", "");          VERIF(k == 1);
    k = filtrer_reservations(tab, n, res, "", 0, "", "Confirmée");         VERIF(k == 3);

    /* 6. trier par passager, puis compter */
    trier_reservations(tab, n, 1);
    for (i = 1; i < n; i++) VERIF(strcasecmp(tab[i - 1].passager, tab[i].passager) <= 0);
    VERIF(sauvegarder_reservations("tri_reservation.txt", tab, n) == 1);
    trier_reservations(tab, n, 0);
    for (i = 1; i < n; i++) VERIF(strcasecmp(tab[i - 1].passager, tab[i].passager) >= 0);
    VERIF(compter_statut(tab, n, "Confirmée") == 3);
    VERIF(compter_statut(tab, n, "Annulée") == 0);

    /* 7. supprimer */
    VERIF(supprimer_reservation(fic, "R003") == 1);
    VERIF(supprimer_reservation(fic, "R003") == 0);
    n = charger_reservations(fic, tab, MAX_RESERVATIONS);
    VERIF(n == 2);

    /* 8. changer le statut seul, exporter */
    VERIF(changer_statut_reservation(fic, "R001", "Annulée") == 1);
    x = chercher_reservation(fic, "R001");
    VERIF(strcmp(x.statut, "Annulée") == 0 && x.places == 2);
    VERIF(changer_statut_reservation(fic, "R999", "Annulée") == 0);
    n = charger_reservations(fic, tab, MAX_RESERVATIONS);
    VERIF(exporter_reservations("export_reservation.csv", tab, n) == 1);

    /* 9. fichier absent */
    VERIF(charger_reservations("fichier_inexistant.txt", tab, MAX_RESERVATIONS) == -1);

    fprintf(rapport, "%d verifications, %d echec(s)\n", total, echecs);
    fclose(rapport);
    return echecs == 0 ? 0 : 1;
}
