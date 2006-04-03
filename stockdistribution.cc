#include "stockdistribution.h"
#include "readfunc.h"
#include "readword.h"
#include "readaggregation.h"
#include "errorhandler.h"
#include "areatime.h"
#include "fleet.h"
#include "stock.h"
#include "mathfunc.h"
#include "stockprey.h"
#include "gadget.h"

extern ErrorHandler handle;

StockDistribution::StockDistribution(CommentStream& infile,
  const AreaClass* const Area, const TimeClass* const TimeInfo,
  double weight, const char* name)
  : Likelihood(STOCKDISTRIBUTIONLIKELIHOOD, weight, name), alptr(0) {

  int i, j;
  char text[MaxStrLength];
  strncpy(text, "", MaxStrLength);
  int numarea = 0, numage = 0, numlen = 0;

  char datafilename[MaxStrLength];
  char aggfilename[MaxStrLength];
  strncpy(datafilename, "", MaxStrLength);
  strncpy(aggfilename, "", MaxStrLength);
  ifstream datafile;
  CommentStream subdata(datafile);

  timeindex = 0;
  functionname = new char[MaxStrLength];
  strncpy(functionname, "", MaxStrLength);
  readWordAndValue(infile, "datafile", datafilename);
  readWordAndValue(infile, "function", functionname);
  if (strcasecmp(functionname, "multinomial") == 0) {
    MN = Multinomial();
    functionnumber = 1;
  } else if (strcasecmp(functionname, "sumofsquares") == 0) {
    functionnumber = 2;
  } else
    handle.logFileMessage(LOGFAIL, "\nError in stockdistribution - unrecognised function", functionname);

  //JMB - changed to make the reading of overconsumption optional
  infile >> ws;
  char c = infile.peek();
  if ((c == 'o') || (c == 'O')) {
    readWordAndVariable(infile, "overconsumption", overconsumption);
    infile >> ws;
    c = infile.peek();
  } else
    overconsumption = 0;

  if (overconsumption != 0 && overconsumption != 1)
    handle.logFileMessage(LOGFAIL, "\nError in stockdistribution - overconsumption must be 0 or 1");

  //JMB - changed to make the reading of minimum probability optional
  if ((c == 'm') || (c == 'M'))
    readWordAndVariable(infile, "minimumprobability", epsilon);
  else if ((c == 'e') || (c == 'E'))
    readWordAndVariable(infile, "epsilon", epsilon);
  else
    epsilon = 10.0;

  if (epsilon < verysmall) {
    handle.logFileMessage(LOGWARN, "epsilon should be a positive integer - set to default value 10");
    epsilon = 10.0;
  }

  //read in area aggregation from file
  readWordAndValue(infile, "areaaggfile", aggfilename);
  datafile.open(aggfilename, ios::in);
  handle.checkIfFailure(datafile, aggfilename);
  handle.Open(aggfilename);
  numarea = readAggregation(subdata, areas, areaindex);
  handle.Close();
  datafile.close();
  datafile.clear();

  //read in age aggregation from file
  readWordAndValue(infile, "ageaggfile", aggfilename);
  datafile.open(aggfilename, ios::in);
  handle.checkIfFailure(datafile, aggfilename);
  handle.Open(aggfilename);
  numage = readAggregation(subdata, ages, ageindex);
  handle.Close();
  datafile.close();
  datafile.clear();

  //read in length aggregation from file
  readWordAndValue(infile, "lenaggfile", aggfilename);
  datafile.open(aggfilename, ios::in);
  handle.checkIfFailure(datafile, aggfilename);
  handle.Open(aggfilename);
  numlen = readLengthAggregation(subdata, lengths, lenindex);
  handle.Close();
  datafile.close();
  datafile.clear();

  //Must change from outer areas to inner areas.
  for (i = 0; i < areas.Nrow(); i++)
    for (j = 0; j < areas.Ncol(i); j++)
      areas[i][j] = Area->getInnerArea(areas[i][j]);

  //Must create the length group division
  LgrpDiv = new LengthGroupDivision(lengths);

  //read in the fleetnames
  i = 0;
  infile >> text >> ws;
  if (!(strcasecmp(text, "fleetnames") == 0))
    handle.logFileUnexpected(LOGFAIL, "fleetnames", text);
  infile >> text >> ws;
  while (!infile.eof() && !(strcasecmp(text, "stocknames") == 0)) {
    fleetnames.resize(new char[strlen(text) + 1]);
    strcpy(fleetnames[i++], text);
    infile >> text >> ws;
  }
  if (fleetnames.Size() == 0)
    handle.logFileMessage(LOGFAIL, "\nError in stockdistribution - failed to read fleets");
  handle.logMessage(LOGMESSAGE, "Read fleet data - number of fleets", fleetnames.Size());

  //read in the stocknames
  i = 0;
  if (!(strcasecmp(text, "stocknames") == 0))
    handle.logFileUnexpected(LOGFAIL, "stocknames", text);
  infile >> text;
  while (!infile.eof() && !(strcasecmp(text, "[component]") == 0)) {
    infile >> ws;
    stocknames.resize(new char[strlen(text) + 1]);
    strcpy(stocknames[i++], text);
    infile >> text;
  }
  if (stocknames.Size() == 0)
    handle.logFileMessage(LOGFAIL, "\nError in stockdistribution - failed to read stocks");
  handle.logMessage(LOGMESSAGE, "Read stock data - number of stocks", stocknames.Size());

  //We have now read in all the data from the main likelihood file
  //But we have to read in the statistics data from datafilename
  datafile.open(datafilename, ios::in);
  handle.checkIfFailure(datafile, datafilename);
  handle.Open(datafilename);
  readStockData(subdata, TimeInfo, numarea, numage, numlen);
  handle.Close();
  datafile.close();
  datafile.clear();
}

void StockDistribution::readStockData(CommentStream& infile,
  const TimeClass* TimeInfo, int numarea, int numage, int numlen) {

  double tmpnumber;
  char tmparea[MaxStrLength], tmpstock[MaxStrLength];
  char tmpage[MaxStrLength], tmplen[MaxStrLength];
  strncpy(tmparea, "", MaxStrLength);
  strncpy(tmpstock, "", MaxStrLength);
  strncpy(tmpage, "", MaxStrLength);
  strncpy(tmplen, "", MaxStrLength);
  int i, j, year, step, count;
  int keepdata, timeid, stockid, ageid, areaid, lenid;
  int numstock = stocknames.Size();

  //Check the number of columns in the inputfile
  infile >> ws;
  if (countColumns(infile) != 7)
    handle.logFileMessage(LOGFAIL, "wrong number of columns in inputfile - should be 7");

  year = step = count = 0;
  while (!infile.eof()) {
    keepdata = 0;
    infile >> year >> step >> tmparea >> tmpstock >> tmpage >> tmplen >> tmpnumber >> ws;

    //crude check to see if something has gone wrong and avoid infinite loops
    if (strlen(tmparea) == 0)
      handle.logFileMessage(LOGFAIL, "failed to read data from file");

    //if tmpstock is in stocknames find stockid, else dont keep the data
    stockid = -1;
    for (i = 0; i < numstock; i++)
      if (strcasecmp(stocknames[i], tmpstock) == 0)
        stockid = i;

    if (stockid == -1)
      keepdata = 1;

    //if tmparea is in areaindex find areaid, else dont keep the data
    areaid = -1;
    for (i = 0; i < numarea; i++)
      if (strcasecmp(areaindex[i], tmparea) == 0)
        areaid = i;

    if (areaid == -1)
      keepdata = 1;

    //if tmpage is in ageindex find ageid, else dont keep the data
    ageid = -1;
    for (i = 0; i < numage; i++)
      if (strcasecmp(ageindex[i], tmpage) == 0)
        ageid = i;

    if (ageid == -1)
      keepdata = 1;

    //if tmplen is in lenindex find lenid, else dont keep the data
    lenid = -1;
    for (i = 0; i < numlen; i++)
      if (strcasecmp(lenindex[i], tmplen) == 0)
        lenid = i;

    if (lenid == -1)
      keepdata = 1;

    //check if the year and step are in the simulation
    timeid = -1;
    if ((TimeInfo->isWithinPeriod(year, step)) && (keepdata == 0)) {
      //if this is a new timestep, resize to store the data
      for (i = 0; i < Years.Size(); i++)
        if ((Years[i] == year) && (Steps[i] == step))
          timeid = i;

      if (timeid == -1) {
        Years.resize(1, year);
        Steps.resize(1, step);
        timeid = (Years.Size() - 1);

        obsDistribution.resize();
        modelDistribution.resize();
        likelihoodValues.AddRows(1, numarea, 0.0);
        for (i = 0; i < numarea; i++) {
          obsDistribution[timeid].resize(new DoubleMatrix(numstock, (numage * numlen), 0.0));
          modelDistribution[timeid].resize(new DoubleMatrix(numstock, (numage * numlen), 0.0));
        }
      }

    } else {
      //dont keep the data
      keepdata = 1;
    }

    if (keepdata == 0) {
      //stock distribution data is required, so store it
      count++;
      i = ageid + (numage * lenid);
      //JMB - this should be stored as [time][area][stock][age][length]
      (*obsDistribution[timeid][areaid])[stockid][i] = tmpnumber;
    }
  }
  AAT.addActions(Years, Steps, TimeInfo);
  if (count == 0)
    handle.logMessage(LOGWARN, "Warning in stockdistribution - found no data in the data file for", this->getName());
  handle.logMessage(LOGMESSAGE, "Read stockdistribution data file - number of entries", count);
}

StockDistribution::~StockDistribution() {
  int i, j;
  for (i = 0; i < stocknames.Size(); i++) {
    delete[] stocknames[i];
    delete aggregator[i];
  }
  delete[] aggregator;
  delete[] functionname;
  delete LgrpDiv;

  for (i = 0; i < fleetnames.Size(); i++)
    delete[] fleetnames[i];
  for (i = 0; i < areaindex.Size(); i++)
    delete[] areaindex[i];
  for (i = 0; i < ageindex.Size(); i++)
    delete[] ageindex[i];
  for (i = 0; i < lenindex.Size(); i++)
    delete[] lenindex[i];
  for (i = 0; i < obsDistribution.Nrow(); i++)
    for (j = 0; j < obsDistribution.Ncol(i); j++) {
      delete obsDistribution[i][j];
      delete modelDistribution[i][j];
    }
}

void StockDistribution::Reset(const Keeper* const keeper) {
  Likelihood::Reset(keeper);
  switch (functionnumber) {
    case 1:
      MN.setValue(epsilon);
      break;
    case 2:
      break;
    default:
      handle.logMessage(LOGWARN, "Warning in stockdistribution - unrecognised function", functionname);
      break;
  }
  if (handle.getLogLevel() >= LOGMESSAGE)
    handle.logMessage(LOGMESSAGE, "Reset stockdistribution component", this->getName());
}

void StockDistribution::Print(ofstream& outfile) const {
  int i;
  outfile << "\nStock Distribution " << this->getName() << " - likelihood value " << likelihood
    << "\n\tFunction " << functionname << "\n\tStock names:";
  for (i = 0; i < stocknames.Size(); i++)
    outfile << sep << stocknames[i];
  outfile << "\n\tFleet names:";
  for (i = 0; i < fleetnames.Size(); i++)
    outfile << sep << fleetnames[i];
  outfile << endl;
  for (i = 0; i < stocknames.Size(); i++)
    aggregator[i]->Print(outfile);
  outfile.flush();
}

void StockDistribution::setFleetsAndStocks(FleetPtrVector& Fleets, StockPtrVector& Stocks) {
  int s, i, j, k, found, minage, maxage;
  FleetPtrVector fleets;
  StockPtrVector checkstocks;
  aggregator = new FleetPreyAggregator*[stocknames.Size()];

  for (i = 0; i < fleetnames.Size(); i++) {
    found = 0;
    for (j = 0; j < Fleets.Size(); j++) {
      if (strcasecmp(fleetnames[i], Fleets[j]->getName()) == 0) {
        found++;
        fleets.resize(Fleets[j]);
      }
    }
    if (found == 0)
      handle.logMessage(LOGFAIL, "Error in stockdistribution - unrecognised fleet", fleetnames[i]);
  }

  //check fleet areas
  if (handle.getLogLevel() >= LOGWARN) {
    for (j = 0; j < areas.Nrow(); j++) {
      found = 0;
      for (i = 0; i < fleets.Size(); i++)
        for (k = 0; k < areas.Ncol(j); k++)
          if (fleets[i]->isInArea(areas[j][k]))
            found++;
      if (found == 0)
        handle.logMessage(LOGWARN, "Warning in stockdistribution - fleet not defined on all areas");
    }
  }

  for (s = 0; s < stocknames.Size(); s++) {
    StockPtrVector stocks;
    found = 0;
    for (j = 0; j < Stocks.Size(); j++) {
      if (Stocks[j]->isEaten()) {
        if (strcasecmp(stocknames[s], Stocks[j]->getName()) == 0) {
          found++;
          stocks.resize(Stocks[j]);
          checkstocks.resize(Stocks[j]);
        }
      }
    }
    if (found == 0)
      handle.logMessage(LOGFAIL, "Error in stockdistribution - unrecognised stock", stocknames[i]);

    aggregator[s] = new FleetPreyAggregator(fleets, stocks, LgrpDiv, areas, ages, overconsumption);
  }

  //check areas, ages and lengths
  if (handle.getLogLevel() >= LOGWARN) {
    for (j = 0; j < areas.Nrow(); j++) {
      found = 0;
      for (i = 0; i < checkstocks.Size(); i++)
        for (k = 0; k < areas.Ncol(j); k++)
          if (checkstocks[i]->isInArea(areas[j][k]))
            found++;
      if (found == 0)
        handle.logMessage(LOGWARN, "Warning in stockdistribution - stock not defined on all areas");
    }

    minage = 9999;
    maxage = 0;
    for (i = 0; i < ages.Nrow(); i++) {
      for (j = 0; j < ages.Ncol(i); j++) {
        minage = min(ages[i][j], minage);
        maxage = max(ages[i][j], maxage);
      }
    }

    found = 0;
    for (i = 0; i < checkstocks.Size(); i++)
      if (minage >= checkstocks[i]->minAge())
        found++;
    if (found == 0)
      handle.logMessage(LOGWARN, "Warning in stockdistribution - minimum age less than stock age");

    found = 0;
    for (i = 0; i < checkstocks.Size(); i++)
      if (maxage <= checkstocks[i]->maxAge())
        found++;
    if (found == 0)
      handle.logMessage(LOGWARN, "Warning in stockdistribution - maximum age greater than stock age");

    found = 0;
    for (i = 0; i < checkstocks.Size(); i++)
      if (LgrpDiv->maxLength(0) > checkstocks[i]->getLengthGroupDiv()->minLength())
        found++;
    if (found == 0)
      handle.logMessage(LOGWARN, "Warning in stockdistribution - minimum length group less than stock length");

    found = 0;
    for (i = 0; i < checkstocks.Size(); i++)
      if (LgrpDiv->minLength(LgrpDiv->numLengthGroups()) < checkstocks[i]->getLengthGroupDiv()->maxLength())
        found++;
    if (found == 0)
      handle.logMessage(LOGWARN, "Warning in stockdistribution - maximum length group greater than stock length");
  }
}

void StockDistribution::addLikelihood(const TimeClass* const TimeInfo) {
  if (!(AAT.atCurrentTime(TimeInfo)))
    return;

  int i;
  timeindex = -1;
  for (i = 0; i < Years.Size(); i++)
    if ((Years[i] == TimeInfo->getYear()) && (Steps[i] == TimeInfo->getStep()))
      timeindex = i;
  if (timeindex == -1)
    handle.logMessage(LOGFAIL, "Error in stockdistribution - invalid timestep");

  double l = 0.0;
  if (handle.getLogLevel() >= LOGMESSAGE)
    handle.logMessage(LOGMESSAGE, "Calculating likelihood score for stockdistribution component", this->getName());
  for (i = 0; i < stocknames.Size(); i++) {
    aggregator[i]->Sum();
    if ((handle.getLogLevel() >= LOGWARN) && (aggregator[i]->checkCatchData() == 1))
      handle.logMessage(LOGWARN, "Warning in stockdistribution - zero catch found");
  }

  switch (functionnumber) {
    case 1:
      l = calcLikMultinomial();
      break;
    case 2:
      l = calcLikSumSquares();
      break;
    default:
      handle.logMessage(LOGWARN, "Warning in stockdistribution - unrecognised function", functionname);
      break;
  }
  likelihood += l;
  if (handle.getLogLevel() >= LOGMESSAGE)
    handle.logMessage(LOGMESSAGE, "The likelihood score for this component on this timestep is", l);
}

//The code here is probably unnessecarily complicated because
//is always only one length group with this class.
double StockDistribution::calcLikMultinomial() {
  int age, len, area, s, i;
  //JMB - number of age and length groups is constant for all stocks
  int numage = ages.Nrow();
  int numlen = LgrpDiv->numLengthGroups();
  DoubleVector obsdata(stocknames.Size(), 0.0);
  DoubleVector moddata(stocknames.Size(), 0.0);

  MN.Reset();
  //the object MN does most of the work, accumulating likelihood
  for (area = 0; area < areas.Nrow(); area++) {
    likelihoodValues[timeindex][area] = 0.0;
    for (s = 0; s < stocknames.Size(); s++) {
      alptr = &aggregator[s]->getSum();
      for (age = 0; age < numage; age++)
        for (len = 0; len < numlen; len++)
          (*modelDistribution[timeindex][area])[s][age + (numage * len)] = ((*alptr)[area][age][len]).N;
    }

    for (i = 0; i < (numage * numlen); i++) {
      for (s = 0; s < stocknames.Size(); s++) {
        obsdata[s] = (*obsDistribution[timeindex][area])[s][i];
        moddata[s] = (*modelDistribution[timeindex][area])[s][i];
      }
      likelihoodValues[timeindex][area] += MN.calcLogLikelihood(obsdata, moddata);
    }
  }
  return MN.getLogLikelihood();
}

double StockDistribution::calcLikSumSquares() {
  double totallikelihood = 0.0, temp = 0.0;
  double totalmodel, totaldata;
  int age, len, area, s, i;

  //JMB - number of age and length groups is constant for all stocks
  int numage = ages.Nrow();
  int numlen = LgrpDiv->numLengthGroups();

  for (area = 0; area < areas.Nrow(); area++) {
    likelihoodValues[timeindex][area] = 0.0;
    for (s = 0; s < stocknames.Size(); s++) {
      alptr = &aggregator[s]->getSum();
      for (age = 0; age < numage; age++)
        for (len = 0; len < numlen; len++)
          (*modelDistribution[timeindex][area])[s][age + (numage * len)] = ((*alptr)[area][age][len]).N;
    }

    for (age = 0; age < numage; age++) {
      for (len = 0; len < numlen; len++) {
        totalmodel = 0.0;
        totaldata = 0.0;
        i = age + (numage * len);
        for (s = 0; s < stocknames.Size(); s++) {
          totalmodel += (*modelDistribution[timeindex][area])[s][i];
          totaldata += (*obsDistribution[timeindex][area])[s][i];
        }

        for (s = 0; s < stocknames.Size(); s++) {
          if ((isZero(totaldata)) && (isZero(totalmodel)))
            temp = 0.0;
          else if (isZero(totaldata))
            temp = (*modelDistribution[timeindex][area])[s][i] / totalmodel;
          else if (isZero(totalmodel))
            temp = (*obsDistribution[timeindex][area])[s][i] / totaldata;
          else
            temp = (((*obsDistribution[timeindex][area])[s][i] / totaldata)
              - ((*modelDistribution[timeindex][area])[s][i] / totalmodel));

          likelihoodValues[timeindex][area] += (temp * temp);
        }
      }
    }
    totallikelihood += likelihoodValues[timeindex][area];
  }
  return totallikelihood;
}

void StockDistribution::printLikelihood(ofstream& outfile, const TimeClass* const TimeInfo) {

  if (!AAT.atCurrentTime(TimeInfo))
    return;

  int area, s, i, age, len;
  int numage = ages.Nrow();

  timeindex = -1;
  for (i = 0; i < Years.Size(); i++)
    if ((Years[i] == TimeInfo->getYear()) && (Steps[i] == TimeInfo->getStep()))
      timeindex = i;
  if (timeindex == -1)
    handle.logMessage(LOGFAIL, "Error in stockdistribution - invalid timestep");

  for (area = 0; area < modelDistribution.Ncol(timeindex); area++) {
    for (s = 0; s < modelDistribution[timeindex][area]->Nrow(); s++) {
      for (i = 0; i < modelDistribution[timeindex][area]->Ncol(s); i++) {
        // need to calculate the age and length index from i
        // i = ageid + (numage * lenid);
        age = i % numage;
        len = i - age;
        outfile << setw(lowwidth) << Years[timeindex] << sep << setw(lowwidth)
          << Steps[timeindex] << sep << setw(printwidth) << areaindex[area] << sep
          << setw(printwidth) << stocknames[s] << sep << setw(printwidth)
          << ageindex[age] << sep << setw(printwidth) << lenindex[len]
          << sep << setprecision(largeprecision) << setw(largewidth)
          << (*modelDistribution[timeindex][area])[s][i] << endl;
      }
    }
  }
}

void StockDistribution::printSummary(ofstream& outfile) {
  int year, area;

  for (year = 0; year < likelihoodValues.Nrow(); year++)
    for (area = 0; area < likelihoodValues.Ncol(year); area++)
      outfile << setw(lowwidth) << Years[year] << sep << setw(lowwidth)
        << Steps[year] << sep << setw(printwidth) << areaindex[area] << sep
        << setw(largewidth) << this->getName() << sep << setprecision(smallprecision)
        << setw(smallwidth) << weight << sep << setprecision(largeprecision)
        << setw(largewidth) << likelihoodValues[year][area] << endl;

  outfile.flush();
}
