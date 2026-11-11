#ifndef DATE_H
#define DATE_H

/* date.h - structure Date partagée par les modules avion et maintenance */
typedef struct {
    int jour;
    int mois;
    int annee;
} Date;

#endif
