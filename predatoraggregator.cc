#include "predatoraggregator.h"
#include "prey.h"
#include "mortprey.h"
#include "predator.h"
#include "mathfunc.h"
#include "popinfovector.h"
#include "errorhandler.h"
#include "gadget.h"

extern ErrorHandler handle;

PredatorAggregator::PredatorAggregator(const PredatorPtrVector& Predators,
  const PreyPtrVector& Preys, const IntMatrix& Areas,
  const LengthGroupDivision* const predLgrpDiv, const LengthGroupDivision* const preyLgrpDiv)
  : predators(Predators), preys(Preys), areas(Areas), doeseat(Predators.Size(), Preys.Size(), 0) {

  int i, j, k;

  for (i = 0; i < predators.Size(); i++)
    checkLengthGroupIsFiner(predators[i]->returnLengthGroupDiv(), predLgrpDiv);

  for (i = 0; i < preys.Size(); i++)
    checkLengthGroupIsFiner(preys[i]->returnLengthGroupDiv(), preyLgrpDiv);

  for (i = 0; i < predators.Size(); i++) {
    predConv.AddRows(1, predators[i]->NoLengthGroups(), 0);
    for (j = 0; j < predConv.Ncol(i); j++)
      predConv[i][j] = predLgrpDiv->NoLengthGroup(predators[i]->Length(j));
  }
  for (i = 0; i < preys.Size(); i++) {
    preyConv.AddRows(1, preys[i]->NoLengthGroups(), 0);
    for (j = 0; j < preyConv.Ncol(i); j++)
      preyConv[i][j] = preyLgrpDiv->NoLengthGroup(preys[i]->Length(j));
  }

  for (i = 0; i < predators.Size(); i++)
    for (j = 0; j < preys.Size(); j++)
      if (predators[i]->doesEat(preys[j]->Name()))
        doeseat[i][j] = 1;

  DoubleMatrix dm(predLgrpDiv->NoLengthGroups(), preyLgrpDiv->NoLengthGroups(), 1.0);
  BandMatrix bm(dm, 0, 0);
  total.resize(areas.Nrow(), bm);
  //Now total is initialised to 1, change it to 0.
  for (i = 0; i < total.Size(); i++)
    for (j = 0; j < total[i].Nrow(); j++)
      for (k = 0; k < total[i].Ncol(j); k++)
        total[i][j][k] = 0.0;
}

PredatorAggregator::PredatorAggregator(const CharPtrVector& prednames, PreyPtrVector& Preys,
  const IntMatrix& Areas, const IntVector& ages, const LengthGroupDivision* const preyLgrpDiv)
  : areas(Areas) {

  //written by kgf 19/2 99
  //modified by kgf 4/3 99
  //The purpose of this constructor is to construct a data set that have the
  //predator's age and the predator's length as "dimensions". The consum due
  //to cannibalism is stored in prey, and a single species model is supposed.
  //Possible predators are the immature and the mature parts of the stock.
  //pred_names are the name of the predator stocks that are read from file.
  //Note that the last age read from file always is considered a plus group.

  int i, j, k, n;
  int found, minage, maxage, preds, predtot;

  found = 0;
  for (i = 0; i <  Preys.Size(); i++)
    if (((MortPrey*)Preys[i])->cannIsTrue()) {
      preys.resize(1, Preys[i]);
      found++;
    }

  if (found == 0) {
    handle.logWarning("Warning in predatoraggregator - failed to match preys with cannibalism");
    return;
  }
  if (found < Preys.Size())
    handle.logWarning("Warning in predatoraggregator - failed to match preys");

  for (i = 0; i < preys.Size(); i++)
    doeseat.AddRows(1, ((MortPrey*)preys[i])->getNoCannPreds(), 0);

  //Check to see if the predators read from files are found in the predator list in preys.
  for (i = 0; i < preys.Size(); i++) {
    found = 0;
    for (j = 0; j < doeseat.Ncol(i); j++)
      for (k = 0; k < prednames.Size(); k++)
        if (strcasecmp(((MortPrey*)preys[i])->cannPredName(j), prednames[k]) == 0) {
          found++;
          doeseat[i][j] = 1;
        }

    if (found == 0)
      handle.logWarning("Warning in predatoraggregator - failed to match predators");
  }

  //First we check that the predator ages are consistent with the predator
  //ages used in Cannibalism. It has to be some common ages, and the highest
  //age read from file must not exceed the highest predator age in Cannibalism.
  found = 0;
  for (i = 0; i < preys.Size(); i++) {
    if (ages[0] > ((MortPrey*)preys[i])->maxPredAge() ||
        ages[ages.Size() - 1] < ((MortPrey*)preys[i])->minPredAge()) //no intercept
      found = 1;
    if (ages[ages.Size() - 1] > ((MortPrey*)preys[i])->maxPredAge())
      found = 1;
  }
  if (found == 1)
    handle.logWarning("Warning in predatoraggregator - failed to match predator ages");

  for (i = 0; i < preys.Size(); i++)
    checkLengthGroupIsFiner(preys[i]->returnLengthGroupDiv(), preyLgrpDiv);

  predtot = 0;
  for (i = 0; i < preys.Size(); i++) {
    preds = ((MortPrey*)preys[i])->getNoCannPreds();
    for (j = 0; j < preds; j++) {
      minage = ((MortPrey*)preys[i])->getPredMinAge(j);
      maxage = ((MortPrey*)preys[i])->getPredMaxAge(j);
      predConv.AddRows(1, maxage - minage + 1, 0);
      for (n = 0; n < predConv.Ncol(predtot + j); n++) {
        if ((minage + n) < ages[0])  //out of range
          predConv[predtot + j][n] = -1;
        else {
          k = 0;
          while (k < ages.Size() && ((n + minage) != ages[k]))
            k++;
          if (k == ages.Size()) //treat as plus group
            predConv[predtot + j][n] = ages.Size() - 1;
          else
            predConv[predtot + j][n] = k;
        }
      }
    }
    predtot += preds;
  }

  for (i = 0; i < preys.Size(); i++) {
    preyConv.AddRows(1, preys[i]->NoLengthGroups(), 0);
    for (j = 0; j < preyConv.Ncol(i); j++)
      preyConv[i][j] = preyLgrpDiv->NoLengthGroup(preys[i]->Length(j));
  }

  DoubleMatrix dm(ages.Size(), preyLgrpDiv->NoLengthGroups(), 1.0);
  BandMatrix bm(dm, 0, 0);
  total.resize(areas.Nrow(), bm);
  //Now total is initialised to 1, change it to 0.
  for (i = 0; i < total.Size(); i++)
    for (j = 0; j < total[i].Nrow(); j++)
      for (k = 0; k < total[i].Ncol(j); k++)
        total[i][j][k] = 0.0;
}

void PredatorAggregator::Sum() {
  int g, h, i, j, k, l;
  int area, predLength, preyLength;

  for (i = 0; i < total.Size(); i++)
    for (j = 0; j < total[i].Nrow(); j++)
      for (k = 0; k < total[i].Ncol(j); k++)
        total[i][j][k] = 0.0;

  //Sum over the appropriate preys, predators, areas, and lengths.
  for (g = 0; g < predators.Size(); g++) {
    for (h = 0; h < preys.Size(); h++) {
      if (doeseat[g][h]) {
        for (l = 0; l < areas.Nrow(); l++) {
          for (j = 0; j < areas.Ncol(l); j++) {
            area = areas[l][j];
            if (predators[g]->IsInArea(area) && preys[h]->IsInArea(area)) {
              const BandMatrix* bptr = &predators[g]->Consumption(area, preys[h]->Name());
              for (k = bptr->Minrow(); k <= bptr->Maxrow(); k++) {
                predLength = predConv[g][k];
                if (predLength >= 0) {
                  for (i = bptr->Mincol(k); i < bptr->Maxcol(k); i++) {
                    preyLength = preyConv[h][i];
                    if (preyLength >= 0)
                      total[l][predLength][preyLength] += (*bptr)[k][i];
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

void PredatorAggregator::MeanSum() {
  //written by kgf 19/2 99
  //Modified by kgf 5/3 99
  //To be used together with the second constructor.
  //prey must be of type MortPrey, i.e. mortality model must be used.

  int g, h, i, j, k, l, g1;
  int pred_age, prey_length;
  int area, minrow, maxrow;
  DoubleMatrix tot_predators;

  for (i = 0; i < total.Size(); i++) {
    tot_predators.AddRows(1, total[i].Nrow(), 0.0);
    for (j = 0; j < total[i].Nrow(); j++)
      for (k = 0; k < total[i].Ncol(j); k++)
        total[i][j][k] = 0.0;
  }

  //Sum over the appropriate preys, predators, areas, ages and lengths.
  g1 = 0;
  for (g = 0; g < preys.Size(); g++) {
    for (h = 0; h < ((MortPrey*)preys[g])->getNoCannPreds(); h++) {
      if (doeseat[g][h]) {
        for (i = 0; i < areas.Nrow(); i++) {
          for (j = 0; j < areas.Ncol(i); j++) {
            area = areas[i][j];
            if (preys[g]->IsInArea(area)) {
              const BandMatrix* bptr = &((MortPrey*)preys[g])->cannConsum(area, h);
              minrow = bptr->Minrow();
              maxrow = bptr->Maxrow();
              for (k = minrow; k <= maxrow; k++) {
                pred_age = predConv[g1 + h][k - minrow];
                if (pred_age >= 0) {
                  tot_predators[i][pred_age] +=
                    (*((MortPrey*)preys[g])->ageGroupMatrix(h))[area][k - minrow];
                  for (l = bptr->Mincol(k); l < bptr->Maxcol(k); l++) {
                    prey_length = preyConv[g][l];
                    if (prey_length >= 0)
                      total[i][pred_age][prey_length] += (*bptr)[k][l];
                  }
                }
              }
            }
          }
        }
      }
    }
    g1 += ((MortPrey*)preys[g])->getNoCannPreds();
  }

  for (i = 0; i < total.Size(); i++) //get average consumption pr fish
    for (j = 0; j < total[i].Nrow(); j++)
      for (k = 0; k < total[i].Ncol(j); k++) {
        if (tot_predators[i][j] > 0) //average consumption pr fish
          total[i][j][k] /= tot_predators[i][j];
        else
          if (total[i][j][k] > 0 && isZero(tot_predators[i][j]))
            handle.logWarning("Warning in predatoraggregator - consumption without predators");
      }
}

//Sum the numbers (not biomass)
void PredatorAggregator::NumberSum() {
  int i, j, k, g, h, l;
  int area, predLength, preyLength;

  for (i = 0; i < total.Size(); i++)
    for (j = 0; j < total[i].Nrow(); j++)
      for (k = 0; k < total[i].Ncol(j); k++)
        total[i][j][k] = 0.0;

  //Sum over the appropriate preys, predators, areas, and lengths.
  for (g = 0; g < predators.Size(); g++) {
    for (h = 0; h < preys.Size(); h++) {
      if (doeseat[g][h]) {
        for (l = 0; l < areas.Nrow(); l++) {
          for (j = 0; j < areas.Ncol(l); j++) {
            area = areas[l][j];
            if (predators[g]->IsInArea(area) && preys[h]->IsInArea(area)) {
              const BandMatrix* bptr = &predators[g]->Consumption(area, preys[h]->Name());
              const PopInfoVector* preymeanw = &predators[g]->NumberPriortoEating(area, preys[h]->Name());
              for (k = bptr->Minrow(); k <= bptr->Maxrow(); k++) {
                predLength = predConv[g][k];
                if (predLength >= 0) {
                  for (i = bptr->Mincol(k); i < bptr->Maxcol(k); i++) {
                    preyLength = preyConv[h][i];
                    if (preyLength >= 0 && (*preymeanw)[i].W > 0)
                      total[l][predLength][preyLength] += (*bptr)[k][i] / (*preymeanw)[i].W;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}
