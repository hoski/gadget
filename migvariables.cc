#include "migvariables.h"
#include "errorhandler.h"
#include "gadget.h"

extern ErrorHandler handle;

MigVariable::MigVariable(CommentStream& infile, int firstyear, int lastyear,
  Keeper* const keeper) : ValuesReadFromFile(1) {

  int i;
  keeper->addString("migvariable");
  char text[MaxStrLength];
  strncpy(text, "", MaxStrLength);
  infile >> text;
  if (strcasecmp(text, "coeff") != 0)
    handle.Unexpected("coeff", text);
  coeff.resize(4, keeper);
  infile >> coeff;
  coeff.Inform(keeper);
  if (coeff[3] <= coeff[2])
    handle.Message("Minvalue > Maxvalue");
  infile >> ws >> text;
  if (strcasecmp(text, "data") != 0)
    handle.Unexpected("data", text);
  int NumberOfYears = lastyear - firstyear + 1;
  years.resize(NumberOfYears);
  temperature.resize(NumberOfYears);
  values.resize(NumberOfYears, keeper);
  for (i = 0; i < NumberOfYears; i++) {
    infile >> years[i] >> temperature[i] >> values[i];
    if (infile.fail() || infile.eof())
      handle.Message("End of file or error in file");
    values[i].Inform(keeper);
    if ((i > 0) && (years[i] - years[i - 1] != 1))
      handle.Message("Difference between adjacent years not one");
  }
}

MigVariable::~MigVariable() {
}

MigVariable::MigVariable(Formula* const number, int firstyear, int lastyear,
  Keeper* const keeper) : ValuesReadFromFile(0) {

  number->Interchange(value, keeper);
}

double MigVariable::ValueOfVariable(int year) {
  double result;
  int i;
  if (ValuesReadFromFile) {
    i = year - years[0];
    result = coeff[0] * values[i] + coeff[1] * temperature[i];
    if (result < coeff[2])
      result = coeff[2];
    if (result > coeff[3])
      result = coeff[3];
  } else
    result = value;
  return result;
}
