/* outils.c - fonctions d'aide pour l'interface GTK 2 / libglade 2 */
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include "app.h"

void copier_texte(char *dest, int taille, const char *source)
{
    strncpy(dest, source ? source : "", taille - 1);
    dest[taille - 1] = '\0';
}

void enlever_espaces(char *s)                 /* supprime les espaces au début et à la fin */
{
    int debut = 0, fin = strlen(s);
    while (s[debut] && isspace((unsigned char)s[debut])) debut++;
    while (fin > debut && isspace((unsigned char)s[fin - 1])) fin--;
    memmove(s, s + debut, fin - debut);
    s[fin - debut] = '\0';
}

/* libglade 2 ne remplit pas les GtkComboBox : on met les choix ici.
 * actif = numéro du choix sélectionné au départ (-1 = aucun). */
void remplir_combo(GtkWidget *combo, const char **choix, int n, int actif)
{
    int i;
    GtkListStore *st = gtk_list_store_new(1, G_TYPE_STRING);
    GList *cellules = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(combo));

    for (i = 0; i < n; i++) {
        GtkTreeIter it;
        gtk_list_store_append(st, &it);
        gtk_list_store_set(st, &it, 0, choix[i], -1);
    }
    gtk_combo_box_set_model(GTK_COMBO_BOX(combo), GTK_TREE_MODEL(st));
    g_object_unref(st);
    if (cellules == NULL) {
        GtkCellRenderer *r = gtk_cell_renderer_text_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), r, TRUE);
        gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), r, "text", 0, NULL);
    }
    g_list_free(cellules);
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), actif);
}

char *texte_combo(GtkWidget *combo)
{
    GtkTreeIter it;
    char *t = NULL;
    GtkTreeModel *m = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
    if (m != NULL && gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &it))
        gtk_tree_model_get(m, &it, 0, &t, -1);
    return t ? t : g_strdup("");
}

void choisir_combo(GtkWidget *combo, const char *valeur)
{
    GtkTreeModel *m = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
    GtkTreeIter it;
    int i = 0;

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), -1);
    if (m == NULL || !gtk_tree_model_get_iter_first(m, &it)) return;
    do {
        char *s = NULL;
        gtk_tree_model_get(m, &it, 0, &s, -1);
        if (s != NULL && strcmp(s, valeur) == 0) {
            g_free(s);
            gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i);
            return;
        }
        g_free(s);
        i++;
    } while (gtk_tree_model_iter_next(m, &it));
}

void ecrire_entry(GtkWidget *entry, const char *texte)
{
    gtk_entry_set_text(GTK_ENTRY(entry), texte ? texte : "");
}

void afficher_statut(GtkWidget *barre, const char *texte)
{
    gtk_statusbar_pop(GTK_STATUSBAR(barre), 0);
    gtk_statusbar_push(GTK_STATUSBAR(barre), 0, texte);
}

/* Prépare un GtkTreeView : toutes les colonnes contiennent du texte. */
GtkListStore *preparer_liste(GtkWidget *treeview, int nb_colonnes, const char **titres)
{
    int i;
    GType types[16];
    GtkListStore *st;

    for (i = 0; i < nb_colonnes; i++) types[i] = G_TYPE_STRING;
    st = gtk_list_store_newv(nb_colonnes, types);
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(st));
    for (i = 0; i < nb_colonnes; i++) {
        GtkCellRenderer *r = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *c = gtk_tree_view_column_new_with_attributes(titres[i], r, "text", i, NULL);
        gtk_tree_view_column_set_resizable(c, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), c);
    }
    g_object_unref(st);
    return st;
}

/* Texte de la colonne « colonne » de la ligne sélectionnée (retourne 0 si aucune ligne). */
int valeur_selectionnee(GtkWidget *treeview, int colonne, char *dest, int taille)
{
    GtkTreeModel *m;
    GtkTreeIter it;
    char *s = NULL;
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

    if (!gtk_tree_selection_get_selected(sel, &m, &it)) return 0;
    gtk_tree_model_get(m, &it, colonne, &s, -1);
    copier_texte(dest, taille, s);
    g_free(s);
    return 1;
}

/* ------------------------------------------------------------------ */
/*  Présentation : cartes de chiffres, symboles, alignement            */
/* ------------------------------------------------------------------ */
void afficher_gros(GtkWidget *label, const char *texte)
{
    char *m = g_markup_printf_escaped("<span size=\"22000\" weight=\"bold\">%s</span>", texte);
    gtk_label_set_markup(GTK_LABEL(label), m);
    g_free(m);
}

void afficher_nombre_gros(GtkWidget *label, int valeur)
{
    char b[24];
    snprintf(b, sizeof b, "%d", valeur);
    afficher_gros(label, b);
}

/* Un petit symbole devant l'état : lisible sans couleur. */
const char *avec_symbole(const char *etat, char *dest, int taille)
{
    const char *symbole = "";

    if (strcmp(etat, "Opérationnel") == 0) symbole = "●  ";
    else if (strcmp(etat, "En maintenance") == 0) symbole = "▲  ";
    else if (strcmp(etat, "Hors service") == 0) symbole = "✖  ";
    else if (strcmp(etat, "Planifiée") == 0) symbole = "○  ";
    else if (strcmp(etat, "En cours") == 0) symbole = "▶  ";
    else if (strcmp(etat, "Terminée") == 0) symbole = "✔  ";
    snprintf(dest, taille, "%s%s", symbole, etat);
    return dest;
}

void aligner_colonne(GtkWidget *treeview, int colonne)
{
    GtkTreeViewColumn *c = gtk_tree_view_get_column(GTK_TREE_VIEW(treeview), colonne);
    GList *rendus = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(c));

    if (rendus != NULL) g_object_set(rendus->data, "xalign", 1.0, NULL);
    gtk_tree_view_column_set_alignment(c, 1.0);
    g_list_free(rendus);
}

/* Barre de progression : valeur / total, avec un texte du genre « 120 / 802 places (15 %) » */
void regler_progression(GtkWidget *barre, int valeur, int total, const char *unite)
{
    char b[80];
    double fraction = total > 0 ? (double)valeur / total : 0.0;

    if (fraction > 1.0) fraction = 1.0;
    snprintf(b, sizeof b, "%d / %d %s (%d %%)", valeur, total, unite, total > 0 ? valeur * 100 / total : 0);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(barre), fraction);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(barre), b);
}

void afficher_date_du_jour(GtkWidget *label)
{
    char b[40];
    time_t maintenant = time(NULL);
    strftime(b, sizeof b, "%d/%m/%Y", localtime(&maintenant));
    gtk_label_set_text(GTK_LABEL(label), b);
}
