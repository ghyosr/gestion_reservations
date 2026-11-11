/* espace.c - Espace PASSAGER : le passager gère SES réservations (le compte contient son CIN).
 * Il peut consulter, payer, annuler et faire une nouvelle réservation. Il ne voit jamais celles des autres. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "app.h"

static GladeXML *xml_esp;
#define WE(id) glade_xml_get_widget(xml_esp, id)

static Reservation tab_e[MAX_RESERVATIONS], vue_e[MAX_RESERVATIONS];
static Paiement pai_e[MAX_PAIEMENTS];
static int nb_e = 0, nv_e = 0, npa_e = 0;
static char cle_e[12] = "";                  /* réservation sélectionnée */
static GtkListStore *liste_e;

static const char *classes_e[] = { "Économique", "Affaires", "Première" };
static const int   prix_classe[] = { 300, 700, 1200 };       /* prix d'une place selon la classe (TND) */
static const char *modes_e[] = { "Espèces", "Carte bancaire", "Virement", "Chèque" };
static const char *titres_e[] = { "Réservation", "Vol", "Date", "Classe", "Places", "Montant", "Statut" };

static Date aujourdhui(void)
{
    Date d;
    time_t t = time(NULL);
    struct tm *l = localtime(&t);
    d.jour = l->tm_mday; d.mois = l->tm_mon + 1; d.annee = l->tm_year + 1900;
    return d;
}

static void message(const char *texte) { gtk_label_set_text(GTK_LABEL(WE("lbl_u_msg")), texte); }

static void afficher_details_res(Reservation *r)
{
    char b[48];
    int paye, total = montant_total(*r);

    npa_e = charger_paiements(FICHIER_PAIEMENTS, pai_e, MAX_PAIEMENTS);
    if (npa_e < 0) npa_e = 0;
    paye = total_paye(pai_e, npa_e, r->id);
    gtk_label_set_text(GTK_LABEL(WE("u_val_id")), r->id);
    gtk_label_set_text(GTK_LABEL(WE("u_val_passager")), r->passager);
    gtk_label_set_text(GTK_LABEL(WE("u_val_cin")), r->cin);
    gtk_label_set_text(GTK_LABEL(WE("u_val_vol")), r->vol);
    snprintf(b, sizeof b, "%02d/%02d/%04d", r->date.jour, r->date.mois, r->date.annee);
    gtk_label_set_text(GTK_LABEL(WE("u_val_date")), b);
    gtk_label_set_text(GTK_LABEL(WE("u_val_classe")), r->classe);
    snprintf(b, sizeof b, "%d", r->places);          gtk_label_set_text(GTK_LABEL(WE("u_val_places")), b);
    snprintf(b, sizeof b, "%d TND", r->prix);        gtk_label_set_text(GTK_LABEL(WE("u_val_prix")), b);
    snprintf(b, sizeof b, "%d TND", total);          gtk_label_set_text(GTK_LABEL(WE("u_val_total")), b);
    gtk_label_set_text(GTK_LABEL(WE("u_val_statut")), r->statut);
    snprintf(b, sizeof b, "%d TND", paye);           gtk_label_set_text(GTK_LABEL(WE("u_val_paye")), b);
    snprintf(b, sizeof b, "%d TND", total - paye);   gtk_label_set_text(GTK_LABEL(WE("u_val_reste")), b);
    regler_progression(WE("bar_u_paiement"), paye, total, "TND payés");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(WE("u_montant")), total - paye > 0 ? total - paye : 1);
}

static void afficher_liste_e(void)
{
    int i, k;
    char texte[64], info[60], pl[16], mt[16];

    copier_texte(texte, sizeof texte, gtk_entry_get_text(GTK_ENTRY(WE("entry_u_recherche"))));
    enlever_espaces(texte);
    nv_e = filtrer_reservations(tab_e, nb_e, vue_e, texte, 0, "", "");
    k = 0;
    for (i = 0; i < nv_e; i++)                              /* le passager ne voit que SES réservations */
        if (strcmp(vue_e[i].cin, reference) == 0) vue_e[k++] = vue_e[i];
    nv_e = k;
    gtk_list_store_clear(liste_e);
    for (i = 0; i < nv_e; i++) {
        GtkTreeIter it;
        char date[16];
        snprintf(date, sizeof date, "%02d/%02d/%04d", vue_e[i].date.jour, vue_e[i].date.mois, vue_e[i].date.annee);
        snprintf(pl, sizeof pl, "%d", vue_e[i].places);
        snprintf(mt, sizeof mt, "%d TND", montant_total(vue_e[i]));
        gtk_list_store_append(liste_e, &it);
        gtk_list_store_set(liste_e, &it, 0, vue_e[i].id, 1, vue_e[i].vol, 2, date, 3, vue_e[i].classe, 4, pl, 5, mt, 6, vue_e[i].statut, -1);
    }
    snprintf(info, sizeof info, "%d réservation(s)", nv_e);
    gtk_label_set_text(GTK_LABEL(WE("lbl_u_nb")), info);
}

static void recharger_e(void)
{
    nb_e = charger_reservations(FICHIER_RESERVATIONS, tab_e, MAX_RESERVATIONS);
    if (nb_e < 0) nb_e = 0;
    afficher_liste_e();
}

void ouvrir_utilisateur(void)
{
    GtkWidget *fenetre;
    char b[80];

    cle_e[0] = '\0';
    xml_esp = glade_xml_new(FICHIER_GLADE, "window_user", NULL);
    glade_xml_signal_autoconnect(xml_esp);
    fenetre = WE("window_user");
    g_signal_connect(fenetre, "delete-event", G_CALLBACK(gtk_main_quit), NULL);
    remplir_combo(WE("combo_u_mode"), modes_e, 4, 0);
    remplir_combo(WE("combo_u_classe"), classes_e, 3, 0);
    liste_e = preparer_liste(WE("treeview_u"), 7, titres_e);
    snprintf(b, sizeof b, "Connecté : %s (CIN %s)", utilisateur, reference);
    gtk_label_set_text(GTK_LABEL(WE("lbl_user_user")), b);
    afficher_date_du_jour(WE("lbl_date_us"));
    recharger_e();
    afficher_statut(WE("statusbar_user"), "Espace passager.");
    gtk_widget_show_all(fenetre);
    if (nv_e > 0) {                                      /* on affiche tout de suite la première réservation */
        copier_texte(cle_e, sizeof cle_e, vue_e[0].id);
        afficher_details_res(&vue_e[0]);
    }
}

/* Plus grand numéro + 1, pour fabriquer R006, P005... */
static int numero_suivant_res(void)
{
    int i, m = 0;
    for (i = 0; i < nb_e; i++) if (atoi(tab_e[i].id + 1) > m) m = atoi(tab_e[i].id + 1);
    return m + 1;
}

static int numero_suivant_pai(void)
{
    int i, m = 0;
    npa_e = charger_paiements(FICHIER_PAIEMENTS, pai_e, MAX_PAIEMENTS);
    if (npa_e < 0) npa_e = 0;
    for (i = 0; i < npa_e; i++) if (atoi(pai_e[i].id + 1) > m) m = atoi(pai_e[i].id + 1);
    return m + 1;
}

/* ----------------------- gestionnaires de signaux ----------------------- */
void on_treeview_u_cursor_changed(GtkWidget *tv, gpointer d)
{
    char id[12];
    Reservation r;
    (void)d;
    if (!valeur_selectionnee(tv, 0, id, sizeof id)) return;
    r = chercher_reservation(FICHIER_RESERVATIONS, id);
    if (r.id[0]) { copier_texte(cle_e, sizeof cle_e, id); afficher_details_res(&r); message(""); }
}

void on_btn_u_rechercher_clicked(GtkWidget *w, gpointer d)
{ (void)w; (void)d; afficher_liste_e(); afficher_statut(WE("statusbar_user"), "Recherche effectuée."); }

void on_entry_u_recherche_activate(GtkWidget *w, gpointer d)
{ on_btn_u_rechercher_clicked(w, d); }

void on_btn_u_reinitialiser_clicked(GtkWidget *w, gpointer d)
{
    (void)w; (void)d;
    ecrire_entry(WE("entry_u_recherche"), "");
    afficher_liste_e();
}

void on_btn_u_annuler_clicked(GtkWidget *w, gpointer d)
{
    Reservation r;
    (void)w; (void)d;
    if (!cle_e[0]) { message("Sélectionnez d'abord une réservation dans la liste."); return; }
    r = chercher_reservation(FICHIER_RESERVATIONS, cle_e);
    if (strcmp(r.statut, "Annulée") == 0) { message("Cette réservation est déjà annulée."); return; }
    if (changer_statut_reservation(FICHIER_RESERVATIONS, cle_e, "Annulée") == 1) {
        historiser(cle_e, "Modification", "Annulée par le passager");
        recharger_e();
        r = chercher_reservation(FICHIER_RESERVATIONS, cle_e);
        afficher_details_res(&r);
        message("Réservation annulée (fichier reservations.txt mis à jour).");
    }
    else message("Annulation impossible.");
}

void on_btn_u_payer_clicked(GtkWidget *w, gpointer d)
{
    Paiement p;
    char *mode, detail[80];
    int erreur;
    Reservation r;
    (void)w; (void)d;
    if (!cle_e[0]) { message("Sélectionnez d'abord une réservation dans la liste."); return; }
    memset(&p, 0, sizeof p);
    snprintf(p.id, sizeof p.id, "P%03d", numero_suivant_pai());
    copier_texte(p.reservation, sizeof p.reservation, cle_e);
    p.date = aujourdhui();
    p.montant = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(WE("u_montant")));
    mode = texte_combo(WE("combo_u_mode"));
    copier_texte(p.mode, sizeof p.mode, mode);
    g_free(mode);
    copier_texte(p.remarque, sizeof p.remarque, "Paiement par le passager");
    erreur = valider_paiement(p);
    if (erreur == 0) erreur = controler_paiement(FICHIER_RESERVATIONS, FICHIER_PAIEMENTS, p, "");
    if (erreur != 0) { message(message_erreur_paiement(erreur)); return; }
    if (ajouter_paiement(FICHIER_PAIEMENTS, p) != 1) { message("Erreur d'accès au fichier paiements.txt."); return; }
    snprintf(detail, sizeof detail, "Paiement %s de %d TND (%s)", p.id, p.montant, p.mode);
    historiser(cle_e, "Paiement", detail);
    synchroniser_statut_reservation(FICHIER_RESERVATIONS, FICHIER_PAIEMENTS, cle_e);
    recharger_e();
    r = chercher_reservation(FICHIER_RESERVATIONS, cle_e);
    afficher_details_res(&r);
    message("Paiement enregistré. Merci !");
}

void on_btn_u_reserver_clicked(GtkWidget *w, gpointer d)
{
    Reservation r;
    char *classe;
    int i, erreur;
    (void)w; (void)d;
    memset(&r, 0, sizeof r);
    snprintf(r.id, sizeof r.id, "R%03d", numero_suivant_res());
    copier_texte(r.passager, sizeof r.passager, utilisateur);
    for (i = 0; i < nb_e; i++)                                   /* on reprend le nom déjà connu pour ce CIN */
        if (strcmp(tab_e[i].cin, reference) == 0) { copier_texte(r.passager, sizeof r.passager, tab_e[i].passager); break; }
    copier_texte(r.cin, sizeof r.cin, reference);
    copier_texte(r.vol, sizeof r.vol, gtk_entry_get_text(GTK_ENTRY(WE("u_vol"))));
    enlever_espaces(r.vol);
    r.date = aujourdhui();
    classe = texte_combo(WE("combo_u_classe"));
    copier_texte(r.classe, sizeof r.classe, classe);
    for (i = 0; i < 3; i++) if (strcmp(classes_e[i], classe) == 0) r.prix = prix_classe[i];
    g_free(classe);
    r.places = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(WE("u_places")));
    copier_texte(r.statut, sizeof r.statut, "En attente");
    erreur = valider_reservation(r);
    if (erreur) { message(message_erreur_reservation(erreur)); return; }
    if (ajouter_reservation(FICHIER_RESERVATIONS, r) != 1) { message("Réservation impossible."); return; }
    historiser(r.id, "Ajout", "Réservation faite par le passager");
    ecrire_entry(WE("u_vol"), "");
    recharger_e();
    message("Réservation enregistrée (statut « En attente » : payez-la pour la confirmer).");
}

void on_btn_deconnexion_user_clicked(GtkWidget *w, gpointer d)
{
    (void)w; (void)d;
    gtk_widget_destroy(WE("window_user"));
    ouvrir_login();
}
