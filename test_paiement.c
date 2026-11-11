/* test_paiement.c - Teste le module Paiement SANS aucun affichage.
 * Résultats à lire dans : test_paiement.txt, tri_paiement.txt, export_paiement.csv, rapport_paiement.txt.
 * Code retour 0 = tout est correct. */
#include <stdio.h>
#include <string.h>
#include "paiement.h"

static int total = 0, echecs = 0;
static FILE *rapport;
#define VERIF(cond) do { total++; if (!(cond)) { echecs++; fprintf(rapport, "ECHEC ligne %d : %s\n", __LINE__, #cond); } } while (0)

static Paiement faire(char *id, char *res, int j, int m, int a, int montant, char *mode, char *rem)
{
    Paiement p;
    memset(&p, 0, sizeof p);
    strcpy(p.id, id); strcpy(p.reservation, res);
    p.date.jour = j; p.date.mois = m; p.date.annee = a;
    p.montant = montant; strcpy(p.mode, mode); strcpy(p.remarque, rem);
    return p;
}

static Reservation faire_res(char *id, int places, int prix, char *statut)
{
    Reservation r;
    memset(&r, 0, sizeof r);
    strcpy(r.id, id); strcpy(r.passager, "Test Passager"); strcpy(r.cin, "12345678"); strcpy(r.vol, "CA101");
    r.date.jour = 1; r.date.mois = 11; r.date.annee = 2026;
    strcpy(r.classe, "Économique"); r.places = places; r.prix = prix; strcpy(r.statut, statut);
    return r;
}

int main(void)
{
    char *fic = "test_paiement.txt", *fres = "test_pai_reservations.txt";
    Paiement tab[MAX_PAIEMENTS], res[MAX_PAIEMENTS], x;
    Paiement p1 = faire("P001", "R001", 2, 11, 2026, 300, "Espèces", "Acompte");
    Paiement p2 = faire("P002", "R001", 5, 11, 2026, 540, "Carte bancaire", "");
    Paiement p3 = faire("P003", "R002", 1, 10, 2026, 100, "Virement", "Solde");
    Reservation r;
    int n, k;

    rapport = fopen("rapport_paiement.txt", "w");
    if (rapport == NULL) return 2;
    remove(fic); remove(fres);

    /* 1. validation */
    VERIF(valider_paiement(p1) == 0);
    x = p1; strcpy(x.id, "P");               VERIF(valider_paiement(x) == 1);
    x = p1; strcpy(x.reservation, "");       VERIF(valider_paiement(x) == 2);
    x = p1; x.date.mois = 13;                VERIF(valider_paiement(x) == 3);
    x = p1; x.montant = 0;                   VERIF(valider_paiement(x) == 4);
    x = p1; strcpy(x.mode, "Bitcoin");       VERIF(valider_paiement(x) == 5);

    /* 2. ajouter / charger */
    VERIF(ajouter_paiement(fic, p1) == 1);
    VERIF(ajouter_paiement(fic, p2) == 1);
    VERIF(ajouter_paiement(fic, p3) == 1);
    VERIF(ajouter_paiement(fic, p1) == 0);                    /* identifiant déjà utilisé */
    n = charger_paiements(fic, tab, MAX_PAIEMENTS);
    VERIF(n == 3);
    VERIF(charger_paiements("inexistant.txt", tab, MAX_PAIEMENTS) == -1);

    /* 3. chercher, modifier */
    x = chercher_paiement(fic, "P002");
    VERIF(x.montant == 540 && strcmp(x.mode, "Carte bancaire") == 0);
    x = chercher_paiement(fic, "P999");
    VERIF(x.id[0] == '\0');
    x = p3; x.montant = 150;
    VERIF(modifier_paiement(fic, "P003", x) == 1);
    x = chercher_paiement(fic, "P003");
    VERIF(x.montant == 150);
    VERIF(modifier_paiement(fic, "P999", p3) == 0);

    /* 4. filtrer, trier, compter, sommes */
    n = charger_paiements(fic, tab, MAX_PAIEMENTS);
    k = filtrer_paiements(tab, n, res, "", 0, "");                  VERIF(k == 3);
    k = filtrer_paiements(tab, n, res, "R001", 2, "");              VERIF(k == 2);
    k = filtrer_paiements(tab, n, res, "acompte", 3, "");           VERIF(k == 1);
    k = filtrer_paiements(tab, n, res, "", 0, "Virement");          VERIF(k == 1);
    trier_paiements(tab, n, 1);
    VERIF(strcmp(tab[0].id, "P003") == 0 && strcmp(tab[2].id, "P002") == 0);      /* du plus ancien au plus récent */
    VERIF(sauvegarder_paiements("tri_paiement.txt", tab, n) == 1);
    trier_paiements(tab, n, 0);
    VERIF(strcmp(tab[0].id, "P002") == 0);
    VERIF(total_paye(tab, n, "R001") == 840);
    VERIF(total_paye(tab, n, "R002") == 150);
    VERIF(total_paye(tab, n, "R999") == 0);
    VERIF(somme_paiements(tab, n) == 990);
    VERIF(compter_mode(tab, n, "Espèces") == 1);
    VERIF(compter_mode(tab, n, "Chèque") == 0);
    VERIF(exporter_paiements("export_paiement.csv", tab, n) == 1);

    /* 5. liens avec les réservations : R001 = 2 x 420 = 840, R002 = 1 x 900, R003 annulée */
    r = faire_res("R001", 2, 420, "En attente");   VERIF(ajouter_reservation(fres, r) == 1);
    r = faire_res("R002", 1, 900, "En attente");   VERIF(ajouter_reservation(fres, r) == 1);
    r = faire_res("R003", 1, 500, "Annulée");      VERIF(ajouter_reservation(fres, r) == 1);
    VERIF(reste_a_payer(fres, fic, "R001") == 0);
    VERIF(reste_a_payer(fres, fic, "R002") == 750);
    VERIF(reste_a_payer(fres, fic, "R999") == -1);
    VERIF(controler_paiement(fres, fic, faire("P010", "R002", 1, 11, 2026, 750, "Espèces", ""), "") == 0);
    VERIF(controler_paiement(fres, fic, faire("P010", "R002", 1, 11, 2026, 751, "Espèces", ""), "") == 13);
    VERIF(controler_paiement(fres, fic, faire("P003", "R002", 1, 11, 2026, 900, "Espèces", ""), "P003") == 0);   /* modification : on exclut P003 */
    VERIF(controler_paiement(fres, fic, faire("P010", "R999", 1, 11, 2026, 10, "Espèces", ""), "") == 11);
    VERIF(controler_paiement(fres, fic, faire("P010", "R003", 1, 11, 2026, 10, "Espèces", ""), "") == 12);

    /* 6. le statut suit les paiements */
    VERIF(synchroniser_statut_reservation(fres, fic, "R001") == 1);   /* entièrement payée : devient Confirmée */
    r = chercher_reservation(fres, "R001");
    VERIF(strcmp(r.statut, "Confirmée") == 0);
    VERIF(synchroniser_statut_reservation(fres, fic, "R001") == 0);   /* déjà fait */
    VERIF(synchroniser_statut_reservation(fres, fic, "R002") == 0);   /* paiement partiel : reste en attente */
    r = chercher_reservation(fres, "R002");
    VERIF(strcmp(r.statut, "En attente") == 0);
    VERIF(synchroniser_statut_reservation(fres, fic, "R999") == 0);
    VERIF(synchroniser_statut_reservation(fres, fic, "R003") == 0);   /* annulée : inchangée */

    /* 7. supprimer */
    VERIF(supprimer_paiement(fic, "P003") == 1);
    VERIF(supprimer_paiement(fic, "P003") == 0);
    VERIF(supprimer_paiements_reservation(fic, "R001") == 2);
    VERIF(supprimer_paiements_reservation(fic, "R001") == 0);
    n = charger_paiements(fic, tab, MAX_PAIEMENTS);
    VERIF(n == 0);

    fprintf(rapport, "%d verifications, %d echec(s)\n", total, echecs);
    fclose(rapport);
    return echecs == 0 ? 0 : 1;
}
