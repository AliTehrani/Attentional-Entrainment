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
#include <set>

//#define useANN

using namespace std;

//static int masterID=0; // JDSREMOVE?

//class tDot{ // JDSREMOVE? *
//public:
//	double xPos,yPos;
//};


class tAgent{
public:
	vector<tHMMU*> hmmus;
	vector<unsigned char> genome;
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
	set<int> learned, notLearned;

	tAgent();
	~tAgent();
	void setupRandomAgent(int nucleotides);
	void loadAgent(char* filename);
	void determinePhenotype(void);
	void inherit(tAgent *from,double mutationRate,int theTime);
	unsigned char * getStatesPointer(void);
	void updateStates(void);
	void resetBrain(void);
	void seedWithStartCodons(void);
	void showBrain(void);
	void showPhenotype(void);
	void savePhenotype(string phenFilename);//AT
	void saveToDotFile(char *filename);
	void saveToDotFileDiagram(char *filename, int numberOfSensors, int numberOfOutputs);//AT
	void saveToDotFileFullLayout(char *filename);
	
	void initializePhysical(int x, int y, int d);
	tAgent* findLMRCA(void);
	void saveFromLMRCAtoNULL(FILE *statsFile,FILE *genomeFile);
	void saveLOD(FILE *statsFile,FILE *genomeFile);
	void retire(void);
	void saveLogicTable(FILE *f);
	void saveGenome(FILE *f);
};

#endif
