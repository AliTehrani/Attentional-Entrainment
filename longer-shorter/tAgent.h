/*
 *  tAgent.h
 *  HMMBrain
 *
 *  Created by Arend on 9/16/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef _tAgent_h_included_
#define _tAgent_h_included_

#include "globalConst.h"
#include "tHMM.h"
#include <vector>
//#include "tANN.h"
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <set>
#include <map>
#include <cmath>

//#define useANN

using namespace std;

class tAgentSetter{
public:
  int brainUpdates;
  set<int> inputNodes, outputNodes;

  tAgentSetter(int bu, int nrIns, int nrOuts, int ins[], int outs[]){
    brainUpdates=bu;
    for (int i=0; i<nrIns; i++)
      inputNodes.insert(ins[i]);
    for (int i=0; i<nrOuts; i++)
      outputNodes.insert(outs[i]);
  }
};


class tAgent{
public:
	vector<tHMMU*> hmmus;
	vector<unsigned char> genome;

	int brainUpdates;
	set<int> inputNodes, outputNodes;

	//vector<tDot> dots; // JDSREMOVE? // JDSREMOVE?
#ifdef useANN
	tANN *ANN;
#endif
	
	tAgent *ancestor;
	unsigned int nrPointingAtMe;
	unsigned char states[brainSize],newStates[brainSize]; // read-write double buffer
	double fitness;
	//double convFitness; //JDSREMOVE?
	vector<double> fitnesses; // holds multiple evaluations if we want better statistics for fitness
	
	double x,y,direction;
	int ID,nrOfOffspring;
	bool saved;
	bool retired;
	int born;
	int correct,incorrect;
	int hit, miss, falseAlarm, correctReject;//@AT
	set<int> learned, notLearned;//@AT
	set<int> actNeurons, alwaysZero;//@AT
	//key: <(ioi,toneLength, brainState)>, value: number of times brainState is (ioi,seq) visited
	map<pair<string,int>, int> visStates;//@AT
	//key: <state0, input>, value: <state1, action> (action is used for color coding of the state: action is 1 if state%2==1 and it occurs in last time step)
	//map<pair<int,int>, pair<int,int> > fsm;
	map<pair<long,long>, set<int> > fsm;//key: state1 -> state2, value: the input(s) that transition state1 to state 2
	set<long> fsmColors;//set of decision states 
	map<int,int>  orig, lo, ott;

	tAgent(tAgentSetter *setter);
	~tAgent();
	void setupRandomAgent(int nucleotides);
	void loadAgent(char* filename);
	void determinePhenotype(void);
	void inherit(tAgent *from,double mutationRate,int theTime);
	unsigned char * getStatesPointer(void);
	void setStates(vector<int> t);
	void setSensors(vector<int> sensors);
	int getAction();

	void updateStates(void);
	void resetBrain(void);
	void trembleBrain(int t);//AT
	void seedWithStartCodons(void);
	void showBrain(void);
	int calcState(set<int> nodes);//AT
	int calcRawState();//AT
	void showPhenotype(void);
	void savePhenotype(string phenFilename);//AT
	void saveToDotFile(char *filename);
	void saveToDotFileDiagram(char *filename, int numberOfSensors, int numberOfOutputs);//AT
	void saveToDotFileFullLayout(char *filename);//AT
	
	void initializePhysical(int x, int y, int d);
	tAgent* findLMRCA(void);
	void saveFromLMRCAtoNULL(FILE *statsFile,FILE *genomeFile);
	void saveLOD(FILE *statsFile);//,FILE *genomeFile);//@AT
	void retire(void);
	void saveLogicTable(FILE *f);
	void getLogicTable(vector<vector<int> > &t0, vector<vector<int> > &t1);//AT
	void saveStatesMap(string fn);//AT
	void saveFSM(string fn);//AT
	vector<int> attention();//AT
	void findActiveNeurons();
	void saveGenome(FILE *f);
};

#endif
