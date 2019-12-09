/*
 *  tGame.h
 *  HMMBrain
 *
 *  Created by Arend on 10/09/23.
 *  Adapted by Jory on 13/12/04
 *
 */
 
#ifndef _tGame_h_included_
#define _tGame_h_included_

#include "globalConst.h"
#include "tAgent.h"
#include <vector>
#include <map>
#include <set>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

class tTrial{
public:
    int nrIOI,skipIOI,ioi,tone,obTone,obDelay,obPos, obPitch;
    int decTime, decCorrect;
    double rew;//, re, reOB, rs, rl;
    vector<char> signal;//, decision;
    //vector<double> reward;//, penalty;

    tTrial();
    tTrial(int i, int t, int obT, int obP, int obD, double rewSh, double rewLo, double rewEq);
    ~tTrial();

    void printTrial(ostream &out);
    void doIOI(int pos);
    void doOB(int dec);
};

class tGame{
public:
    int scoreTable[83][9];
    string executeGame(tAgent* agent, bool test, bool report, tTrial trial);
    //string executeTest(tAgent* agent, bool report, int brainUpdates, const double &penaltyRatio, int caseIndex);

    vector<tTrial> testTrials;
    vector<vector<tTrial> > trainTrialsSh, trainTrialsLo, trainTrialsEq;
    tGame(vector<pair<int, int> > patterns, vector<vector<int> > patternObs, double rewSh, double rewLo, double rewEq);
	~tGame();
	double mutualInformation(vector<int> A,vector<int>B);
	double ei(vector<int> A,vector<int> B,int theMask);
	double computeAtomicPhi(vector<int>A,int states);
	double predictiveI(vector<int>A);
	double nonPredictiveI(vector<int>A);
	double predictNextInput(vector<int>A);
	double computeR(vector<vector<int> > table,int howFarBack);
	double computeOldR(vector<vector<int> > table);
	double entropy(vector<int> list);
	
	//void fillTrials(int ioiMin, int ioiMax, int toneMin, int toneMax);

    void computeAllMI(char *filename);
    void makeAllSets(char *filename,set<int> target,set<int> source);
    double doInformationCombination(set<int> combo);


};
#endif
