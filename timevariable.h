#ifndef timevariable_h
#define timevariable_h

#include "formulavector.h"
#include "commentstream.h"

class TimeVariable {
public:
  TimeVariable();
  void read(CommentStream& infile, const TimeClass* const TimeInfo, Keeper* const keeper);
  void readFromFile(CommentStream& infile, const TimeClass* const TimeInfo, Keeper* const keeper);
  operator double() const { return value; };
  void Interchange(TimeVariable& Newtvar, Keeper* const keeper) const;
  void Update(const TimeClass* const TimeInfo);
  void Update();
  void Delete(Keeper* const keeper) const;
  int DidChange(const TimeClass* const TimeInfo);
  int AtCurrentTime(const TimeClass* const TimeInfo) const;
  ~TimeVariable();
private:
  int ValuesReadFromFile;
  int usemodelmatrix;
  int timestepnr;   //last timestep
  int time;
  int firsttimestepnr;
  IntVector years;  //Years when things change
  IntVector steps;  //Steps when things change
  FormulaVector values;
  DoubleMatrix modelmatrix;
  FormulaVector coeff;
  Formula Value;
  double lastvalue;
  double value;
  char* description;
};

#endif
