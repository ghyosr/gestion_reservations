/* paiement_ui.c - Onglet « Paiements » de la gestion des réservations.
 * Toute la logique (fichier, recherche, tri, contrôle) est dans paiement.c. */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "app.h"

static Paiement tab[MAX_PAIEMENTS];      /* tous les paiements du fichier */
static Paiement vue[MAX_PAIEMENTS];      /* paiements affichés            */
static int nbp = 0, nvp = 0;
static int tri_croissant = 1;
static int pret_p = 0;
static char id_choisi[12] = "";          /* identifiant de la ligne sélectionnée */
static GtkListStore *liste_p;

static const char *choix_mode[]  = { "Espèces", "Carte bancaire", "Virement", "Chèque" };
static const char *choix_crit[]  = { "Tous les champs", "Identifiant", "Réservation", "Remarque" };
static const char *filtre_mode[] = { "Tous", "Espèces", "Carte bancaire", "Virement", "Chèque" };
static const char *titres_p[] = { "Identifiant", "Réservation", "Date", "Montant (TND)", "Mode", "Remarque" };

/* ------------------------------------------------------------------ */
/*  Situation de la réservation choisie : total, déjà payé, reste      */
/* ------------------------------------------------------------------ */
static void maj_situation(void)
{
    char *id = texte_combo(WR("combo_p_reservation"));
    Reservation r = chercher_reservation(FICHIER_RESERVATIONS, id);
    char b[40];

    if (r.id[0] == '\0') {
        gtk_label_set_text(GTK_LABEL(WR("lbl_p_total")), "--");
        gtk_label_set_text(GTK_LABEL(WR("lbl_p_paye")), "--");
        gtk_label_set_text(GTK_LABEL(WR("lbl_p_reste")), "--");
    } else {
        int paye = total_paye(tab, nbp, id);
        snprintf(b, sizeof b, "%d TND", montant_total(r));
        gtk_label_set_text(GTK_LABEL(WR("lbl_p_total")), b);
        snprintf(b, sizeof b, "%d TND", paye);
        gtk_label_set_text(GTK_LABEL(WR("lbl_p_paye")), b);
        snprintf(b, sizeof b, "%d TND", montant_total(r) - paye);
        gtk_label_set_text(GTK_LABEL(WR("lbl_p_reste")), b);
    }
    g_free(id);
}

/* Liste des réservations proposée dans le formulaire */
void remplir_combo_reservations(void)
{
    static Reservation rs[MAX_RESERVATIONS];
    static char noms[MAX_RESERVATIONS][12];
    static const char *ptr[MAX_RESERVATIONS];
    int i, n = charger_reservations(FICHIER_RESERVATIONS, rs, MAX_RESERVATIONS);
    char *ancien = texte_combo(WR("combo_p_reservation"));

    if (n < 0) n = 0;
    for (i = 0; i < n; i++) {
        copier_texte(noms[i], sizeof noms[i], rs[i].id);
        ptr[i] = noms[i];
    }
    remplir_combo(WR("combo_p_reservation"), ptr, n, -1);
    choisir_combo(WR("combo_p_reservation"), ancien);      /* on garde le choix en cours s'il existe encore */
    g_free(ancien);
    maj_situation();
}

/* ------------------------------------------------------------------ */
/*  Affichage de la liste                                              */
/* ------------------------------------------------------------------ */
static void afficher_liste_p(void)
{
    int i, critere;
    char texte[64], info[80], date[16], montant[16];
    char *mode = texte_combo(WR("combo_p_fmode"));

    critere = gtk_combo_box_get_active(GTK_COMBO_BOX(WR("combo_p_critere")));
    if (critere < 0) critere = 0;
    if (strcmp(mode, "Tous") == 0) mode[0] = '\0';
    copier_texte(texte, sizeof texte, gtk_entry_get_text(GTK_ENTRY(WR("entry_p_recherche"))));
    enlever_espaces(texte);

    nvp = filtrer_paiements(tab, nbp, vue, texte, critere, mode);
    trier_paiements(vue, nvp, tri_croissant);

    gtk_list_store_clear(liste_p);
    for (i = 0; i < nvp; i++) {
        GtkTreeIter it;
        snprintf(date, sizeof date, "%02d/%02d/%04d", vue[i].date.jour, vue[i].date.mois, vue[i].date.annee);
        snprintf(montant, sizeof montant, "%d", vue[i].montant);
        gtk_list_store_append(liste_p, &it);
        gtk_list_store_set(liste_p, &it, 0, vue[i].id, 1, vue[i].reservation, 2, date, 3, montant,
                           4, vue[i].mode, 5, vue[i].remarque, -1);
    }
    snprintf(info, sizeof info, "%d paiements affichés", nvp);
    gtk_label_set_text(GTK_LABEL(WR("lbl_p_nb")), info);
    snprintf(info, sizeof info, "Total affiché : %d TND", somme_paiements(vue, nvp));
    gtk_label_set_text(GTK_LABEL(WR("lbl_p_somme")), info);
    g_free(mode);
}

void recharger_paiements(void)
{
    nbp = charger_paiements(FICHIER_PAIEMENTS, tab, MAX_PAIEMENTS);
    if (nbp < 0) nbp = 0;
    if (pret_p) {
        afficher_liste_p();
        maj_situation();
    }
}

/* ------------------------------------------------------------------ */
/*  Formulaire                                                         */
/* ------------------------------------------------------------------ */
static void regler_date(Date d)
{
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(WR("p_djour")), d.jour);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(WR("p_dmois")), d.mois);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(WR("p_dannee")), d.annee);
}

static Paiement lire_formulaire(void)
{
    Paiement p;
    char *t;
    int i;

    memset(&p, 0, sizeof p);
    copier_texte(p.id, sizeof p.id, gtk_entry_get_text(GTK_ENTRY(WR("p_id"))));
    enlever_espaces(p.id);
    for (i = 0; p.id[i]; i++) p.id[i] = toupper((unsigned char)p.id[i]);
    t = texte_combo(WR("combo_p_reservation"));
    copier_texte(p.reservation, sizeof p.reservation, t);
    g_free(t);
    p.date.jour = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(WR("p_djour")));
    p.date.mois = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(WR("p_dmois")));
    p.date.annee = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(WR("p_dannee")));
    p.montant = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(WR("p_montant")));
    t = texte_combo(WR("combo_p_mode"));
    copier_texte(p.mode, sizeof p.mode, t);
    g_free(t);
    copier_texte(p.remarque, sizeof p.remarque, gtk_entry_get_text(GTK_ENTRY(WR("p_remarque"))));
    enlever_espaces(p.remarque);
    return p;
}

static void vider_formulaire(void)
{
    time_t maintenant = time(NULL);
    struct tm *t = localtime(&maintenant);
    Date d;

    d.jour = t->tm_mday;                      /* par défaut : la date du jour */
    d.mois = t->tm_mon + 1;
    d.annee = t->tm_year + 1900;

    id_choisi[0] = '\0';
    ecrire_entry(WR("p_id"), "");
    gtk_editable_set_editable(GTK_EDITABLE(WR("p_id")), TRUE);
    gtk_combo_box_set_active(GTK_COMBO_BOX(WR("combo_p_reservation")), -1);
    gtk_combo_box_set_active(GTK_COMBO_BOX(WR("combo_p_mode")), -1);
    regler_date(d);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(WR("p_montant")), 1);
    ecrire_entry(WR("p_remarque"), "");
    gtk_label_set_text(GTK_LABEL(WR("lbl_msg_pai")), "");
    maj_situation();
}

static void remplir_formulaire(Paiement p)
{
    copier_texte(id_choisi, sizeof id_choisi, p.id);
    ecrire_entry(WR("p_id"), p.id);
    gtk_editable_set_editable(GTK_EDITABLE(WR("p_id")), FALSE);          /* l'identifiant ne se modifie pas */
    choisir_combo(WR("combo_p_reservation"), p.reservation);
    choisir_combo(WR("combo_p_mode"), p.mode);
    regler_date(p.date);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(WR("p_montant")), p.montant);
    ecrire_entry(WR("p_remarque"), p.remarque);
    maj_situation();
}

/* Si la réservation est entièrement payée, son statut passe à « Confirmée » (et on le note dans l'historique). */
static void mettre_a_jour_reservation(char *reservation)
{
    if (synchroniser_statut_reservation(FICHIER_RESERVATIONS, FICHIER_PAIEMENTS, reservation) == 1)
        historiser(reservation, "Modification", "Statut : Confirmée (réservation entièrement payée)");
}

void init_paiements(void)
{
    pret_p = 0;
    remplir_combo(WR("combo_p_mode"), choix_mode, 4, -1);
    remplir_combo(WR("combo_p_critere"), choix_crit, 4, 0);
    remplir_combo(WR("combo_p_fmode"), filtre_mode, 5, 0);
    liste_p = preparer_liste(WR("treeview_paiements"), 6, titres_p);
    tri_croissant = 1;
    recharger_paiements();
    afficher_liste_p();
    vider_formulaire();
    pret_p = 1;
}

/* Bouton « Paiements de la réservation » de l'onglet Réservations. */
void voir_paiements_reservation(const char *id)
{
    pret_p = 0;
    ecrire_entry(WR("entry_p_recherche"), id);
    gtk_combo_box_set_active(GTK_COMBO_BOX(WR("combo_p_critere")), 2);
    gtk_combo_box_set_active(GTK_COMBO_BOX(WR("combo_p_fmode")), 0);
    pret_p = 1;
    afficher_liste_p();
    gtk_notebook_set_current_page(GTK_NOTEBOOK(WR("notebook")), 1);
}

/* ----------------------- gestionnaires de signaux ----------------------- */
void on_combo_p_reservation_changed(GtkWidget *w, gpointer data)
{ (void)w; (void)data; if (pret_p) maj_situation(); }

void on_btn_p_ajouter_clicked(GtkWidget *w, gpointer data)
{
    Paiement p;
    int erreur, r;
    char detail[80];
    (void)w; (void)data;

    p = lire_formulaire();
    erreur = valider_paiement(p);
    if (erreur == 0) erreur = controler_paiement(FICHIER_RESERVATIONS, FICHIER_PAIEMENTS, p, "");
    if (erreur != 0) {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_pai")), message_erreur_paiement(erreur));
        return;
    }
    r = ajouter_paiement(FICHIER_PAIEMENTS, p);
    if (r == 1) {
        snprintf(detail, sizeof detail, "Paiement %s de %d TND (%s)", p.id, p.montant, p.mode);
        historiser(p.reservation, "Paiement", detail);
        mettre_a_jour_reservation(p.reservation);
        recharger_paiements();
        recharger_reservations();
        vider_formulaire();
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_pai")), "Paiement ajouté : le fichier paiements.txt a été mis à jour.");
    } else if (r == 0) {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_pai")), "Cet identifiant existe déjà.");
    } else {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_pai")), "Erreur d'accès au fichier paiements.txt.");
    }
}

void on_btn_p_modifier_clicked(GtkWidget *w, gpointer data)
{
    Paiement p;
    int erreur, r;
    char detail[80];
    (void)w; (void)data;

    if (id_choisi[0] == '\0') {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_pai")), "Sélectionnez d'abord un paiement dans la liste.");
        return;
    }
    p = lire_formulaire();
    erreur = valider_paiement(p);
    if (erreur == 0) erreur = controler_paiement(FICHIER_RESERVATIONS, FICHIER_PAIEMENTS, p, id_choisi);
    if (erreur != 0) {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_pai")), message_erreur_paiement(erreur));
        return;
    }
    r = modifier_paiement(FICHIER_PAIEMENTS, id_choisi, p);
    if (r == 1) {
        snprintf(detail, sizeof detail, "Paiement %s modifié (%d TND, %s)", id_choisi, p.montant, p.mode);
        historiser(p.reservation, "Paiement", detail);
        mettre_a_jour_reservation(p.reservation);
        recharger_paiements();
        recharger_reservations();
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_pai")), "Modification enregistrée dans le fichier.");
    } else if (r == 0) {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_pai")), "Paiement introuvable.");
    } else {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_pai")), "Erreur d'accès au fichier paiements.txt.");
    }
}

void on_btn_p_supprimer_clicked(GtkWidget *w, gpointer data)
{
    GtkWidget *dialogue;
    Paiement ancien;
    int reponse;
    char detail[80];
    (void)w; (void)data;

    if (id_choisi[0] == '\0') {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_pai")), "Sélectionnez d'abord un paiement dans la liste.");
        return;
    }
    dialogue = gtk_message_dialog_new(GTK_WINDOW(WR("window_reservations")), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
                                      GTK_BUTTONS_YES_NO, "Supprimer le paiement %s ?", id_choisi);
    reponse = gtk_dialog_run(GTK_DIALOG(dialogue));
    gtk_widget_destroy(dialogue);
    if (reponse != GTK_RESPONSE_YES) return;

    ancien = chercher_paiement(FICHIER_PAIEMENTS, id_choisi);
    if (supprimer_paiement(FICHIER_PAIEMENTS, id_choisi) == 1) {
        snprintf(detail, sizeof detail, "Paiement %s supprimé", id_choisi);
        historiser(ancien.reservation, "Paiement", detail);
        recharger_paiements();
        recharger_reservations();
        vider_formulaire();
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_pai")), "Paiement supprimé du fichier.");
    } else {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_pai")), "Suppression impossible.");
    }
}

void on_btn_p_vider_clicked(GtkWidget *w, gpointer data)
{ (void)w; (void)data; vider_formulaire(); }

void on_treeview_paiements_cursor_changed(GtkWidget *tv, gpointer data)
{
    char id[12];
    Paiement p;
    (void)data;

    if (!valeur_selectionnee(tv, 0, id, sizeof id)) return;
    p = chercher_paiement(FICHIER_PAIEMENTS, id);
    if (p.id[0] != '\0') {
        remplir_formulaire(p);
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_pai")), "");
    }
}

void on_btn_p_rechercher_clicked(GtkWidget *w, gpointer data)
{ (void)w; (void)data; afficher_liste_p(); }

void on_entry_p_recherche_activate(GtkWidget *w, gpointer data)
{ on_btn_p_rechercher_clicked(w, data); }

void on_btn_p_reinitialiser_clicked(GtkWidget *w, gpointer data)
{
    (void)w; (void)data;
    pret_p = 0;
    ecrire_entry(WR("entry_p_recherche"), "");
    gtk_combo_box_set_active(GTK_COMBO_BOX(WR("combo_p_critere")), 0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(WR("combo_p_fmode")), 0);
    pret_p = 1;
    afficher_liste_p();
}

void on_btn_p_exporter_clicked(GtkWidget *w, gpointer data)
{
    (void)w; (void)data;
    if (exporter_paiements("export_paiements.csv", vue, nvp) == 1) {
        historiser("", "Export", "Paiements exportés : export_paiements.csv");
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_pai")), "Liste exportée dans le fichier export_paiements.csv.");
    } else {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_pai")), "Export impossible.");
    }
}

void on_rb_p_asc_toggled(GtkWidget *w, gpointer data)
{
    (void)data;
    if (!pret_p || !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
    tri_croissant = 1;
    afficher_liste_p();
}

void on_rb_p_desc_toggled(GtkWidget *w, gpointer data)
{
    (void)data;
    if (!pret_p || !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
    tri_croissant = 0;
    afficher_liste_p();
}

void on_combo_p_fmode_changed(GtkWidget *w, gpointer data)
{ (void)w; (void)data; if (pret_p) afficher_liste_p(); }
