CC     = gcc
CFLAGS = -Wall -Wextra -g `pkg-config --cflags gtk+-2.0 libglade-2.0`
LIBS   = `pkg-config --libs gtk+-2.0 libglade-2.0` -export-dynamic
OBJ    = main.o login.o principal.o gestion_reservations.o paiement_ui.o historique_ui.o espace.o outils.o reservation.o paiement.o historique.o

# Application complète (interface Glade 2)
gestion_reservations: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LIBS)

# Tests des modules SANS interface : on lit les résultats dans les fichiers .txt
test: test_reservation test_paiement test_historique
	@./test_reservation; echo "reservation  : code retour = $$? (0 = tout est correct)"; cat rapport_reservation.txt
	@./test_paiement; echo "paiement     : code retour = $$? (0 = tout est correct)"; cat rapport_paiement.txt
	@./test_historique; echo "historique   : code retour = $$? (0 = tout est correct)"; cat rapport_historique.txt

test_reservation: test_reservation.o reservation.o
	$(CC) -o $@ test_reservation.o reservation.o

test_paiement: test_paiement.o paiement.o reservation.o
	$(CC) -o $@ test_paiement.o paiement.o reservation.o

test_historique: test_historique.o historique.o
	$(CC) -o $@ test_historique.o historique.o

%.o: %.c app.h reservation.h paiement.h historique.h date.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o gestion_reservations test_reservation test_paiement test_historique \
	      test_reservation.txt tri_reservation.txt rapport_reservation.txt export_reservation.csv \
	      test_paiement.txt test_pai_reservations.txt tri_paiement.txt rapport_paiement.txt export_paiement.csv \
	      test_historique.txt rapport_historique.txt export_historique.csv
