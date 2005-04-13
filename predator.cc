#include "mathfunc.h"
#include "keeper.h"
#include "prey.h"
#include "suits.h"
#include "predator.h"
#include "errorhandler.h"
#include "readfunc.h"
#include "gadget.h"

extern ErrorHandler handle;

Predator::Predator(const char* givenname, const IntVector& Areas)
  : HasName(givenname), LivesOnAreas(Areas), Suitable(0) {
}

Predator::~Predator() {
  int i;
  for (i = 0; i < Suitable->numPreys(); i++)
    delete Suitable->FuncPrey(i);
  delete Suitable;
}

void Predator::setPrey(PreyPtrVector& preyvec, Keeper* const keeper) {
  int i, j;
  int found = 0;

  preys.resize(this->numPreys(), 0);
  for (i = 0; i < preyvec.Size(); i++) {
    found = 0;
    for (j = 0; j < this->numPreys(); j++) {
      if (strcasecmp(this->Preyname(j), preyvec[i]->getName()) == 0) {
        if (found == 0) {
          preys[j] = preyvec[i];
          found++;
        } else
          handle.logFailure("Error in predator - repeated suitability values for prey", preyvec[i]->getName());

      }
    }
  }

  found = 0;
  for (i = 0; i < preys.Size(); i++) {
    //If we find a prey that we have read the suitability for, but not
    //received a pointer to, we issue a warning and delete it
    if (preys[i] == 0) {
      found++;
      handle.logWarning("Warning in predator - failed to match prey", this->Preyname(i));
      this->DeleteParametersForPrey(i, keeper);
      //This function allows derived classes to delete the information they keep.
      //Since we have deleted element no. i from the vectors, we must take
      //care not to miss the element that is now no. i
      if (found != preys.Size())
        i--;
    }
  }
  if (this->numPreys() == 0)
    handle.logFailure("Error in predator - no preys for predator", this->getName());
}

int Predator::doesEat(const char* preyname) const {
  int i;
  for (i = 0; i < this->numPreys(); i++)
    if (strcasecmp(this->Preyname(i), preyname) == 0)
      return 1;
  return 0;
}

void Predator::Print(ofstream& outfile) const {
  int i;
  outfile << "\tName" << sep << this->getName()
    << "\n\tNames of preys:";
  for (i = 0; i < this->numPreys(); i++)
    outfile << sep << this->Preyname(i);
  outfile << endl;
  for (i = 0; i < this->numPreys(); i++) {
    outfile << "\tSuitability for " << this->Preyname(i) << endl;
    this->Suitability(i).Print(outfile);
  }
}

void Predator::Reset(const TimeClass* const TimeInfo) {
  Suitable->Reset(this, TimeInfo);
}

void Predator::DeleteParametersForPrey(int p, Keeper* const keeper) {
  /* This class has set a data invariant for the protected variables
   * that derived classes are to keep.
   * This function bases its action on that data invariant.
   * Derived classes that need to delete information when a prey is
   * deleted should do so in this function.
   * Explanation of parameters: int p is the number of the element
   * of this->Preyname() that keeps the name of the prey that is to be deleted.*/
  if ((p < 0) || (p > preys.Size()))
    handle.logFailure("Error in predator - invalid prey to delete from predator", this->getName());
  preys.Delete(p);
  Suitable->DeletePrey(p, keeper);
}

void Predator::readSuitability(CommentStream& infile,
  const char* FinalString, const TimeClass* const TimeInfo, Keeper* const keeper) {

  int i, j;
  char preyname[MaxStrLength];
  char text[MaxStrLength];
  strncpy(preyname, "", MaxStrLength);
  strncpy(text, "", MaxStrLength);
  Suitable = new Suits();
  SuitFuncPtrVector suitf;
  keeper->addString("suitabilityfor");

  infile >> preyname >> ws;
  while (!(strcasecmp(preyname, FinalString) == 0) && (!infile.eof())) {
    keeper->addString(preyname);

    infile >> text >> ws;
    if (strcasecmp(text, "function") == 0) {
      infile >> text >> ws;
      if (readSuitFunction(suitf, infile, text, TimeInfo, keeper) == 1)
        Suitable->addPrey(preyname, suitf[suitf.Size() - 1]);
      else
        handle.Message("Error in suitability - unrecognised suitability function");

    } else if (strcasecmp(text, "suitfile") == 0) {
      handle.Message("Reading suitability values directly from file is no longer supported\nGadget version 2.0.07 was the last version to allow this functionality");

    } else
      handle.Message("Error in suitability - unrecognised format", text);

    infile >> preyname >> ws;
    keeper->clearLast();
  }

  keeper->clearLast();
  if (!infile.good())
    handle.Failure();
}
