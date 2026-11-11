/* historique_ui.c - Onglet « Historique » : affiche le journal des actions.
 * Toute la logique (fichier, filtre) est dans historique.c. */
#include <stdio.h>
#include <string.h>
#include "app.h"

static Historique tout[MAX_HIST];       /* tout le journal (lu dans historique.txt) */
static Historique vue[MAX_HIST];        /* lignes affichées                          */
static int nbh = 0, nvh = 0;
static int pret_h = 0;
static GtkListStore *liste_h;

static const char *choix_action[] = { "Toutes", "Ajout", "Modification", "Suppression", "Paiement", "Export" };
static const char *titres_h[] = { "Date et heure", "Utilisateur", "Réservation", "Action", "Détails" };

static void afficher_liste_h(void)
{
    int i;
    char texte[64], info[60];
    char *action = texte_combo(WR("combo_h_action"));

    if (strcmp(action, "Toutes") == 0) action[0] = '\0';
    copier_texte(texte, sizeof texte, gtk_entry_get_text(GTK_ENTRY(WR("entry_h_recherche"))));
    enlever_espaces(texte);
    nvh = filtrer_historique(tout, nbh, vue, texte, action);

    gtk_list_store_clear(liste_h);
    for (i = nvh - 1; i >= 0; i--) {                 /* les actions les plus récentes en premier */
        GtkTreeIter it;
        gtk_list_store_append(liste_h, &it);
        gtk_list_store_set(liste_h, &it, 0, vue[i].date, 1, vue[i].utilisateur, 2, vue[i].reference,
                           3, vue[i].action, 4, vue[i].details, -1);
    }
    snprintf(info, sizeof info, "%d actions affichées (sur %d)", nvh, nbh);
    gtk_label_set_text(GTK_LABEL(WR("lbl_h_nb")), info);
    g_free(action);
}

void recharger_historique(void)
{
    nbh = charger_historique(FICHIER_HIST, tout, MAX_HIST);
    if (nbh < 0) nbh = 0;
    if (pret_h) afficher_liste_h();
}

void init_historique(void)
{
    pret_h = 0;
    remplir_combo(WR("combo_h_action"), choix_action, 6, 0);
    liste_h = preparer_liste(WR("treeview_hist"), 5, titres_h);
    recharger_historique();
    afficher_liste_h();
    pret_h = 1;
}

/* Appelée par les autres onglets après chaque action : une ligne est ajoutée au fichier historique.txt. */
void historiser(const char *reference, const char *action, const char *details)
{
    char i[12], a[16], d[80];
    copier_texte(i, sizeof i, reference);
    copier_texte(a, sizeof a, action);
    copier_texte(d, sizeof d, details);
    ajouter_historique(FICHIER_HIST, utilisateur, i, a, d);
    recharger_historique();
    afficher_statistiques();
}

/* Bouton « Historique de la réservation » de l'onglet Réservations. */
void voir_historique_reservation(const char *reference)
{
    pret_h = 0;
    ecrire_entry(WR("entry_h_recherche"), reference);
    gtk_combo_box_set_active(GTK_COMBO_BOX(WR("combo_h_action")), 0);
    pret_h = 1;
    afficher_liste_h();
    gtk_notebook_set_current_page(GTK_NOTEBOOK(WR("notebook")), 2);
}

/* ----------------------- gestionnaires de signaux ----------------------- */
void on_btn_h_rechercher_clicked(GtkWidget *w, gpointer data)
{ (void)w; (void)data; afficher_liste_h(); }

void on_entry_h_recherche_activate(GtkWidget *w, gpointer data)
{ on_btn_h_rechercher_clicked(w, data); }

void on_combo_h_action_changed(GtkWidget *w, gpointer data)
{ (void)w; (void)data; if (pret_h) afficher_liste_h(); }

void on_btn_h_reinitialiser_clicked(GtkWidget *w, gpointer data)
{
    (void)w; (void)data;
    pret_h = 0;
    ecrire_entry(WR("entry_h_recherche"), "");
    gtk_combo_box_set_active(GTK_COMBO_BOX(WR("combo_h_action")), 0);
    pret_h = 1;
    afficher_liste_h();
}

void on_btn_h_exporter_clicked(GtkWidget *w, gpointer data)
{
    (void)w; (void)data;
    if (exporter_historique("export_historique.csv", vue, nvh) == 1)
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_hist")), "Historique exporté dans le fichier export_historique.csv.");
    else
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_hist")), "Export impossible.");
}

void on_btn_h_effacer_clicked(GtkWidget *w, gpointer data)
{
    GtkWidget *dialogue;
    int reponse;
    (void)w; (void)data;

    dialogue = gtk_message_dialog_new(GTK_WINDOW(WR("window_reservations")), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
                                      GTK_BUTTONS_YES_NO, "Effacer tout l'historique ?\nCette action est définitive.");
    reponse = gtk_dialog_run(GTK_DIALOG(dialogue));
    gtk_widget_destroy(dialogue);
    if (reponse != GTK_RESPONSE_YES) return;

    if (vider_historique(FICHIER_HIST) == 1) {
        historiser("", "Suppression", "Historique effacé");          /* trace de l'effacement */
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_hist")), "Historique effacé.");
    } else {
        gtk_label_set_text(GTK_LABEL(WR("lbl_msg_hist")), "Erreur d'accès au fichier historique.txt.");
    }
}
