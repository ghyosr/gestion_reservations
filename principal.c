/* principal.c - Interface principale : tableau de bord et menu de l'administrateur */
#include <stdio.h>
#include <string.h>
#include "app.h"

static GladeXML *xml_principal;
#define W(id) glade_xml_get_widget(xml_principal, id)

static void ecrire_nombre(const char *id, int valeur)
{
    afficher_nombre_gros(W(id), valeur);               /* grand nombre dans une carte */
}

/* Remplit le tableau de bord en lisant les trois fichiers texte. */
static void remplir_tableau_de_bord(void)
{
    static Reservation rs[MAX_RESERVATIONS];
    static Paiement pa[MAX_PAIEMENTS];
    static Historique hs[MAX_HIST];
    int nr = charger_reservations(FICHIER_RESERVATIONS, rs, MAX_RESERVATIONS);
    int np = charger_paiements(FICHIER_PAIEMENTS, pa, MAX_PAIEMENTS);
    int nh = charger_historique(FICHIER_HIST, hs, MAX_HIST);
    int i, reste = 0;

    if (nr < 0) nr = 0;
    if (np < 0) np = 0;
    if (nh < 0) nh = 0;
    for (i = 0; i < nr; i++)
        if (strcmp(rs[i].statut, "Annulée") != 0) reste += montant_total(rs[i]) - total_paye(pa, np, rs[i].id);
    ecrire_nombre("lbl_d_total", nr);
    ecrire_nombre("lbl_d_conf", compter_statut(rs, nr, "Confirmée"));
    ecrire_nombre("lbl_d_att", compter_statut(rs, nr, "En attente"));
    ecrire_nombre("lbl_d_ann", compter_statut(rs, nr, "Annulée"));
    ecrire_nombre("lbl_d_enc", somme_paiements(pa, np));
    ecrire_nombre("lbl_d_reste", reste);
    ecrire_nombre("lbl_d_hist", nh);
    regler_progression(W("bar_enc"), somme_paiements(pa, np), somme_paiements(pa, np) + reste, "TND encaissés");
}

void ouvrir_principal(void)
{
    GtkWidget *fenetre;
    char b[80];

    xml_principal = glade_xml_new(FICHIER_GLADE, "window_principal", NULL);
    glade_xml_signal_autoconnect(xml_principal);
    fenetre = W("window_principal");
    g_signal_connect(fenetre, "delete-event", G_CALLBACK(gtk_main_quit), NULL);
    snprintf(b, sizeof b, "Connecté : %s", utilisateur);
    gtk_label_set_text(GTK_LABEL(W("lbl_user_menu")), b);
    remplir_tableau_de_bord();
    gtk_widget_show_all(fenetre);
}

/* ----------------------- gestionnaires de signaux ----------------------- */
void on_btn_gestion_reservations_clicked(GtkWidget *w, gpointer data)
{
    (void)w; (void)data;
    gtk_widget_destroy(W("window_principal"));
    ouvrir_reservations(0);
}

void on_btn_gestion_paiements_clicked(GtkWidget *w, gpointer data)
{
    (void)w; (void)data;
    gtk_widget_destroy(W("window_principal"));
    ouvrir_reservations(1);
}

void on_btn_gestion_historique_clicked(GtkWidget *w, gpointer data)
{
    (void)w; (void)data;
    gtk_widget_destroy(W("window_principal"));
    ouvrir_reservations(2);
}

void on_btn_deconnexion_menu_clicked(GtkWidget *w, gpointer data)
{
    (void)w; (void)data;
    gtk_widget_destroy(W("window_principal"));
    ouvrir_login();
}

void on_btn_quitter_menu_clicked(GtkWidget *w, gpointer data)
{ (void)w; (void)data; gtk_main_quit(); }
