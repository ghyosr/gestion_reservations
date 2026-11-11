# Gestion des réservations - Carthage Airways

Gestion des réservations avec **Paiements** et **Historique** (version complète). Projet de programmation C **(séance 4)** - interface **Glade 2 / GTK 2** (libglade2) - Ubuntu 18.04.
Cette gestion possède **deux interfaces** : l'**espace administrateur** et l'**espace passager** (utilisateur).

## 1. Lancer le projet

```bash
sudo apt install build-essential libgtk2.0-dev libglade2-dev pkg-config   # une seule fois
make                         # compile l'application
./gestion_reservations       # lance l'application (depuis ce dossier !)
make test                    # teste les 3 modules SANS interface (résultats dans des fichiers .txt)
```

## 2. Comptes de test (`users.txt` : `identifiant;mot de passe;role;reference`)

| Identifiant | Mot de passe | Interface ouverte |
|---|---|---|
| `admin` | `admin123` | **Administrateur** |
| `client1` | `client123` | Passager Sami Trabelsi (référence = son CIN 09123456) |
| `client2` | `client456` | Passager Leila Gharbi (CIN 08234567) |
| `client3` | `client789` | Passager Marc Dupont (CIN 12345678) |

## 3. Interface ADMINISTRATEUR

1. **Menu principal** : tableau de bord en cartes (réservations par statut, total encaissé, reste à encaisser, actions) + barre d'encaissement.
2. **Fenêtre de gestion** à 4 onglets :

| Onglet | Contenu |
|---|---|
| **Réservations** | fiche, Ajouter / Modifier / Supprimer / Vider, recherche, tri, filtres *Classe* et *Statut*, case « En attente », export `.csv`, colonnes *Payé* et *Reste* |
| **Paiements** | versements liés à une réservation (plusieurs paiements possibles), reste à payer calculé, paiement refusé s'il dépasse le reste ou si la réservation est annulée |
| **Historique** | journal automatique (date, heure, utilisateur, réservation, action) |
| **Statistiques** | réservations par statut, chiffre d'affaires, paiements par mode, reste à encaisser |

Règle automatique : une réservation « En attente » **entièrement payée** devient « Confirmée ».

## 4. Interface UTILISATEUR : espace passager

Le passager ne voit **que ses réservations** (filtrées par son CIN).

- **Recherche** dans mes réservations + liste (GtkTreeView).
- **Détails** : identifiant, vol, date, classe, places, prix, montant total, statut, **déjà payé**, **reste à payer**, barre de paiement.
- **Payer maintenant** (montant + mode) : crée un paiement ; la réservation devient « Confirmée » quand tout est payé.
- **Annuler la réservation** sélectionnée.
- **Nouvelle réservation** (vol, classe, nombre de places) : créée « En attente » (prix selon la classe : Économique 300, Affaires 700, Première 1200 TND).
- Toutes ces actions sont écrites dans l'historique avec le nom du passager.

## 5. Modules (le module porte le nom de la structure)

| Module | Structure | Fichier texte |
|---|---|---|
| `reservation.h/.c` | `Reservation` | `reservations.txt` |
| `paiement.h/.c` | `Paiement` | `paiements.txt` |
| `historique.h/.c` | `Historique` | `historique.txt` |
| `date.h` | `Date` | - |

Chaque module : ajouter, modifier, supprimer, chercher, charger, sauvegarder, filtrer, trier, compter, exporter, valider. Toutes les fonctions retournent un **code** : **aucun `printf` / `scanf`**.

## 6. Respect des consignes du devoir

| Consigne | Réponse |
|---|---|
| Implémenter les fonctions demandées | `reservation.c`, `paiement.c`, `historique.c` |
| Travailler avec des fichiers texte | `reservations.txt`, `paiements.txt`, `historique.txt`, `users.txt` |
| Nommer le module d'après la structure | `Reservation` -> `reservation.h/.c`, etc. |
| Pas de `printf` / `scanf` dans les fonctions | aucun ; affichage par l'interface Glade |
| Vérifier dans le fichier texte | `make test` : `test_reservation.txt`, `tri_*.txt`, `rapport_*.txt`... (117 vérifications) |

## 7. Fichiers

`reservation.*`, `paiement.*`, `historique.*`, `date.h` (modules) - `test_*.c` - `main.c`, `login.c` (connexion par rôle) - `principal.c` (menu admin) - `gestion_reservations.c`, `paiement_ui.c`, `historique_ui.c` (onglets admin) - **`espace.c` (espace passager)** - `outils.c`, `app.h` - `gestion_reservations.glade` - `Makefile` - données `.txt`.
