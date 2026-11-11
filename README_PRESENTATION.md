# Présentation séance 4 - Gestion des réservations (avec Paiements et Historique)

Trois modules, chacun nommé d'après sa structure : **`reservation`**, **`paiement`**, **`historique`** (`.h` + `.c`), plus `date.h` et `main.c`.

---

## 1. Les consignes du devoir et où elles sont respectées

| Consigne | Notre réponse |
|---|---|
| Implémenter les fonctions demandées | ajouter, modifier, supprimer, chercher + recherche, tri, comptage, contrôle de saisie dans chaque module |
| Travailler avec des fichiers texte | `reservations.txt`, `paiements.txt`, `historique.txt` : une ligne par enregistrement, champs séparés par `;` |
| Nommer le module d'après la structure | `Reservation` -> `reservation.h/.c` ; `Paiement` -> `paiement.h/.c` ; `Historique` -> `historique.h/.c` |
| Jamais de `printf` / `scanf` dans les fonctions | aucune dans les trois modules : les fonctions **retournent un code** (1, 0, -1...) et l'interface décide quoi afficher |
| Vérifier directement dans le fichier texte, sans implémenter l'affichage | `test_reservation.c`, `test_paiement.c`, `test_historique.c` écrivent des fichiers `.txt` / `.csv` qu'on ouvre ; l'affichage est fait avec des **GtkTreeView** |

---

## 2. Vue d'ensemble

```
date.h              -> structure Date (jour, mois, année) partagée
reservation.h / .c  -> module Reservation (fichier reservations.txt)
paiement.h / .c     -> module Paiement    (fichier paiements.txt)
historique.h / .c   -> module Historique  (fichier historique.txt)
main.c              -> point d'entrée : lance GTK et la fenêtre de connexion
test_*.c            -> tests des modules, sans interface
login.c, principal.c, gestion_reservations.c, paiement_ui.c, historique_ui.c -> interface GTK
```

```
interface (GTK) --appelle--> modules .c --lisent/écrivent--> fichiers .txt
```

Les trois modules ne connaissent **ni GTK ni l'écran** : c'est ce qui permet de les tester seuls.

### Comment les modules travaillent ensemble

```
Paiement de 830 TND pour la réservation R003 (reste à payer : 830 TND)
   -> controler_paiement()                  accepte : la réservation existe, n'est pas annulée, le montant ne dépasse pas le reste
   -> ajouter_paiement()                    écrit dans paiements.txt
   -> synchroniser_statut_reservation()     R003 passe de "En attente" à "Confirmée" dans reservations.txt
   -> ajouter_historique()                  2 lignes ajoutées dans historique.txt
```

---

## 3. Module Reservation (`reservation.h` / `reservation.c`)

```c
typedef struct {
    char id[12];         /* identifiant unique, ex : R001 */
    char passager[30];
    char cin[9];         /* 8 chiffres */
    char vol[10];        /* ex : CA101 */
    Date date;           /* date de la réservation */
    char classe[16];     /* Économique / Affaires / Première */
    int  places;         /* 1 à 9 */
    int  prix;           /* prix d'une place en TND */
    char statut[16];     /* Confirmée / En attente / Annulée */
} Reservation;
```

| Fonction | Rôle | Retour |
|---|---|---|
| `ajouter_reservation` | ajoute une réservation | 1 ok / 0 identifiant déjà utilisé / -1 erreur fichier |
| `modifier_reservation` | modifie une réservation | 1 / 0 introuvable / -1 |
| `supprimer_reservation` | supprime une réservation | 1 / 0 introuvable / -1 |
| `chercher_reservation` | retourne une réservation | `id` vide si introuvable |
| `charger_reservations` / `sauvegarder_reservations` | lit / réécrit tout le fichier | nombre lu (-1 si absent) / 1 ou -1 |
| `filtrer_reservations`, `trier_reservations`, `compter_statut`, `montant_total` | recherche, tri par passager, statistiques, places x prix | nombre / - / entier |
| **`changer_statut_reservation`** (nouveau) | change seulement le statut | 1 / 0 introuvable / -1 |
| **`exporter_reservations`** (nouveau) | écrit un fichier `.csv` (Excel) | 1 / -1 |
| `valider_reservation`, `message_erreur_reservation` | contrôle de saisie (codes 1 à 9) | 0 = valide |

---

## 4. Module Paiement (`paiement.h` / `paiement.c`) - NOUVEAU

```c
typedef struct {
    char id[12];           /* identifiant unique, ex : P001 */
    char reservation[12];  /* réservation payée */
    Date date;             /* date du paiement */
    int  montant;          /* montant versé en TND */
    char mode[16];         /* Espèces / Carte bancaire / Virement / Chèque */
    char remarque[60];     /* facultatif */
} Paiement;
```

Format d'une ligne : `P003;R003;04/11/2026;400;Espèces;Acompte`

| Fonction | Rôle | Retour |
|---|---|---|
| `ajouter_paiement` | ajoute un paiement | 1 / 0 identifiant déjà utilisé / -1 |
| `modifier_paiement` | modifie (l'identifiant ne change pas) | 1 / 0 / -1 |
| `supprimer_paiement` | supprime un paiement | 1 / 0 / -1 |
| `supprimer_paiements_reservation` | supprime tous les paiements d'une réservation | nombre supprimé |
| `chercher_paiement` | retourne un paiement | `id` vide si introuvable |
| `filtrer_paiements`, `trier_paiements` | recherche + filtre par mode, tri par date | nombre / - |
| `total_paye`, `somme_paiements`, `compter_mode` | calculs | entier |
| **`reste_a_payer`** | montant de la réservation - déjà payé | entier (-1 si introuvable) |
| **`controler_paiement`** | vérifie qu'on peut enregistrer le paiement | 0 possible / 11 / 12 / 13 |
| **`synchroniser_statut_reservation`** | « En attente » entièrement payée -> « Confirmée » | 1 si changé |
| `valider_paiement`, `message_erreur_paiement` | contrôle de saisie (codes 1 à 5) | 0 = valide |

### Points à expliquer

- **Un paiement dépend d'une réservation** : `paiement.h` inclut `reservation.h` et appelle `chercher_reservation` et `montant_total`. Le module Paiement utilise donc le module Reservation (le contraire n'existe pas).
- **Reste à payer** : `montant_total(réservation) - total_paye(paiements)`. On peut payer en plusieurs fois (acompte puis solde).
- **`controler_paiement`** : 11 = réservation introuvable, 12 = réservation annulée, 13 = le montant dépasse le reste. Le paramètre `exclu` sert à la **modification** : on ignore l'ancien montant du paiement qu'on est en train de corriger.
- **Règle de cohérence** (`synchroniser_statut_reservation`) : seulement vers le haut (En attente -> Confirmée). Une réservation annulée n'est jamais modifiée.
- **Codes de `valider_paiement`** : 1 identifiant, 2 réservation non choisie, 3 date invalide, 4 montant inférieur à 1, 5 mode invalide.

---

## 5. Module Historique (`historique.h` / `historique.c`) - NOUVEAU

```c
typedef struct {
    char date[20];          /* "10/11/2026 14:35:02" */
    char utilisateur[20];   /* qui a fait l'action */
    char reference[12];     /* réservation concernée */
    char action[16];        /* Ajout / Modification / Suppression / Paiement / Export */
    char details[80];
} Historique;
```

Format : `04/11/2026 11:02:30;admin;R003;Paiement;Paiement P003 de 400 TND (Espèces)`

| Fonction | Rôle | Retour |
|---|---|---|
| `ajouter_historique(fichier, utilisateur, reference, action, details)` | ajoute une ligne **datée automatiquement** (`time`, `localtime`, `strftime`) | 1 / -1 |
| `enregistrer_historique(fichier, h)` | même chose avec une date choisie (pour les tests) | 1 / -1 |
| `charger_historique` | lit le journal | nombre (-1 si absent) |
| `filtrer_historique(tab, n, res, texte, action)` | recherche dans date, utilisateur, référence, détails + filtre d'action | nombre |
| `compter_action`, `vider_historique`, `exporter_historique` | comptage, effacement, export `.csv` | entier / 1 ou -1 |

Le journal est écrit en mode `"a"` (ajout à la fin) : on ne modifie jamais les lignes existantes. Seule `vider_historique` efface (mode `"w"`).

---

## 6. `main.c` et l'interface

```c
int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);   /* initialise GTK */
    ouvrir_login();           /* fenêtre de connexion */
    gtk_main();               /* boucle d'événements */
    return 0;
}
```

Parcours : **connexion -> menu principal (tableau de bord) -> fenêtre de gestion à 4 onglets** (Réservations, Paiements, Historique, Statistiques).
`gestion_reservations.c`, `paiement_ui.c` et `historique_ui.c` sont les trois onglets de la même fenêtre ; ils appellent uniquement les fonctions des modules. La fonction `historiser(...)` appelée après chaque action écrit dans `historique.txt`.

---

## 7. Tests (sans interface)

| Programme | Vérifications | Fichiers à ouvrir |
|---|---|---|
| `test_reservation` | 43 | `test_reservation.txt`, `tri_reservation.txt`, `export_reservation.csv`, `rapport_reservation.txt` |
| `test_paiement` | 54 | `test_paiement.txt`, `tri_paiement.txt`, `export_paiement.csv`, `rapport_paiement.txt` |
| `test_historique` | 20 | `test_historique.txt`, `export_historique.csv`, `rapport_historique.txt` |

`make test` lance les trois : chaque ligne `code retour = 0` signifie que tout est correct. Chaque rapport se termine par `N verifications, 0 echec(s)`.

---

## 8. Déroulé conseillé (7 à 10 minutes)

1. **Consignes** (30 s) : tableau du chapitre 1.
2. **Architecture** (1 min) : les trois modules, le schéma du chapitre 2.
3. **Reservation** (1 min) : structure et prototypes ; rappeler l'absence de `printf`.
4. **Paiement** (2 min) : structure, `reste_a_payer`, `controler_paiement`, `synchroniser_statut_reservation`.
5. **Historique** (1 min) : date automatique, mode `"a"`.
6. **Tests** (1 min) : `make test`, ouvrir un `rapport_*.txt`.
7. **Démonstration** (2 min) : `./gestion_reservations`, `admin` / `admin123` ; dans *Paiements*, choisir la réservation R003 (en attente, reste 830 TND), essayer 900 (refusé : dépasse le reste), puis 830 : montrer dans *Réservations* qu'elle est « Confirmée » avec Reste = 0, dans *Historique* les deux lignes ajoutées, et ouvrir `paiements.txt` / `historique.txt`.

---

## 9. Questions possibles du professeur

**Pourquoi trois modules et pas un seul ?** Une structure = un module. Chacun a son fichier, ses fonctions et ses tests.

**Pourquoi `date.h` à part ?** `Date` sert à `Reservation` et à `Paiement` ; un fichier commun évite la double définition.

**Comment éviter qu'un client paie plus que le montant ?** `controler_paiement` compare ce qui est déjà payé + le nouveau montant au montant de la réservation (code 13).

**Pourquoi le paramètre `exclu` ?** En modifiant un paiement, son ancien montant ne doit pas être compté deux fois.

**Pourquoi le statut est-il modifié dans le module et pas dans l'interface ?** Pour que la règle soit testable sans interface (`test_paiement.c`) et réutilisable.

**Pourquoi `"a"` pour ajouter et `"w"` pour sauvegarder ?** `"a"` écrit à la fin sans effacer ; `"w"` efface et réécrit tout (modification, suppression).

**Que se passe-t-il à la suppression d'une réservation ?** Ses paiements sont supprimés (`supprimer_paiements_reservation`), l'action est notée dans l'historique.

**Que se passe-t-il si on tape un `;` dans un champ ?** `nettoyer` le remplace par `,` pour ne pas décaler les champs du fichier.

**Où est l'affichage ?** Dans `gestion_reservations.c`, `paiement_ui.c` et `historique_ui.c` (GtkTreeView). Les modules ne font aucun affichage.

---

## 10. Compiler et lancer

```bash
sudo apt install build-essential libgtk2.0-dev libglade2-dev   # une seule fois
cd gestion_reservations
make                         # compile
./gestion_reservations       # admin / admin123
make test                    # teste les 3 modules sans interface
make clean
```
