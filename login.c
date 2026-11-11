/* login.c - Fenêtre de connexion : l'identifiant décide de l'interface ouverte
 * (administrateur ou utilisateur). Les comptes sont dans users.txt : identifiant;mot de passe;role;reference */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app.h"

char utilisateur[40] = "";
char reference[40] = "";
int  est_admin = 0;
static GladeXML *xml_login;
#define W(id) glade_xml_get_widget(xml_login, id)

/* Cherche le compte dans users.txt. Retourne 1 si trouvé (et remplit role / ref). */
static int verifier_utilisateur(const char *login, const char *mdp, char *role, char *ref)
{
    FILE *f = fopen(FICHIER_USERS, "r");
    char ligne[200];
    int ok = 0;

    if (f == NULL) return 0;
    while (!ok && fgets(ligne, sizeof ligne, f) != NULL) {
        char *champ[4];
        int n = 0;
        char *p;
        ligne[strcspn(ligne, "\r\n")] = '\0';
        champ[n++] = ligne;
        for (p = ligne; *p && n < 4; p++)
            if (*p == ';') { *p = '\0'; champ[n++] = p + 1; }
        if (n < 3) continue;
        if (strcmp(champ[0], login) == 0 && strcmp(champ[1], mdp) == 0) {
            ok = 1;
            copier_texte(role, 16, champ[2]);
            copier_texte(ref, 40, n > 3 ? champ[3] : "");
        }
    }
    fclose(f);
    return ok;
}

void ouvrir_login(void)
{
    GtkWidget *fenetre;

    xml_login = glade_xml_new(FICHIER_GLADE, "window_login", NULL);
    if (xml_login == NULL) {
        fprintf(stderr, "Impossible d'ouvrir %s (lancez ./gestion_reservations depuis son dossier)\n", FICHIER_GLADE);
        exit(1);
    }
    glade_xml_signal_autoconnect(xml_login);
    fenetre = W("window_login");
    g_signal_connect(fenetre, "delete-event", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(fenetre);
    gtk_widget_grab_focus(W("entry_identifiant"));
}

/* ----------------------- gestionnaires de signaux ----------------------- */
void on_btn_connexion_clicked(GtkWidget *bouton, gpointer data)
{
    char login[40], mdp[40], role[16], ref[40];
    (void)bouton; (void)data;

    copier_texte(login, sizeof login, gtk_entry_get_text(GTK_ENTRY(W("entry_identifiant"))));
    copier_texte(mdp, sizeof mdp, gtk_entry_get_text(GTK_ENTRY(W("entry_motdepasse"))));
    enlever_espaces(login);
    if (strcmp(login, "") == 0 || strcmp(mdp, "") == 0) {
        gtk_label_set_text(GTK_LABEL(W("lbl_msg_login")), "Saisissez l'identifiant et le mot de passe.");
        return;
    }
    if (!verifier_utilisateur(login, mdp, role, ref)) {
        gtk_label_set_text(GTK_LABEL(W("lbl_msg_login")), "Identifiant ou mot de passe incorrect.");
        ecrire_entry(W("entry_motdepasse"), "");
        return;
    }
    copier_texte(utilisateur, sizeof utilisateur, login);
    copier_texte(reference, sizeof reference, ref);
    est_admin = (strcmp(role, "admin") == 0);
    gtk_widget_destroy(W("window_login"));
    if (est_admin) ouvrir_principal();
    else ouvrir_utilisateur();
}

void on_entry_identifiant_activate(GtkWidget *w, gpointer data)
{ (void)w; (void)data; gtk_widget_grab_focus(W("entry_motdepasse")); }

void on_entry_motdepasse_activate(GtkWidget *w, gpointer data)
{ on_btn_connexion_clicked(w, data); }

void on_btn_quitter_clicked(GtkWidget *w, gpointer data)
{ (void)w; (void)data; gtk_main_quit(); }
