/* main.c - Gestion des réservations - Carthage Airways
 * Compilation : make        Exécution : ./gestion_reservations */
#include "app.h"

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    ouvrir_login();
    gtk_main();
    return 0;
}
