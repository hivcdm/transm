#ifndef INFECTIONRISK_HPP
#define INFECTIONRISK_HPP

#include <iostream>

#include "entitytypes.hpp"

namespace transm {

/**
 * The protection PrEP gives a user at an adherence level: the configured efficacy
 * scaled by how much of it that adherence keeps. Off PrEP there is no protection.
 * An unknown level is reported and keeps the full efficacy, as it always has. */
inline double PrepEfficacyForAdherence(double efficacy, PrepAherenceLevel adherence)
{
    switch (adherence) {
    case PrepAherenceLevel::PREP_ADHERENT:               return efficacy * 0.99;
    case PrepAherenceLevel::PREP_SUBSTANTIALLY_ADHERENT: return efficacy * 0.80;
    case PrepAherenceLevel::PREP_PARTIALLY_ADHERENT:     return efficacy * 0.10;
    case PrepAherenceLevel::PREP_INADHERENT:             return efficacy * 0.05;
    case PrepAherenceLevel::PREP_OFF_PREP:               return efficacy * 0.0;
    }
    std::cout << "Invalid adherence level" << std::endl;
    return efficacy;
}

/**
 * Chance of infection from one partner in a month: the transmission coefficient for
 * the infector's viral load, reduced by each protection independently.
 *
 * @param condomEfficacy protection from a condom, 0 if none was used
 * @param otherEfficacy  microbicide (women) or circumcision (men)
 * @param prepEfficacy   from PrepEfficacyForAdherence */
inline double ForceOfInfection(double coefficient, double condomEfficacy, double otherEfficacy, double prepEfficacy)
{
    return coefficient * (1 - condomEfficacy) * (1 - otherEfficacy) * (1 - prepEfficacy);
}

} // namespace transm

#endif /* INFECTIONRISK_HPP */
