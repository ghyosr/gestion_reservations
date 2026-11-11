/* gestion_reservations.c - Fenêtre « Gestion des réservations » (administrateur) : onglet Réservations + Statistiques.
 * Les onglets Paiements et Historique sont dans paiement_ui.c et historique_ui.c.
 * Toute la logique (fichier, recherche, tri, contrôle) est dans reservation.c :
 * ici on s'occupe seulement de l'interface. */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "app.h"

GladeXML *xml_res;                    /* partagé avec paiement_ui.c et historique_ui.c */

static Reservation tab[MAX_RESERVATIONS];   /* toutes les réservations du fichier   */
static Reservation vue[MAX_RESERVATIONS];   /* réservations affichées dans la liste */
static int nb = 0, nv = 0;
static int tri_croissant = 1;
static int pret = 0;                        /* 1 quand la fenêtre est entièrement initialisée */
static char id_choisi[12] = "";             /* identifiant de la ligne sélectionnée */
static GtkListStore *liste;

static const char *choix_classe[]  = { "Économique", "Affaires", "Première" };
static const char *choix_crit[]    = { "Tous les champs", "Identifiant", "Passager", "CIN", "Vol" };
static const char *filtre_classe[] = { "Toutes", "Économique", "Affaires", "Première" };
static const char *filtre_statut[] = { "Tous", "Confirmée", "En attente", "Annulée" };
static const char *titres[] = { "Id", "Passager", "CIN", "Vol", "Date", "Classe",
                                "Places", "Prix", "Total", "Payé", "Reste", "Statut" };

/* ------------------------------------------------------------------ */
/*  Affichage de la liste et des statistiques                          */
/* ------------------------------------------------------------------ */
static void afficher_liste(void)
{
    int i, k, critere;
    char texte[64], info[80], pl[16], px[16], mt[16], date[16], paye[16], reste[16];
    static Paiement pa[MAX_PAIEMENTS];
    int np = charger_paiements(FICHIER_PAIEMENTS, pa, MAX_PAIEMENTS);
    char *classe = texte_combo(WR("combo_filtre_classe"));
    char *statut = texte_combo(WR("combo_filtre_statut"));

    critere = gtk_combo_box_get_active(GTK_COMBO_BOX(WR("combo_critere")));
    if (critere < 0) critere = 0;
    if (strcmp(classe, "Toutes") == 0) classe[0] = '\0';
    if (strcmp(statut, "Tous") == 0) statut[0] = '\0';
    copier_texte(texte, sizeof texte, gtk_entry_get_text(GTK_ENTRY(WR("entry_recherche"))));
    enlever_espaces(texte);

    if (np < 0) np = 0;
    nv = filtrer_reservations(tab, nb, vue, texte, critere, classe, statut);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(WR("chk_attente")))) {
        k = 0;                                    /* on garde seulement les réservations en attente */
        for (i = 0; i < nv; i++)
            if (strcmp(vue[i].statut, "En attente") == 0) vue[k++] = vue[i];
        nv = k;
    }
    trier_reservations(vue, nv, tri_croissant);

    gtk_list_store_clear(liste);
    for (i = 0; i < nv; i++) {
        GtkTreeIter it;
        snprintf(date, sizeof date, "%02d/%02d/%04d", vue[i].date.jour, vue[i].date.mois, vue[i].date.annee);
        snprintf(pl, sizeof pl, "%d", vue[i].places);
        snprintf(px, sizeof px, "%d", vue[i].prix);
        snprintf(mt, sizeof mt, "%d", montant_total(vue[i]));
        snprintf(paye, sizeof paye, "%d", total_paye(pa, np, vue[i].id));
        snprintf(reste, sizeof reste, "%d", strcmp(vue[i].statut, "Annulée") == 0 ? 0 : montant_total(vue[i]) - total_paye(pa, np, vue[i].id));
        gtk_list_store_append(liste, &it);
        gtk_list_store_set(liste, &it, 0, vue[i].id, 1, vue[i].passager, 2, vue[i].cin, 3, vue[i].vol,
                           4, date, 5, vue[i].classe, 6, pl, 7, px, 8, mt, 9, paye, 10, reste, 11, vue[i].statut, -1);
    }
    snprintf(info, sizeof info, "%d réservations affichées", nv);
    gtk_label_set_text(GTK_LABEL(WR("lbl_nb")), info);
    g_free(classe);
    g_free(statut);
}

void afficher_statistiques(void)
{
    static Paiement pa[MAX_PAIEMENTS];
    static Historique hs[MAX_HIST];
    int i, places = 0, ca = 0, reste = 0, np, nh;
    char b[40];

    np = charger_paiements(FICHIER_PAIEMENTS, pa, MAX_PAIEMENTS);
    nh = charger_historique(FICHIER_HIST, hs, MAX_HIST);
    if (np < 0) np = 0;
    if (nh < 0) nh = 0;
    for (i = 0; i < nb; i++)
        if (strcmp(tab[i].statut, "Annulée") != 0) reste += montant_total(tab[i]) - total_paye(pa, np, tab[i].id);

    for (i = 0; i < nb; i++)
        if (strcmp(tab[i].statut, "Annulée") != 0) places += tab[i].places;
    for (i = 0; i < nb; i++)
        if (strcmp(tab[i].statut, "Confirmée") == 0) ca += montant_total(tab[i]);
    snprintf(b, sizeof b, "%d", nb);                                  gtk_label_set_text(GTK_LABEL(WR("lbl_st_total")), b);
    snprintf(b, sizeof b, "%d", compter_statut(tab, nb, "Confirmée")); gtk_label_set_text(GTK_LABEL(WR("lbl_st_conf")), b);
    snprintf(b, sizeof b, "%d", compter_statut(tab, nb, "En attente")); gtk_label_set_text(GTK_LABEL(WR("lbl_st_att")), b);
    snprintf(b, sizeof b, "%d", compter_statut(tab, nb, "Annulée"));   gtk_label_set_text(GTK_LABEL(WR("lbl_st_ann")), b);
    snprintf(b, sizeof b, "%d", places);                              gtk_label_set_text(GTK_LABEL(WR("lbl_st_places")), b);
    snprintf(b, sizeof b, "%d TND", ca);                              gtk_label_set_text(GTK_LABEL(WR("lbl_st_ca")), b);
    snprintf(b, sizeof b, "%d", np);                                  gtk_label_set_text(GTK_LABEL(WR("lbl_st_ptot")), b);
    snprintf(b, sizeof b, "%d TND", somme_paiements(pa, np));         gtk_label_set_text(GTK_LABEL(WR("lbl_st_enc")), b);
    snprintf(b, sizeof b, "%d TND", reste);                           gtk_label_set_text(GTK_LABEL(WR("lbl_st_reste")), b);
    snprintf(b, sizeof b, "%d", compter_mode(pa, np, "Espèces"));        gtk_label_set_text(GTK_LABEL(WR("lbl_st_mesp")), b);
    snprintf(b, sizeof b, "%d", compter_mode(pa, np, "Carte bancaire")); gtk_label_set_text(GTK_LABEL(WR("lbl_st_mcarte")), b);
    snprintf(b, sizeof b, "%d", compter_mode(pa, np, "Virement"));       gtk_label_set_text(GTK_LABEL(WR("lbl_st_mvir")), b);
    snprintf(b, sizeof b, "%d", compter_mode(pa, np, "Chèque"));         gtk_label_set_text(GTK_LABEL(WR("lbl_st_mchq")), b);
    snprintf(b, sizeof b, "%d", nh);                                  gtk_label_set_text(GTK_LABEL(WR("lbl_st_hist")), b);
}

/* Relit le fichier texte puis met tout à jour. */
void recharger_reservations(void)
{
    nb = charger_reservations(FICHIER_RESERVATIONS, tab, MAX_RESERVATIONS);
    if (nb < 0) nb = 0;
    afficher_liste();
    afficher_statistiques();
    remplir_combo_reservations();           /* la liste des réservations du formulaire Paiements */
}

static void maj_montant(void)
{
    int pl = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(WR("f_places")));
    int px = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(WR("f_prix")));
    char b[32];
    snprintf(b, sizeof b, "%d TND", pl * px);
    gtk_label_set_text(GTK_LABEL(WR("lbl_montant")), b);
}

/* ------------------------------------------------------------------ */
/*  Formulaire                                                         */
/* ------------------------------------------------------------------ */
static void vider_formulaire(void)
{
    time_t maintenant = time(NULL);
    struct tm *t = localtime(&maintenant);

    id_choisi[0] = '\0';
    ecrire_entry(WR("f_id"), "");
    gtk_editable_set_editable(GTK_EDITABLE(WR("f_id")), TRUE);
    ecrire_entry(WR("f_passager"), "");
    ecrire_entry(WR("f_cin"), "");
    ecrire_entry(WR("f_vol"), "");
    gtk_combo_box_set_active(GTK_COMBO_BOX(WR("combo_f_classe")), -1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(WR("f_jour")), t->tm_mday);       /* par défaut : la date du jour */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(WR("f_mois")), t->tm_mon + 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(WR("f_annee")), t->tm_year + 1900);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(WR("f_places")), 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(WR("f_prix")), 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(WR("rb_confirmee")), TRUE);
    gtk_label_set_text(GTK_LABEL(WR("lbl_msg_fiche")), "");
    maj_montant();
}

static void remplir_formulaire(Reservation r)
{
    copier_texte(id_choisi, sizeof id_choisi, r.id);
    ecrire_entry(WR("f_id"), r.id);
    gtk_editable_set_editable(GTK_EDITABLE(WR("f_id")), FALSE);      /* l'identifiant ne se modifie pas */
    ecrire_entry(WR("f_passager"), r.passager);
    ecrire_entry(WR("f_cin"), r.cin);
    ecrire_entry(WR("f_vol"), r.vol);
    choisir_combo(WR("combo_f_classe"), r.classe);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(WR("f_jour")), r.date.jour);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(WR("f_mois")), r.date.mois);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(WR("f_annee")), r.date.annee);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(WR("f_places")), r.places);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(WR("f_prix")), r.prix);
    if (strcmp(r.statut, "En attente") == 0)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(WR("rb_attente")), TRUE);
    else if (strcmp(r.statut, "Annulée") == 0)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(WR("rb_annulee")), TRUE);
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(WR("rb_confirmee")), TRUE);
    maj_montant();
}

/* Lit les champs du formulaire et remplit une structure Reservation. */
static Reservation lire_formulaire(void)
{
    Reservation r;
    char *t;
    int i;

    memset(&r, 0, sizeof r);
    copier_texte(r.id, sizeof r.id, gtk_entry_get_text(GTK_ENTRY(WR("f_id"))));
    enlever_espaces(r.id);
    for (i = 0; r.id[i]; i++) r.id[i] = toupper((unsigned char)r.id[i]);
    copier_texte(r.passager, sizeof r.passager, gtk_entry_get_text(GTK_ENTRY(WR("f_passager"))));
    enlever_espaces(r.passager);
    copier_texte(r.cin, sizeof r.cin, gtk_entry_get_text(GTK_ENTRY(WR("f_cin"))));
    enlever_espaces(r.cin);
    copier_texte(r.vol, sizeof r.vol, gtk_entry_get_text(GTK_ENTRY(WR("f_vol"))));
    enlever_espaces(r.vol);
    for (i = 0; r.vol[i]; i++) r.vol[i] = toupper((unsigned char)r.vol[i]);
    t = texte_combo(WR("combo_f_classe"));
    copier_texte(r.classe, sizeof r.classe, t);
    g_free(t);
    r.date.jour = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(WR("f_jour")));
    r.date.mois = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(WR("f_mois")));
    r.date.annee = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(WR("f_annee")));
    r.places = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(WR("f_places")));
    r.prix = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(WR("f_prix")));
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(WR("rb_confirmee")))) strcpy(r.statut, "Confirmée");
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(WR("rb_attente")))) strcpy(r.statut, "En attente");
    else strcpy(r.statut, "Annulée");
    return r;
}

/* ------------------------------------------------------------------ */
/*  Ouverture de la fenêtre                                            */
/* ------------------------------------------------------------------ */
void ouvrir_reservations(int onglet)
{
    GtkWidget *fenetre;
    char info[60];

    pret = 0;
    xml_res = glade_xml_new(FICHIER_GLADE, "window_reservations", NULL);
    glade_xml_signal_autoconnect(xml_res);
    fenetre = WR("window_reservations");
    g_signal_connect(fenetre, "delete-event", G_CALLBACK(gtk_main_quit), NULL);

    remplir_combo(WR("combo_f_classe"), choix_classe, 3, -1);
    remplir_combo(WR("combo_critere"), choix_crit, 5, 0);
    remplir_combo(WR("combo_filtre_classe"), filtre_classe, 4, 0);
    remplir_combo(WR("combo_filtre_statut"), filtre_statut, 4, 0);
    liste = preparer_liste(WR("treeview_reservations"), 12, titres);
    snprintf(info, sizeof info, "Connecté : %s", utilisateur);
    gtk_label_set_text(GTK_LABEL(WR("lbl_user_res")), info);
    init_paiements();
    init_historique();

    tri_croissant = 1;
    recharger_reservations();
    vider_formulaire();
    pret = 1;
    afficher_statut(WR("statusbar_res"), "Gestion des réservations : les données sont dans reservations.txt");
    gtk_widget_show_all(fenetre);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(WR("notebook")), onglet);
}

/* ------------------------------------------------------------------ */
/*  Gestionnaires de signaux (noms donnés par Glade : on_<widget>_<signal>) */
/* ------------------------------------------------------------------ */
void on_btn_ajouter_clicked(GtkWidget *w, gpointer data)
{
    Reservation r;
    int erreur, res;
    (void)w; (void)data;

    r = lire_formulaire();
    erreur = valider_reservation(r);
    if (erreur != 0) {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_fiche")), message_erreur_reservation(erreur));
        return;
    }
    res = ajouter_reservation(FICHIER_RESERVATIONS, r);
    if (res == 1) {
        historiser(r.id, "Ajout", "Réservation ajoutée");
        recharger_reservations();
        vider_formulaire();
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_fiche")), "Réservation ajoutée : le fichier reservations.txt a été mis à jour.");
    } else if (res == 0) {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_fiche")), "Cet identifiant existe déjà.");
    } else {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_fiche")), "Erreur d'accès au fichier reservations.txt.");
    }
}

void on_btn_modifier_clicked(GtkWidget *w, gpointer data)
{
    Reservation r;
    int erreur, res;
    (void)w; (void)data;

    if (id_choisi[0] == '\0') {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_fiche")), "Sélectionnez d'abord une réservation dans la liste.");
        return;
    }
    r = lire_formulaire();
    erreur = valider_reservation(r);
    if (erreur != 0) {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_fiche")), message_erreur_reservation(erreur));
        return;
    }
    res = modifier_reservation(FICHIER_RESERVATIONS, id_choisi, r);
    if (res == 1) {
        historiser(id_choisi, "Modification", "Fiche de la réservation modifiée");
        recharger_reservations();
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_fiche")), "Modification enregistrée dans le fichier.");
    } else if (res == 0) {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_fiche")), "Réservation introuvable.");
    } else {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_fiche")), "Erreur d'accès au fichier reservations.txt.");
    }
}

void on_btn_supprimer_clicked(GtkWidget *w, gpointer data)
{
    GtkWidget *dialogue;
    int reponse, supprimes;
    (void)w; (void)data;

    if (id_choisi[0] == '\0') {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_fiche")), "Sélectionnez d'abord une réservation dans la liste.");
        return;
    }
    dialogue = gtk_message_dialog_new(GTK_WINDOW(WR("window_reservations")), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
                                      GTK_BUTTONS_YES_NO, "Supprimer la réservation %s ?\nSes paiements seront aussi supprimés.", id_choisi);
    reponse = gtk_dialog_run(GTK_DIALOG(dialogue));
    gtk_widget_destroy(dialogue);
    if (reponse != GTK_RESPONSE_YES) return;

    if (supprimer_reservation(FICHIER_RESERVATIONS, id_choisi) == 1) {
        char detail[60];
        supprimes = supprimer_paiements_reservation(FICHIER_PAIEMENTS, id_choisi);
        snprintf(detail, sizeof detail, "Réservation supprimée (%d paiement(s) supprimé(s))", supprimes);
        historiser(id_choisi, "Suppression", detail);
        recharger_paiements();
        recharger_reservations();
        vider_formulaire();
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_fiche")), "Réservation supprimée du fichier.");
    } else {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_fiche")), "Suppression impossible.");
    }
}

void on_btn_vider_clicked(GtkWidget *w, gpointer data)
{ (void)w; (void)data; vider_formulaire(); }

/* Clic sur une ligne de la liste : on remplit le formulaire. */
void on_treeview_reservations_cursor_changed(GtkWidget *tv, gpointer data)
{
    char id[12];
    Reservation r;
    (void)data;

    if (!valeur_selectionnee(tv, 0, id, sizeof id)) return;
    r = chercher_reservation(FICHIER_RESERVATIONS, id);
    if (r.id[0] != '\0') {
        remplir_formulaire(r);
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_fiche")), "");
    }
}

/* Export de la liste affichée vers un fichier .csv (s'ouvre dans Excel). */
void on_btn_exporter_res_clicked(GtkWidget *w, gpointer data)
{
    (void)w; (void)data;
    if (exporter_reservations("export_reservations.csv", vue, nv) == 1) {
        historiser("", "Export", "Liste des réservations exportée : export_reservations.csv");
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_fiche")), "Liste exportée dans le fichier export_reservations.csv.");
    } else {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_fiche")), "Export impossible.");
    }
}

/* Raccourcis vers les autres onglets, pour la réservation sélectionnée. */
void on_btn_voir_pai_clicked(GtkWidget *w, gpointer data)
{
    (void)w; (void)data;
    if (id_choisi[0] == '\0') {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_fiche")), "Sélectionnez d'abord une réservation dans la liste.");
        return;
    }
    voir_paiements_reservation(id_choisi);
}

void on_btn_voir_hist_clicked(GtkWidget *w, gpointer data)
{
    (void)w; (void)data;
    if (id_choisi[0] == '\0') {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_fiche")), "Sélectionnez d'abord une réservation dans la liste.");
        return;
    }
    voir_historique_reservation(id_choisi);
}

void on_f_places_value_changed(GtkWidget *w, gpointer data)
{ (void)w; (void)data; if (pret) maj_montant(); }

void on_f_prix_value_changed(GtkWidget *w, gpointer data)
{ (void)w; (void)data; if (pret) maj_montant(); }

void on_btn_rechercher_clicked(GtkWidget *w, gpointer data)
{ (void)w; (void)data; afficher_liste(); }

void on_entry_recherche_activate(GtkWidget *w, gpointer data)
{ on_btn_rechercher_clicked(w, data); }

void on_btn_reinitialiser_clicked(GtkWidget *w, gpointer data)
{
    (void)w; (void)data;
    pret = 0;                                    /* évite de réafficher plusieurs fois */
    ecrire_entry(WR("entry_recherche"), "");
    gtk_combo_box_set_active(GTK_COMBO_BOX(WR("combo_critere")), 0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(WR("combo_filtre_classe")), 0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(WR("combo_filtre_statut")), 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(WR("chk_attente")), FALSE);
    pret = 1;
    afficher_liste();
}

void on_rb_tri_asc_toggled(GtkWidget *w, gpointer data)
{
    (void)data;
    if (!pret || !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
    tri_croissant = 1;
    afficher_liste();
}

void on_rb_tri_desc_toggled(GtkWidget *w, gpointer data)
{
    (void)data;
    if (!pret || !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) return;
    tri_croissant = 0;
    afficher_liste();
}

void on_combo_filtre_classe_changed(GtkWidget *w, gpointer data)
{ (void)w; (void)data; if (pret) afficher_liste(); }

void on_combo_filtre_statut_changed(GtkWidget *w, gpointer data)
{ (void)w; (void)data; if (pret) afficher_liste(); }

void on_chk_attente_toggled(GtkWidget *w, gpointer data)
{ (void)w; (void)data; if (pret) afficher_liste(); }

void on_btn_menu_clicked(GtkWidget *w, gpointer data)
{
    (void)w; (void)data;
    gtk_widget_destroy(WR("window_reservations"));
    ouvrir_principal();
}

void on_btn_deconnexion_res_clicked(GtkWidget *w, gpointer data)
{
    (void)w; (void)data;
    gtk_widget_destroy(WR("window_reservations"));
    ouvrir_login();
}
