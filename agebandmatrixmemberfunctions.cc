#include "agebandmatrix.h"
#include "agebandmatrixptrvector.h"
#include "mathfunc.h"
#include "bandmatrix.h"
#include "doublevector.h"
#include "conversionindex.h"
#include "popinfovector.h"
#include "errorhandler.h"
#include "gadget.h"

extern ErrorHandler handle;

void AgeBandMatrix::Add(const AgeBandMatrix& Addition,
  const ConversionIndex &CI, double ratio, int minaddage, int maxaddage) {

  PopInfo pop;
  minaddage = max(this->minAge(), Addition.minAge(), minaddage);
  maxaddage = min(this->maxAge(), Addition.maxAge(), maxaddage);
  int age, l, minl, maxl, offset;

  if (maxaddage < minaddage)
    return;

  if (CI.SameDl()) {
    offset = CI.Offset();
    for (age = minaddage; age <= maxaddage; age++) {
      minl = max(this->minLength(age), Addition.minLength(age) + offset);
      maxl = min(this->maxLength(age), Addition.maxLength(age) + offset);
      for (l = minl; l < maxl; l++) {
        pop = Addition[age][l - offset];
        pop *= ratio;
        (*v[age - minage])[l] += pop;
      }
    }
  } else {
    if (CI.TargetIsFiner()) {
      for (age = minaddage; age <= maxaddage; age++) {
        minl = max(this->minLength(age), CI.minPos(Addition.minLength(age)));
        maxl = min(this->maxLength(age), CI.maxPos(Addition.maxLength(age) - 1) + 1);
        for (l = minl; l < maxl; l++) {
          pop = Addition[age][CI.Pos(l)];
          pop *= ratio;
          if (isZero(CI.Nrof(l)))
            handle.logWarning("Error in agebandmatrix - divide by zero");
          else
            pop.N /= CI.Nrof(l);
          (*v[age - minage])[l] += pop;
        }
      }
    } else {
      for (age = minaddage; age <= maxaddage; age++) {
        minl = max(CI.minPos(this->minLength(age)), Addition.minLength(age));
        maxl = min(CI.maxPos(this->maxLength(age) - 1) + 1, Addition.maxLength(age));
        if (maxl > minl && CI.Pos(maxl - 1) < this->maxLength(age)
            && CI.Pos(minl) >= this->minLength(age)) {
          for (l = minl; l < maxl; l++) {
            pop = Addition[age][l];
            pop *= ratio;
            (*v[age - minage])[CI.Pos(l)] += pop;
          }
        }
      }
    }
  }
}

void AgeBandMatrix::Multiply(const DoubleVector& Ratio, const ConversionIndex& CI) {
  if (CI.TargetIsFiner() == 1)
    handle.logWarning("Warning in agebandmatrix - target is finer for multiply");

  int i, j, j1, j2;
  DoubleVector UsedRatio(Ratio);
  for (i = 0; i < UsedRatio.Size(); i++) {
    if (isZero(UsedRatio[i]))
      UsedRatio[i] = 0.0;
    else if (UsedRatio[i] < 0) {
      handle.logWarning("Error in agebandmatrix - negative ratio");
      UsedRatio[i] = 0.0;
    }
  }

  if (CI.SameDl()) {
    for (i = 0; i < nrow; i++) {
      j1 = max(v[i]->minCol(), CI.minLength());
      j2 = min(v[i]->maxCol(), CI.maxLength());
      for (j = j1; j < j2; j++)
        (*v[i])[j].N *= UsedRatio[j - CI.Offset()];
    }
  } else {
    for (i = 0; i < nrow; i++) {
      j1 = max(v[i]->minCol(), CI.minLength());
      j2 = min(v[i]->maxCol(), CI.maxLength());
      for (j = j1; j < j2; j++)
        (*v[i])[j].N *= UsedRatio[CI.Pos(j)];
    }
  }
}

void AgeBandMatrix::Subtract(const DoubleVector& Consumption, const ConversionIndex& CI, const PopInfoVector& Nrof) {

  if (Consumption.Size() != Nrof.Size())
    handle.logWarning("Warning in agebandmatrix - different sizes in subtract");

  int i;
  DoubleVector Ratio(Consumption.Size(), 1.0);
  for (i = 0; i < Consumption.Size(); i++) {
    if (Nrof[i].N > verysmall)
      Ratio[i] = 1 - (Consumption[i] / Nrof[i].N);
  }

  this->Multiply(Ratio, CI);
}

//-----------------------------------------------------------------
//Multiply AgeBandMatrix by a age dependent vector for example
//natural mortality. Investigate if Ratio should be allowed to be shorter.
void AgeBandMatrix::Multiply(const DoubleVector& Ratio) {
  if (Ratio.Size() != nrow)
    handle.logWarning("Warning in agebandmatrix - different sizes in multiply");

  int i, j;
  for (i = 0; i < nrow; i++)
    for (j = v[i]->minCol(); j < v[i]->maxCol(); j++)
      (*v[i])[j] *= Ratio[i];
}

//--------------------------------------------------------------
//Find the Column sum of a BandMatrix.  In AgeBandMatrix it means
//summation over all ages for each length.
void AgeBandMatrix::sumColumns(PopInfoVector& Result) const {
  int i, j;
  for (i = 0; i < nrow; i++)
    for (j = v[i]->minCol(); j < v[i]->maxCol(); j++)
      Result[j] += (*v[i])[j];
}

//This function increases the age.  When moving from one age class to
//another only the intersection of the agegroups of the length
//classes is moved.  This could possibly be improved later on.
void AgeBandMatrix::IncrementAge() {
  int i, j, j1, j2;

  if (nrow <= 1)
    return;  //No ageing takes place where there is only one age.

  i = nrow - 1;
  j1 = max(v[i]->minCol(), v[i - 1]->minCol());
  j2 = min(v[i]->maxCol(), v[i - 1]->maxCol());
  //For the highest age group
  for (j = j1; j < j2; j++)
    (*v[i])[j] += (*v[i - 1])[j];
  for (j = v[i - 1]->minCol(); j < v[i - 1]->maxCol(); j++) {
    (*v[i - 1])[j].N = 0.0;
    (*v[i - 1])[j].W = 0.0;
  }
  //Now v[nrow-2] has been added to v[nrow-1] and then set to 0.

  //For the other age groups.
  //At the end of each for (i=nrow-2...) loop, the intersection of v[i-1] with
  //v[i] has been copied from v[i-1] to v[i] and v[i-1] has been set to 0.
  for (i = nrow - 2; i > 0; i--) {
    j1 = max(v[i]->minCol(), v[i - 1]->minCol());
    j2 = min(v[i]->maxCol(), v[i - 1]->maxCol());
    for (j = v[i - 1]->minCol(); j < j1; j++) {
      (*v[i - 1])[j].N = 0.0;
      (*v[i - 1])[j].W = 0.0;
    }
    for (j = j1; j < j2; j++) {
      (*v[i])[j] = (*v[i - 1])[j];
      (*v[i - 1])[j].N = 0.0;
      (*v[i - 1])[j].W = 0.0;
    }
    for (j = j2; j < v[i - 1]->maxCol(); j++) {
      (*v[i - 1])[j].N = 0.0;
      (*v[i - 1])[j].W = 0.0;
    }
  }

  //set number in age zero to zero.
  for (j = v[0]->minCol(); j < v[0]->maxCol(); j++) {
    (*v[0])[j].N = 0.0;
    (*v[0])[j].W = 0.0;
  }
}

void AgeBandMatrix::setToZero() {
  int i, j;
  for (i = 0; i < nrow; i++)
    for (j = v[i]->minCol(); j < v[i]->maxCol(); j++) {
      (*v[i])[j].N = 0.0;
      (*v[i])[j].W = 0.0;
    }
}

void AgeBandMatrix::FilterN(double minN) {
  int i, j;
  for (i = 0; i < nrow; i++)
    for (j = v[i]->minCol(); j < v[i]->maxCol(); j++)
      if ((*v[i])[j].N < minN) {
        (*v[i])[j].N = 0.0;
        (*v[i])[j].W = 0.0;
      }
}

void AgeBandMatrix::printNumbers(ofstream& outfile) const {
  int i, j;
  int maxcol = 0;
  for (i = minage; i < minage + nrow; i++)
    if (v[i - minage]->maxCol() > maxcol)
      maxcol = v[i - minage]->maxCol();

  for (i = minage; i < minage + nrow; i++) {
    outfile << TAB;
    if (v[i - minage]->minCol() > 0) {
      for (j = 0; j < v[i - minage]->minCol(); j++) {
        outfile.precision(smallprecision);
        outfile.width(smallwidth);
        outfile << 0.0 << sep;
      }
    }
    for (j = v[i - minage]->minCol(); j < v[i - minage]->maxCol(); j++) {
      outfile.precision(smallprecision);
      outfile.width(smallwidth);
      outfile << (*v[i - minage])[j].N << sep;
    }
    if (v[i - minage]->maxCol() < maxcol) {
      for (j = v[i - minage]->maxCol(); j < maxcol; j++) {
        outfile.precision(smallprecision);
        outfile.width(smallwidth);
        outfile << 0.0 << sep;
      }
    }
    outfile << endl;
  }
}

void AgeBandMatrix::printWeights(ofstream& outfile) const {
  int i, j;
  int maxcol = 0;
  for (i = minage; i < minage + nrow; i++)
    if (v[i - minage]->maxCol() > maxcol)
      maxcol = v[i - minage]->maxCol();

  for (i = minage; i < minage + nrow; i++) {
    outfile << TAB;
    if (v[i - minage]->minCol() > 0) {
      for (j = 0; j < v[i - minage]->minCol(); j++) {
        outfile.precision(smallprecision);
        outfile.width(smallwidth);
        outfile << 0.0 << sep;
      }
    }
    for (j = v[i - minage]->minCol(); j < v[i - minage]->maxCol(); j++) {
      outfile.precision(smallprecision);
      outfile.width(smallwidth);
      outfile << (*v[i - minage])[j].W << sep;
    }
    if (v[i - minage]->maxCol() < maxcol) {
      for (j = v[i - minage]->maxCol(); j < maxcol; j++) {
        outfile.precision(smallprecision);
        outfile.width(smallwidth);
        outfile << 0.0 << sep;
      }
    }
    outfile << endl;
  }
}

void AgeBandMatrixPtrVector::Migrate(const DoubleMatrix& MI) {
  assert(MI.Nrow() == size);
  PopInfoVector tmp(size);
  int i, j, age, length;

  for (age = v[0]->minAge(); age <= v[0]->maxAge(); age++) {
    for (length = v[0]->minLength(age); length < v[0]->maxLength(age); length++) {
      for (j = 0; j < size; j++) {
        tmp[j].N = 0.0;
        tmp[j].W = 0.0;
      }
      //Let tmp[j] keep the population of agelength group on area j after the migration
      for (j = 0; j < size; j++)
        for (i = 0; i < size; i++)
          tmp[j] += (*v[i])[age][length] * MI[j][i];

      for (j = 0; j < size; j++)
        (*v[j])[age][length] = tmp[j];
    }
  }
}
