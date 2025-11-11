#ifndef VECTORSTRUCTURES_H
#define VECTORSTRUCTURES_H

#include <QString>


struct Transition {
    // QString AZ;
    double level;
    double emission;
    QString label;
};

struct Level {
    // QString AZ;
    double lvlEnergy;
    QString spin;
    QString halfLife;
};


#endif // VECTORSTRUCTURES_H

