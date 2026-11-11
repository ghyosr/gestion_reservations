#ifndef APP_H
#define APP_H

#include <gtk/gtk.h>
#include <glade/glade.h>
#include "reservation.h"
#include "paiement.h"
#include "historique.h"

#define FICHIER_GLADE "gestion_reservations.glade"
#define FICHIER_RESERVATIONS "reservations.txt"
#define FICHIER_PAIEMENTS "paiements.txt"
#define FICHIER_HIST "historique.txt"
#define FICHIER_USERS "users.txt"

/* ---- outils.c : petites fonctions d'aide pour l'interface -------------- */
void  copier_texte(char *dest, int taille, const char *source);
void  enlever_espaces(char *s);
void  remplir_combo(GtkWidget *combo, const char **choix, int n, int actif);
char *texte_combo(GtkWidget *combo);                 /* à libérer avec g_free ; "" si rien de choisi */
void  choisir_combo(GtkWidget *combo, const char *valeur);
void  ecrire_entry(GtkWidget *entry, const char *texte);
void  afficher_statut(GtkWidget *barre, const char *texte);
GtkListStore *preparer_liste(GtkWidget *treeview, int nb_colonnes, const char **titres);
int   valeur_selectionnee(GtkWidget *treeview, int colonne, char *dest, int taille);
void  afficher_gros(GtkWidget *label, const char *texte);
void  afficher_nombre_gros(GtkWidget *label, int valeur);
void  regler_progression(GtkWidget *barre, int valeur, int total, const char *unite);
void  afficher_date_du_jour(GtkWidget *label);

/* ---- les fenêtres de l'application ------------------------------------- */
extern char utilisateur[40];            /* identifiant connecté (défini dans login.c) */
extern char reference[40];            /* CIN du passager connecté (vide pour l'administrateur) */
extern int  est_admin;               /* 1 = administrateur, 0 = passager */
void ouvrir_utilisateur(void);          /* espace.c : espace passager */
void ouvrir_login(void);                /* login.c                  */
void ouvrir_principal(void);            /* principal.c              */
void ouvrir_reservations(int onglet);   /* gestion_reservations.c : 0 = Réservations, 1 = Paiements, 2 = Historique, 3 = Statistiques */

/* La fenêtre « Gestion des réservations » est découpée en 3 fichiers (un par onglet) qui la partagent. */
extern GladeXML *xml_res;
#define WR(id) glade_xml_get_widget(xml_res, id)

/* gestion_reservations.c : onglet Réservations + statistiques */
void recharger_reservations(void);
void afficher_statistiques(void);
/* paiement_ui.c : onglet Paiements */
void init_paiements(void);
void recharger_paiements(void);
void remplir_combo_reservations(void);              /* liste des identifiants dans le formulaire Paiements */
void voir_paiements_reservation(const char *id);
/* historique_ui.c : onglet Historique */
void init_historique(void);
void recharger_historique(void);
void historiser(const char *reference, const char *action, const char *details);   /* ajoute une ligne au journal */
void voir_historique_reservation(const char *reference);

#endif
