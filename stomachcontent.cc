#include "stomachcontent.h"
#include "areatime.h"
#include "predator.h"
#include "prey.h"
#include "conversion.h"
#include "predatoraggregator.h"
#include "bandmatrixptrvector.h"
#include "readfunc.h"
#include "readword.h"
#include "readaggregation.h"
#include "multinomial.h"
#include "errorhandler.h"
#include "formatedprinting.h"
#include "gadget.h"

StomachContent::StomachContent(CommentStream& infile,
  const AreaClass* const Area, const TimeClass* const TimeInfo,
  Keeper* const keeper, double likweight, const char* sname)
  : Likelihood(STOMACHCONTENTLIKELIHOOD, likweight), name(strdup(sname)) {

  ErrorHandler handle;
  char datafilename[MaxStrLength];
  char numfilename[MaxStrLength];
  strncpy(datafilename, "", MaxStrLength);
  strncpy(numfilename, "", MaxStrLength);

  ReadWordAndVariable(infile, "functionnumber", functionnumber);
  ReadWordAndValue(infile, "datafile", datafilename);
  switch(functionnumber) {
    case 1:
      StomCont = new SCNumbers(infile, Area, TimeInfo, keeper, name, datafilename, numfilename);
      break;
    case 2:
      ReadWordAndValue(infile, "numberfile", numfilename);
      StomCont = new SCRatios(infile, Area, TimeInfo, keeper, name, datafilename, numfilename);
      break;
    case 3:
      ReadWordAndValue(infile, "numberfile", numfilename);
      StomCont = new SCAmounts(infile, Area, TimeInfo, keeper, name, datafilename, numfilename);
      break;
    default:
      handle.Message("Unrecognized functionnumber");
  }
}

void StomachContent::AddToLikelihood(const TimeClass* const TimeInfo) {
  likelihood += StomCont->Likelihood(TimeInfo);
}

void StomachContent::Print(ofstream& outfile) const {
  outfile << "\nStomach Content\nlikelihood " << likelihood
    << "\nfunction number " << functionnumber << endl;
  StomCont->Print(outfile);
}

SC::SC(CommentStream& infile, const AreaClass* const Area,
  const TimeClass* const TimeInfo, Keeper* const keeper,
  const char* sname, const char* datafilename) : name(strdup(sname)) {

  ErrorHandler handle;
  int i, j;
  char text[MaxStrLength];
  strncpy(text, "", MaxStrLength);
  int numarea = 0;
  int numpred = 0;
  int numprey = 0;

  char aggfilename[MaxStrLength];
  strncpy(aggfilename, "", MaxStrLength);

  ifstream datafile;
  CommentStream subdata(datafile);

  timeindex = 0;
  ReadWordAndVariable(infile, "minimumprobability", minp);
  if (minp <= 0) {
    handle.Warning("Minimumprobability should be a positive integer - set to default value 20");
    minp = 20;
  }

  //Read in area aggregation from file
  ReadWordAndValue(infile, "areaaggfile", aggfilename);
  datafile.open(aggfilename);
  CheckIfFailure(datafile, aggfilename);
  handle.Open(aggfilename);
  numarea = ReadAggregation(subdata, areas, areaindex);
  handle.Close();
  datafile.close();
  datafile.clear();

  //Must change from outer areas to inner areas.
  for (i = 0; i < areas.Nrow(); i++)
    for (j = 0; j < areas.Ncol(i); j++)
      if ((areas[i][j] = Area->InnerArea(areas[i][j])) == -1)
        handle.UndefinedArea(areas[i][j]);

  //Read in the predators
  i = 0;
  infile >> text >> ws;
  if (strcasecmp(text, "predators") != 0)
    handle.Unexpected("predators", text);
  infile >> text >> ws;
  while (!infile.eof() && ((strcasecmp(text, "predatorlengths") != 0)
      && (strcasecmp(text, "predatorages") != 0))) {
    predatornames.resize(1);
    predatornames[i] = new char[strlen(text) + 1];
    strcpy(predatornames[i++], text);
    infile >> text >> ws;
  }

  if (strcasecmp(text, "predatorlengths") == 0) { //read predator lengths
    age_pred = 0; //predator is length structured
    ReadWordAndValue(infile, "lenaggfile", aggfilename);
    datafile.open(aggfilename);
    CheckIfFailure(datafile, aggfilename);
    handle.Open(aggfilename);
    numpred = ReadLengthAggregation(subdata, predatorlengths, predindex);
    handle.Close();
    datafile.close();
    datafile.clear();
  } else if (strcasecmp(text, "predatorages") == 0) { //read predator ages
    age_pred = 1; //predator is age structured
    ReadWordAndValue(infile, "ageaggfile", aggfilename);
    datafile.open(aggfilename);
    CheckIfFailure(datafile, aggfilename);
    handle.Open(aggfilename);
    numpred = ReadAggregation(subdata, predatorages, predindex);
    handle.Close();
    datafile.close();
    datafile.clear();
  } else
    handle.Eof("predator lengths or ages");

  //Read in the preys
  ReadWordAndValue(infile, "preyaggfile", aggfilename);
  datafile.open(aggfilename);
  CheckIfFailure(datafile, aggfilename);
  handle.Open(aggfilename);
  numprey = ReadPreyAggregation(subdata, preynames, preylengths, digestioncoeff, preyindex, keeper);
  handle.Close();
  datafile.close();
  datafile.clear();

  //prepare for next likelihood component
  infile >> ws;
  if (!infile.eof()) {
    infile >> text >> ws;
    if (!(strcasecmp(text, "[component]") == 0))
      handle.Unexpected("[component]", text);
  }
}

void SC::Aggregate(int i) {
  if (age_pred)
    aggregator[i]->Sum(1);
  else
    aggregator[i]->Sum();
}

double SC::Likelihood(const TimeClass* const TimeInfo) {
  if (!AAT.AtCurrentTime(TimeInfo))
    return 0.0;

  int i, a, k;
  int prey_g = 0, prey_l = 0;
  double length = 0.0, mult = 0.0;

  int pred_size = 0;
  if (age_pred) //age structured predator
    pred_size = predatorages.Size();
  else
    pred_size = predatorlengths.Size() - 1;

  //The vector which will keep the consumption, indexed the same way as in stomachcontent
  doublematrixptrvector consumption(areas.Nrow());
  for (a = 0; a < areas.Nrow(); a++)
    consumption[a] = new doublematrix(stomachcontent[timeindex][0]->Nrow(),
      stomachcontent[timeindex][0]->Ncol(), 0.0);

  //Get the consumption from aggregator, indexed the same way as in stomachcontent
  //While we're at it, get the sum of each row, for later scaling.
  doublematrix sum(areas.Nrow(), pred_size, 0);
  for (i = 0; i < preynames.Nrow(); i++) {
    Aggregate(i);
    const bandmatrixvector* cons = &aggregator[i]->ReturnSum();
    for (a = 0; a < areas.Nrow(); a++) {
      for (k = 0; k < (*cons)[a].Nrow(); k++) {
        for (prey_l = 0; prey_l < (*cons)[a].Ncol(); prey_l++) {
          length = (preylengths[i][prey_l] + preylengths[i][prey_l + 1]) * 0.5;
          mult = digestioncoeff[i][0] + digestioncoeff[i][1] *
            pow(length, digestioncoeff[i][2]);
          (*consumption[a])[k][prey_g + prey_l] = (*cons)[a][k][prey_l] * mult;
          sum[a][k] += (*cons)[a][k][prey_l] * mult;
        }
      }
    }
    prey_g += (preylengths[i].Size() - 1);
  }

  for (a = 0; a < consumption.Size(); a++)
    modelConsumption[timeindex][a] = new doublematrix(*consumption[a]);

  /*JMB code removed from here - see RemovedCode for details*/
  //Now, calculate likelihood and cleanup
  double l = CalculateLikelihood(consumption, sum);
  for (a = 0; a < areas.Nrow(); a++)
    delete consumption[a];
  timeindex++;
  return l;
}

SCNumbers::SCNumbers(CommentStream& infile, const AreaClass* const Area,
  const TimeClass* const TimeInfo, Keeper* const keeper,
  const char* nam, const char* datafilename, const char* numfilename)
  : SC(infile, Area, TimeInfo, keeper, nam, datafilename) {

  ErrorHandler handle;
  ifstream datafile;
  CommentStream subdata(datafile);

  //Read in stomach content from file
  datafile.open(datafilename);
  CheckIfFailure(datafile, datafilename);
  handle.Open(datafilename);
  ReadStomachNumberContent(subdata, TimeInfo);
  handle.Close();
  datafile.close();
  datafile.clear();
}

SCAmounts::SCAmounts(CommentStream& infile, const AreaClass* const Area,
  const TimeClass* const TimeInfo, Keeper* const keeper,
  const char* nam, const char* datafilename, const char* numfilename)
  : SC(infile, Area, TimeInfo, keeper, nam, datafilename) {

  ErrorHandler handle;
  ifstream datafile;
  CommentStream subdata(datafile);

  //Read in stomach content amounts from file
  datafile.open(datafilename);
  CheckIfFailure(datafile, datafilename);
  handle.Open(datafilename);
  ReadStomachAmountContent(subdata, TimeInfo);
  handle.Close();
  datafile.close();
  datafile.clear();

  //Read in stomach content sample size from file
  datafile.open(numfilename);
  CheckIfFailure(datafile, numfilename);
  handle.Open(numfilename);
  ReadStomachSampleContent(subdata, TimeInfo);
  handle.Close();
  datafile.close();
  datafile.clear();
}

void SCNumbers::ReadStomachNumberContent(CommentStream& infile, const TimeClass* const TimeInfo) {

  ErrorHandler handle;
  int i;
  int year, step;
  double tmpnumber;
  char tmparea[MaxStrLength], tmppred[MaxStrLength], tmpprey[MaxStrLength];
  strncpy(tmparea, "", MaxStrLength);
  strncpy(tmppred, "", MaxStrLength);
  strncpy(tmpprey, "", MaxStrLength);
  int keepdata, timeid, areaid, predid, preyid;

  int pred_size = 0;
  if (age_pred) //age structured predator
    pred_size = predatorages.Size();
  else
    pred_size = predatorlengths.Size() - 1;

  int numarea = areas.Nrow();
  int nopreygroups = 0;
  for (i = 0; i < preylengths.Nrow(); i++)
    nopreygroups += preylengths[i].Size() - 1;

  //Find start of distribution data in datafile
  infile >> ws;
  char c;
  c = infile.peek();
  if (!isdigit(c)) {
    infile.get(c);
    while (c != '\n' && !infile.eof())
      infile.get(c);
  }

  //Check the number of columns in the inputfile
  if (!(CheckColumns(infile, 6)))
    handle.Message("Wrong number of columns in inputfile - should be 6");

  while (!infile.eof()) {
    keepdata = 0;
    infile >> year >> step >> tmparea >> tmppred >> tmpprey >> tmpnumber >> ws;

    //check if the year and step are in the simulation
    timeid = -1;
    if (TimeInfo->IsWithinPeriod(year, step)) {
      //if this is a new timestep, resize to store the data
      for (i = 0; i < Years.Size(); i++)
        if ((Years[i] == year) && (Steps[i] == step))
          timeid = i;

      if (timeid == -1) {
        Years.resize(1, year);
        Steps.resize(1, step);
        timeid = (Years.Size() - 1);

        stomachcontent.AddRows(1, numarea);
        for (i = 0; i < numarea; i++)
          stomachcontent[timeid][i] = new doublematrix(pred_size, nopreygroups, 0.0);
      }

    } else {
      //dont keep the data
      keepdata = 1;
    }

    //if tmparea is in areaindex find areaid, else dont keep the data
    areaid = -1;
    for (i = 0; i < areaindex.Size(); i++)
      if (strcasecmp(areaindex[i], tmparea) == 0)
        areaid = i;

    if (areaid == -1)
      keepdata = 1;

    //if tmppred is in predindex find predid, else dont keep the data
    predid = -1;
    for (i = 0; i < predindex.Size(); i++)
      if (strcasecmp(predindex[i], tmppred) == 0)
        predid = i;

    if (predid == -1)
      keepdata = 1;

    //if tmpprey is in preyindex find preyid, else dont keep the data
    preyid = -1;
    for (i = 0; i < preyindex.Size(); i++)
      if (strcasecmp(preyindex[i], tmpprey) == 0)
        preyid = i;

    if (preyid == -1)
      keepdata = 1;

    if (keepdata == 0) {
      //stomach content data is required, so store it
      (*stomachcontent[timeid][areaid])[predid][preyid] = tmpnumber;
    }
  }

  AAT.AddActions(Years, Steps, TimeInfo);
  modelConsumption.AddRows(stomachcontent.Nrow(), stomachcontent.Ncol());
}

void SCAmounts::ReadStomachAmountContent(CommentStream& infile, const TimeClass* const TimeInfo) {

  ErrorHandler handle;
  int i;
  int year, step;
  double tmpnumber, tmpstddev;
  char tmparea[MaxStrLength], tmppred[MaxStrLength], tmpprey[MaxStrLength];
  strncpy(tmparea, "", MaxStrLength);
  strncpy(tmppred, "", MaxStrLength);
  strncpy(tmpprey, "", MaxStrLength);
  int keepdata, timeid, areaid, predid, preyid;

  int pred_size = 0;
  if (age_pred) //age structured predator
    pred_size = predatorages.Size();
  else
    pred_size = predatorlengths.Size() - 1;

  int numarea = areas.Nrow();
  int nopreygroups = 0;
  for (i = 0; i < preylengths.Nrow(); i++)
    nopreygroups += preylengths[i].Size() - 1;

  //Find start of distribution data in datafile
  infile >> ws;
  char c;
  c = infile.peek();
  if (!isdigit(c)) {
    infile.get(c);
    while (c != '\n' && !infile.eof())
      infile.get(c);
  }

  //Check the number of columns in the inputfile
  if (!(CheckColumns(infile, 7)))
    handle.Message("Wrong number of columns in inputfile - should be 7");

  while (!infile.eof()) {
    keepdata = 0;
    infile >> year >> step >> tmparea >> tmppred >> tmpprey >> tmpnumber >> tmpstddev >> ws;

    //check if the year and step are in the simulation
    timeid = -1;
    if (TimeInfo->IsWithinPeriod(year, step)) {
      //if this is a new timestep, resize to store the data
      for (i = 0; i < Years.Size(); i++)
        if ((Years[i] == year) && (Steps[i] == step))
          timeid = i;

      if (timeid == -1) {
        Years.resize(1, year);
        Steps.resize(1, step);
        timeid = (Years.Size() - 1);

        stomachcontent.AddRows(1, numarea);
        stddev.AddRows(1, numarea);
        for (i = 0; i < numarea; i++) {
          stomachcontent[timeid][i] = new doublematrix(pred_size, nopreygroups, 0.0);
          stddev[timeid][i] = new doublematrix(pred_size, nopreygroups, 0.0);
        }
      }

    } else {
      //dont keep the data
      keepdata = 1;
    }

    //if tmparea is in areaindex find areaid, else dont keep the data
    areaid = -1;
    for (i = 0; i < areaindex.Size(); i++)
      if (strcasecmp(areaindex[i], tmparea) == 0)
        areaid = i;

    if (areaid == -1)
      keepdata = 1;

    //if tmppred is in predindex find predid, else dont keep the data
    predid = -1;
    for (i = 0; i < predindex.Size(); i++)
      if (strcasecmp(predindex[i], tmppred) == 0)
        predid = i;

    if (predid == -1)
      keepdata = 1;

    //if tmpprey is in preyindex find preyid, else dont keep the data
    preyid = -1;
    for (i = 0; i < preyindex.Size(); i++)
      if (strcasecmp(preyindex[i], tmpprey) == 0)
        preyid = i;

    if (preyid == -1)
      keepdata = 1;

    if (keepdata == 0) {
      //stomach content data is required, so store it
      (*stomachcontent[timeid][areaid])[predid][preyid] = tmpnumber;
      (*stddev[timeid][areaid])[predid][preyid] = tmpstddev;
    }
  }

  AAT.AddActions(Years, Steps, TimeInfo);
  modelConsumption.AddRows(stomachcontent.Nrow(), stomachcontent.Ncol(), 0);
}

void SCAmounts::ReadStomachSampleContent(CommentStream& infile, const TimeClass* const TimeInfo) {

  ErrorHandler handle;
  int i;
  int year, step;
  double tmpnumber;
  int keepdata, timeid, areaid, predid;
  char tmparea[MaxStrLength], tmppred[MaxStrLength];
  strncpy(tmparea, "", MaxStrLength);
  strncpy(tmppred, "", MaxStrLength);

  int pred_size = 0;
  if (age_pred) //age structured predator
    pred_size = predatorages.Size();
  else
    pred_size = predatorlengths.Size() - 1;

  //We know the size that numbers[] will be from stomachcontent
  int numarea = areas.Nrow();
  number.resize(stomachcontent.Nrow());
  for (i = 0; i < stomachcontent.Nrow(); i++)
    number[i] = new doublematrix(numarea, pred_size, 0.0);

  //Find start of distribution data in datafile
  infile >> ws;
  char c;
  c = infile.peek();
  if (!isdigit(c)) {
    infile.get(c);
    while (c != '\n' && !infile.eof())
      infile.get(c);
  }

  //Check the number of columns in the inputfile
  if (!(CheckColumns(infile, 5)))
    handle.Message("Wrong number of columns in inputfile - should be 5");

  while (!infile.eof()) {
    keepdata = 0;
    infile >> year >> step >> tmparea >> tmppred >> tmpnumber >> ws;

    //check if the year and step are in the simulation
    timeid = -1;
    if (TimeInfo->IsWithinPeriod(year, step))
      //find the timeid from Years and Steps
      for (i = 0; i < Years.Size(); i++)
        if ((Years[i] == year) && (Steps[i] == step))
          timeid = i;

    if (timeid == -1)
      keepdata = 1;

    //if tmparea is in areaindex find areaid, else dont keep the data
    areaid = -1;
    for (i = 0; i < areaindex.Size(); i++)
      if (strcasecmp(areaindex[i], tmparea) == 0)
        areaid = i;

    if (areaid == -1)
      keepdata = 1;

    //if tmppred is in predindex find predid, else dont keep the data
    predid = -1;
    for (i = 0; i < predindex.Size(); i++)
      if (strcasecmp(predindex[i], tmppred) == 0)
        predid = i;

    if (predid == -1)
      keepdata = 1;

    if (keepdata == 0) {
      //stomach content data is required, so store it
      (*number[timeid])[areaid][predid] = tmpnumber;
    }
  }
}

SC::~SC() {
  int i, j;
  for (i = 0; i < stomachcontent.Nrow(); i++)
    for (j = 0; j < stomachcontent[i].Size(); j++) {
      delete stomachcontent[i][j];
      delete modelConsumption[i][j];
    }

  for (i = 0; i < preynames.Nrow(); i++) {
    delete aggregator[i];
    for (j = 0; j < preynames[i].Size(); j++)
      delete[] preynames[i][j];
  }

  for (i = 0; i < predatornames.Size(); i++)
    delete[] predatornames[i];
  for (i = 0; i < areaindex.Size(); i++)
    delete[] areaindex[i];
  for (i = 0; i < predindex.Size(); i++)
    delete[] predindex[i];
  for (i = 0; i < preyindex.Size(); i++)
    delete[] preyindex[i];
}

void SC::SetPredatorsAndPreys(Predatorptrvector& Predators, Preyptrvector& Preys) {
  int i, j, k;
  int found = 0;
  Predatorptrvector predators;
  aggregator = new PredatorAggregator*[preynames.Nrow()];
  if (age_pred == 0) { //the test is done in aggregator for age structured predators
    for (i = 0; i < predatornames.Size(); i++) {
      found = 0;
      for (j = 0; j < Predators.Size(); j++)
        if (strcasecmp(predatornames[i], Predators[j]->Name()) == 0) {
          found = 1;
          predators.resize(1, Predators[j]);
        }

      if (found == 0) {
        cerr << "Error when searching for names of predators for StomachContent\n"
          << "Did not find any name matching " << predatornames[i] << endl;
        exit(EXIT_FAILURE);
      }
    }
  }

  for (i = 0; i < preynames.Nrow(); i++) {
    Preyptrvector preys;
    for (j = 0; j < preynames[i].Size(); j++) {
      found = 0;
      for (k = 0; k < Preys.Size(); k++)
        if (strcasecmp(preynames[i][j], Preys[k]->Name()) == 0) {
          found = 1;
          preys.resize(1, Preys[k]);
        }

      if (found == 0) {
        cerr << "Error when searching for names of preys for StomachContent\n"
          << "Did not find any name matching " << preynames[i][j] << endl;
        exit(EXIT_FAILURE);
      }
    }
    LengthGroupDivision* preyLgrpDiv = new LengthGroupDivision(preylengths[i]);
    if (age_pred == 0) { //length structured predator
      LengthGroupDivision* predLgrpDiv = new LengthGroupDivision(predatorlengths);
      aggregator[i] =
        new PredatorAggregator(predators, preys, areas, predLgrpDiv, preyLgrpDiv);
    } else
      aggregator[i] =
        new PredatorAggregator(predatornames, preys, areas, predatorages, preyLgrpDiv);
  }
}

void SC::Print(ofstream& outfile) const {
  int i, j, y, ar;

  outfile << "\tPreys:\n";
  for (i = 0; i < preynames.Nrow(); i++) {
    outfile << "\t\t";
    for (j = 0; j < preynames[i].Size(); j++)
      outfile << preynames[i][j] << sep;
    outfile << "\n\t\tlengths: ";
    for (j = 0; j < preylengths[i].Size(); j++)
      outfile << preylengths[i][j] << sep;
    outfile << endl;
  }

  outfile << "\tPredators: " << sep;
  for (i = 0; i < predatornames.Size(); i++)
    outfile << predatornames[i] << sep;

  if (age_pred) {
    outfile << "\n\t\tages: ";
    for (i = 0; i < predatorages.Size(); i++)
      outfile << predatorages[i] << sep;
    outfile << endl;
  } else {
    outfile << "\n\t\tlengths: ";
    for (i = 0; i < predatorlengths.Size(); i++)
      outfile << predatorlengths[i] << sep;
    outfile << endl;
  }

  outfile << "\tStomach Content Data:\n";
  for (y = 0; y < stomachcontent.Nrow(); y++) {
    outfile << "\trow " << y;
    for (ar = 0; ar < stomachcontent[y].Size(); ar++) {
      outfile << "\n\tareas" << sep << ar << "\n\t\t";
      for (i = 0; i < stomachcontent[y][ar]->Nrow(); i++) {
        for (j = 0; j < (*stomachcontent[y][ar])[i].Size(); j++) {
          outfile.width(printwidth);
          outfile.precision(smallprecision);
          outfile << (*stomachcontent[y][ar])[i][j] << sep;
        }
      outfile << "\n\t\t";
      }
    }
  }
  outfile << endl;
}

void SC::PrintLikelihoodOnStep(ostream& outfile, int time,
  const TimeClass& TimeInfo, int print_type) {

  outfile.setf(ios::fixed);
  //The following two lines assume 'this' is an instance of SCAmounts,
  //needs to be fixed. (mnaa 03.12.99)
  doublematrixptrmatrix& std = ((SCAmounts*)this)->Stddev();
  doublematrixptrvector& num = ((SCAmounts*)this)->StomachNumbers();

  int a, pd, py;
  if (print_type == 0) { //Print log(S/S_hat)
    outfile << "year " << TimeInfo.CurrentYear() << " step "
      << TimeInfo.CurrentStep() << "\nStomach content, log(S/S_hat).\n";
    for (a = 0; a < stomachcontent[time].Size(); a++) {
      for (pd = 0; pd < stomachcontent[time][a]->Nrow(); pd++) {
        for (py = 0; py < (*stomachcontent[time][a])[pd].Size(); py++) {
          outfile.width(printwidth);
          outfile.precision(lowprecision);
          if ((*modelConsumption[time][a])[pd][py] > 0 &&
              (*stomachcontent[time][a])[pd][py] > 0)
            outfile << log ((*stomachcontent[time][a])[pd][py] /
              (*modelConsumption[time][a])[pd][py]);
          else
            outfile << "        -";
        }
        outfile << endl;
      }
    }
    outfile << endl;

  } else if (print_type == 2) { //Print (S_hat-S)^2/Stddev^2*n
    outfile << "year " << TimeInfo.CurrentYear() << " step "
      << TimeInfo.CurrentStep() << "\nStomach content, (S_hat-S)^2/Stddev^2*n.\n";
    for (a = 0; a < stomachcontent[time].Size(); a++) {
      for (pd = 0; pd < stomachcontent[time][a]->Nrow(); pd++) {
        for (py = 0; py < (*stomachcontent[time][a])[pd].Size(); py++) {
          outfile.width(largewidth);
          outfile.precision(printprecision);
          if ((*std[time][a])[pd][py] != 0) //mnaa 24.01.00
            outfile << (((*modelConsumption[time][a])[pd][py] -
              (*stomachcontent[time][a])[pd][py]) *
              ((*modelConsumption[time][a])[pd][py] -
              (*stomachcontent[time][a])[pd][py]) / ((*std[time][a])[pd][py] *
              (*std[time][a])[pd][py])) * (*num[timeindex])[a][pd];
          else
            outfile << "           -";
        }
        outfile << endl;
      }
    }
    outfile << endl;

  } else { //Print S and S_hat seperately
    outfile << "year " << TimeInfo.CurrentYear() << " step "
      << TimeInfo.CurrentStep() << "\nStomach content, observed.\n";
    for (a = 0; a < stomachcontent[time].Size(); a++) {
      for (pd = 0; pd < stomachcontent[time][a]->Nrow(); pd++) {
        for (py = 0; py < (*stomachcontent[time][a])[pd].Size(); py++) {
          outfile.width(printwidth);
          outfile.precision(printprecision);
          outfile << (*stomachcontent[time][a])[pd][py];
        }
        outfile << endl;
      }
    }
    outfile << "\nyear "<<TimeInfo.CurrentYear() << " step "
      << TimeInfo.CurrentStep() << "\nStomach content, model.\n";
    for (a = 0; a < modelConsumption[time].Size(); a++) {
      for (pd = 0; pd < modelConsumption[time][a]->Nrow(); pd++) {
        for (py = 0; py < (*modelConsumption[time][a])[pd].Size(); py++) {
          outfile.width(printwidth);
          outfile.precision(printprecision);
          outfile << (*modelConsumption[time][a])[pd][py];
        }
        outfile << endl;
      }
    }
    outfile << endl;
  }
}

void SCNumbers::Aggregate(int i) {
  aggregator[i]->NumberSum();
}

SCAmounts::~SCAmounts() {
  int i, j;
  for (i = 0; i < stddev.Nrow(); i++) {
    delete number[i];
    for (j = 0; j < stddev[i].Size(); j++)
      delete stddev[i][j];
  }
}

double SCAmounts::CalculateLikelihood(doublematrixptrvector& consumption,
  doublematrix& sum) {

  int a, k, preyl;
  double lik = 0.0;
  double tmplik = 0.0;
  for (a = 0; a < consumption.Size(); a++) {
    for (k = 0; k < consumption[a]->Nrow(); k++) {
      tmplik = 0.0;
      for (preyl = 0; preyl < consumption[a]->Ncol(); preyl++) {
        if ((*stddev[timeindex][a])[k][preyl] != 0)
          tmplik += ((*consumption[a])[k][preyl] -
            (*stomachcontent[timeindex][a])[k][preyl]) *
            ((*consumption[a])[k][preyl] - (*stomachcontent[timeindex][a])[k][preyl]) /
            ((*stddev[timeindex][a])[k][preyl] * (*stddev[timeindex][a])[k][preyl]);
      }
      lik += tmplik * (*number[timeindex])[a][k];
    }
  }
  return lik;
}

void SCAmounts::PrintLikelihood(ofstream& out, const TimeClass& timeInfo) {

  if (!AAT.AtCurrentTime(&timeInfo))
    return;

  out.setf(ios::fixed);
  int a, pd, py;

  //timeindex was increased before this is called, so we subtract 1.
  int time = timeindex - 1;

  out << "\nTime:    Year " << timeInfo.CurrentYear()
    << " Step " << timeInfo.CurrentStep() << "\nName:    " << name << endl;
  for (a = 0; a < modelConsumption[time].Size(); a++) {
    out << "Area:    " << a << "\nObserved:\n";
    for (pd = 0; pd < stomachcontent[time][a]->Nrow(); pd++) {
      for (py = 0; py < (*stomachcontent[time][a])[pd].Size(); py++) {
        out.precision(printprecision);
        out.width(printwidth);
        out << (*stomachcontent[time][a])[pd][py] << sep;
      }
      out << endl;
    }
    out << "Modeled:\n";
    for (pd = 0; pd < modelConsumption[time][a]->Nrow(); pd++) {
      for (py = 0; py < (*modelConsumption[time][a])[pd].Size(); py++) {
        out.precision(printprecision);
        out.width(printwidth);
        out << (*modelConsumption[time][a])[pd][py] << sep;
      }
      out << endl;
    }
    out << "Standard deviation:\n";
    for (pd = 0; pd < modelConsumption[time][a]->Nrow(); pd++) {
      for (py = 0; py < (*modelConsumption[time][a])[pd].Size(); py++) {
        out.precision(printprecision);
        out.width(printwidth);
        out << (*stddev[time][a])[pd][py] << sep;
      }
      out << endl;
    }
    out << "Number of stomachs:\n";
    for (pd = 0; pd < modelConsumption[time][a]->Nrow(); pd++) {
      out.precision(lowprecision);
      out.width(smallwidth);
      out << (*number[time])[a][pd] << sep;
    }
    out << endl;
  }
  out.flush();
}

void SCAmounts::PrintLikelihoodHeader(ofstream& out) {
  int i, j;

  out << "Likelihood:       SCAmounts\nFunction:         -\n"
    << "Calculated every: step\nFilter:           default\nPred by:          ";

  if (age_pred)
    out << "age\n";
  else
    out << "length\n";

  out << "Name:             " << name << "\nPreys:           ";
  for (i = 0; i < preynames.Nrow(); i++) {
    for (j = 0; j < preynames[i].Size(); j++)
      out<< sep << preynames[i][j];
    out << "\nLengths:         ";
    for (j = 0; j < preylengths[i].Size(); j++)
      out << sep << preylengths[i][j];
    out << endl;
  }

  out << "Predators:       ";
  for (i = 0; i < predatornames.Size(); i++)
    out << sep << predatornames[i];
  if (age_pred) {
    out << "\nAges:            ";
    for (i = 0; i < predatorages.Size(); i++)
      out << sep << predatorages[i];
    out << endl;
  } else {
    out << "\nLengths:         ";
    for (i = 0; i < predatorlengths.Size(); i++)
      out << sep << predatorlengths[i];
    out << endl;
  }
  out.flush();
}

void SCRatios::SetPredatorsAndPreys(Predatorptrvector& Predators, Preyptrvector& Preys) {
  int i, j, k, l;
  double sum = 0.0;
  SC::SetPredatorsAndPreys(Predators, Preys);
  //Scale each row such that it sums up to 1
  for (i = 0; i < stomachcontent.Nrow(); i++)
    for (j = 0; j < stomachcontent.Ncol(i); j++)
      for (k = 0; k < stomachcontent[i][j]->Nrow(); k++) {
        sum = 0;
        for (l = 0; l < stomachcontent[i][j]->Ncol(k); l++)
          sum += (*stomachcontent[i][j])[k][l];

        if (sum > 0)
          for (l = 0; l < stomachcontent[i][j]->Ncol(k); l++)
            (*stomachcontent[i][j])[k][l] /= sum;
      }
}

double SCRatios::CalculateLikelihood(doublematrixptrvector& consumption, doublematrix& sum) {
  int a, predl, preyl;
  double tmplik, tmpdivide;
  double lik = 0.0;

  for (a = 0; a < consumption.Size(); a++) {
    for (predl = 0; predl < consumption[a]->Nrow(); predl++) {
      if (sum[a][predl] != 0) {
        tmpdivide = 1 / sum[a][predl];
        tmplik = 0.0;
        for (preyl = 0; preyl < consumption[a]->Ncol(); preyl++) {
          if ((*stddev[timeindex][a])[predl][preyl] != 0)
            tmplik += ((*consumption[a])[predl][preyl] * tmpdivide -
              (*stomachcontent[timeindex][a])[predl][preyl]) *
              ((*consumption[a])[predl][preyl] * tmpdivide -
              (*stomachcontent[timeindex][a])[predl][preyl]) /
              ((*stddev[timeindex][a])[predl][preyl] *
              (*stddev[timeindex][a])[predl][preyl]);
        }
        tmplik *= (*number[timeindex])[a][predl];
        lik += tmplik;
      }
    }
  }
  return lik;
}

//This code will probably be simplified a bit if the Multinomial
//class is changed to take integers.  (Factorial replaced by the gamma function)
double SCNumbers::CalculateLikelihood(doublematrixptrvector& consumption,
  doublematrix& sum) {

  Multinomial MN(minp);
  int a, predl, preyl;
  for (a = 0; a < consumption.Size(); a++) {
    for (predl = 0; predl < consumption[a]->Nrow(); predl++) {
      if (sum[a][predl] != 0) {
        doublevector* numbers = new doublevector(consumption[a]->Ncol(), 0.0);
        for (preyl = 0; preyl < consumption[a]->Ncol(); preyl++)
          (*numbers)[preyl] = (*stomachcontent[timeindex][a])[predl][preyl];

        MN.LogLikelihood(*numbers, (*consumption[a])[predl]);
        delete numbers;
      }
    }
  }
  return MN.ReturnLogLikelihood();
}

void SC::Reset() {
  timeindex = 0;
  int i, j;
  for (i = 0; i < modelConsumption.Nrow(); i++)
    for (j = 0; j < modelConsumption.Ncol(); j++)
      if (modelConsumption[i][j] != 0) {
        delete modelConsumption[i][j];
        modelConsumption[i][j] = 0;
      }
}

void SC::printHeader(ofstream& outfile, const PrintInfo& print) {
  timeindex = 0;
  int i, j;
  if (print.stomachprint) {
    outfile << "\n\n";
    if (!age_pred)
      outfile << "stomachcontent by prey-length and predator-length\n";
    else
      outfile << "stomachcontent by prey-length and predator-age\n";

    outfile << "\nname " << name << "\npreys";
    for (i = 0; i < preynames.Nrow(); i++) {
      outfile << "\t\t";
      for (j = 0; j < preynames[i].Size(); j++)
        outfile << preynames[i][j] << sep;
      outfile << "\n\t\tlengths ";
      for (j = 0; j < preylengths[i].Size(); j++)
        outfile << preylengths[i][j] << sep;
      outfile << endl;
    }
    outfile << "predators\t";
    for (i = 0; i < predatornames.Size(); i++)
      outfile << predatornames[i] << sep;

    if (age_pred == 0) {
      outfile << "\n\t\tlengths ";
      for (i = 0; i < predatorlengths.Size(); i++)
        outfile << predatorlengths[i] << sep;
      outfile << endl;
    } else {
      outfile << "\n\t\tages ";
      for (i = 0; i < predatorages.Size(); i++)
        outfile << predatorages[i] << sep;
      outfile << endl;
    }
  }
}

void SC::print(ofstream& stomachfile, const TimeClass& time, const PrintInfo& print) {
  if (!AAT.AtCurrentTime(&time))
    return;
  else if (print.stomachprint) {
    PrintLikelihoodOnStep(stomachfile, timeindex, time, print.stomachprint - 1);
    timeindex++;
  }
}
