/*
 *  tGame.cpp
 *  HMMBrain
 *
 *  Created by Arend on 9/23/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "tGame.h"
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iterator>

#define whenAct (ioi-1) //act after tone or ioi (after tone: step or tone+1, after ioi: ioi-1)
#define oddballPitch 1 //double signal oddball pitch: 3, different sensor: 2, same oddball pitch: 1 (NOTE THAT NUMBER OF INPUTS AND THEIR SET SHOULD BE SET ACCORDINGLY in main.cpp)

using namespace std;

void tTrial::doIOI(int pos){
  int step;
  for (step=0; step<tone; step++)
    signal.at(pos*ioi+step)=1;
  
  //decision.at(pos*ioi+whenAct)=1;
  //if (pos>=skipIOI){
  //reward.at(pos*ioi+whenAct)=1.0;
    //penalty.at(pos*ioi+whenAct)=pen;
  //}
}

void tTrial::doOB(int dec){
  int step;
  for (step=obDelay; step<obDelay+obTone; step++)
    signal.at(obPos*ioi+step)=obPitch;
  //first step
  //signal.at(obPos*ioi+obDelay)=3;//oddballPitch should be 1
  //entire ioi
  //for (int s=obDelay+obTone; s<obDelay+ioi; s++)
  //signal.at(obPos*ioi+s)=2;//oddballPitch should be 3

  //decision.at(obPos*ioi+whenAct)=dec;
  //reward.at(obPos*ioi+whenAct)=rew;
}

tTrial::tTrial(int i, int t, int obT, int obP, int obD, double rewSh, double rewLo, double rewEq){
  ioi=i;
  tone=t;
  obTone=obT;
  obPos=obP;
  obDelay=obD;
  nrIOI=9;
  skipIOI=4;
  //rs=rewSh;
  //rl=rewLo;
  //re=rewEq;

  //adjust reward with respect to number of each type of trial
  //for ioi:6-10 and tone 3-4 for ioi in [6,8] and tone 4-5 for ioi in [9,10]: rl=80/31, rs=80/29
  //for ioi in [6,8] and tone 3-4: rl=rs=42/15
  if (obTone>tone)
    rew=rewLo;//80.0/31;
  else if (obTone<tone)
    rew=rewSh;//80.0/29;
  else
    rew=rewEq;
  
  //pen=1.0/(nrIOI-1);
  
  //oddball pitch: same(1) or different(2)
  obPitch=oddballPitch;

  signal.resize(nrIOI*ioi,0);
  //decision.resize(nrIOI*ioi,5);
  //reward.resize(nrIOI*ioi,0);
  //penalty.resize(nrIOI*ioi,0);

  int pos, step;
  //before oddball
  for (pos=0; pos<obPos; pos++){
    doIOI(pos);
  }

  //oddball
  //int dec;
  decTime=obPos*ioi+whenAct;
  if (obTone>tone)
    decCorrect=1;
  else if(obTone<tone)
    decCorrect=0;
  else
    decCorrect=-1;

  doOB(decCorrect);

  //after oddball
  for (pos=obPos+1; pos<nrIOI; pos++){
    doIOI(pos);
  }
}

tTrial::tTrial(){
}

tTrial::~tTrial(){
}

void tTrial::printTrial(ostream &out){
  out<<ioi<<","<<tone<<","<<obTone<<","<<obDelay<<","<<obPos<<",";
  //cout << decTime << "," << decCorrect << ",";
  //copy(signal.begin(), signal.end(), ostream_iterator<unsigned char>(cout,','));
  /*for (int i=0; i<signal.size(); i++)
    cout << (int)signal.at(i);
  cout << endl;
  */
  //copy(decision.begin(), decision.end(), ostream_iterator<unsigned char>(cout,','));
  /*for (int i=0; i<decision.size(); i++)
    cout << (int)decision.at(i);
    cout << endl;  
  
  for (int i=0; i<reward.size(); i++)
    cout << reward.at(i);
  cout << endl;  
  for (int i=0; i<penalty.size(); i++)
    cout << (penalty.at(i)>0);
  cout << endl;
  */
}

tGame::tGame(vector<pair<int, int> > patterns, vector<vector<int> > patternObs, double rewSh, double rewLo, double rewEq){
  //for (int ioi=ioiMin; ioi<=ioiMax; ioi++)
  //for (int tone=toneMin; tone<=toneMax; tone++)
  trainTrialsSh.resize(patterns.size());
  trainTrialsLo.resize(patterns.size());
  trainTrialsEq.resize(patterns.size());
  for (vector<pair<int, int> >::iterator p=patterns.begin(); p!=patterns.end(); p++){
    int ioi=p->first, tone=p->second;
    int patInd=p-patterns.begin();
    if (patInd>=patternObs.size()){
	cout << patInd << " " << endl;
	int sth; cin >> sth;
    }
    for (int obPos=4; obPos<=7; obPos++)
	for (vector<int>::iterator ob=patternObs[patInd].begin(); ob!=patternObs[patInd].end(); ob++){
	  int obTone=(*ob);
	  //if (obTone==tone)
	  //continue;
	  tTrial c(ioi, tone, obTone, obPos, 0, rewSh, rewLo, rewEq);
	  if (obTone<tone)
	    trainTrialsSh[patInd].push_back(c);
	  else if (obTone>tone)
	    trainTrialsLo[patInd].push_back(c);
	  else
	    trainTrialsEq[patInd].push_back(c);
	  for (int delay=tone-ioi+1; delay<=ioi-obTone-1; delay++){
	    //if (obTone==tone || delay==0){
	      tTrial c(ioi, tone, obTone, obPos, delay, rewSh, rewLo, rewEq);
	      testTrials.push_back(c);
	      //}
	  }
	}
  }
}

tGame::~tGame(){
}

int auditIndex=0;
int outputIndex=brainSize-1;
int pos=0;

string tGame::executeGame(tAgent* agent, bool test, bool report, tTrial trial){
    int action, obAction=-3, sound;
    ostringstream vid,vidStates,vidRawStates;

    vidStates << "\"";
    vidRawStates << "\"";
    int nrDec=0;
    agent->fitness=0;
    agent->resetBrain();

    for (int time=0; time<trial.ioi*trial.nrIOI; time++){
        int step=time%trial.ioi, pos=time/trial.ioi;
	//hear the tone
	sound=(trial.signal[time]);
	vector<int> sense;
	sense.push_back(int(sound%2));
	sense.push_back(int(sound/2));
	agent->setSensors(sense);
	
	//the states of input neurons are cleared and updates in "hearing", so if they are written to in last update their values are lost which does not matter if brainUpdates is 1
	long state1,state2;
	//write states to file
	if (test){
	  vidRawStates<<agent->calcRawState()<<",";
	  vidStates<<agent->calcState(agent->actNeurons)<<",";
	  state1=agent->calcState(agent->actNeurons);
	}
	
//*********************************update brain**********************************************
	for (int b=0; b<agent->brainUpdates; b++)
	  agent->updateStates();
	
	if (test){
	  state2=agent->calcState(agent->actNeurons);
	  if (trial.obDelay==0){// && trial.obTone==trial.tone){
	    pair<long,long> key(state1,state2);
	    agent->fsm[key].insert(sound);
	  }
	}
	/*
	//is brain expressing decision?
	action=agent->getAction();
	if (pos+1>trial.skipIOI && nrDec==0 && action!=1){
	  //score brain
	  if (action==trial.decCorrect)
	    agent->fitness+=trial.rew;
	  //save stuff
	  if (test){
	    obAction=action;
	    agent->fsmColors.insert(state2);
	  }
	  nrDec++;
	}
	*/
	//is time to score brain?
	if (time==trial.decTime){
	  //get action
	  action=agent->getAction();
	  //score brain
	  if (action==trial.decCorrect)
	    agent->fitness+=trial.rew;
	  //save decision for oddball
	  if (test){
	      obAction=action;
	      if (trial.obDelay==0)
		agent->fsmColors.insert(state2);
	  }
	}
	/*
	//state visit map
	if (test){
	  ostringstream seq;
	  trial.printTrial(seq);
	  seq<<step<<","<<pos<<",";
	  //pair<string,int> seq_state(seq.str().c_str(), agent->calcState(agent->actNeurons));
	  pair<string,int> seq_state(seq.str().c_str(), agent->calcRawState());
	  agent->visStates[seq_state]++;
	}
	*/
    }
    
    //agent->fitness *= 100.0/trial.rew;
    
    if (test){
      trial.printTrial(vid);
      vid << obAction  << "," << vidStates.str().substr(0,vidStates.str().size()-1) << "\"" << "," << vidRawStates.str().substr(0,vidRawStates.str().size()-1) << "\"" << '\n';
    }
    string reportString=vid.str();
    return reportString;
}
  
double tGame::mutualInformation(vector<int> A,vector<int>B){
	set<int> nrA,nrB;
	set<int>::iterator aI,bI;
	map<int,map<int,double> > pXY;
	map<int,double> pX,pY;
	int i,j;
	double c=1.0/(double)A.size();
	double I=0.0;
	for(i=0;i<A.size();i++){
		nrA.insert(A[i]);
		nrB.insert(B[i]);
		pX[A[i]]=0.0;
		pY[B[i]]=0.0;
	}
	for(aI=nrA.begin();aI!=nrA.end();aI++)
		for(bI=nrB.begin();bI!=nrB.end();bI++){
			pXY[*aI][*bI]=0.0;
		}
	for(i=0;i<A.size();i++){
		pXY[A[i]][B[i]]+=c;
		pX[A[i]]+=c;
		pY[B[i]]+=c;
	}
	for(aI=nrA.begin();aI!=nrA.end();aI++)
		for(bI=nrB.begin();bI!=nrB.end();bI++)
			if((pX[*aI]!=0.0)&&(pY[*bI]!=0.0)&&(pXY[*aI][*bI]!=0.0))
				I+=pXY[*aI][*bI]*log2(pXY[*aI][*bI]/(pX[*aI]*pY[*bI]));
	return I;
	
}

double tGame::entropy(vector<int> list){
	map<int, double> p;
	map<int,double>::iterator pI;
	int i;
	double H=0.0;
	double c=1.0/(double)list.size();
	for(i=0;i<list.size();i++)
		p[list[i]]+=c;
	for (pI=p.begin();pI!=p.end();pI++) {
			H+=p[pI->first]*log2(p[pI->first]);	
	}
	return -1.0*H;
}

double tGame::ei(vector<int> A,vector<int> B,int theMask){
	set<int> nrA,nrB;
	set<int>::iterator aI,bI;
	map<int,map<int,double> > pXY;
	map<int,double> pX,pY;
	int i,j;
	double c=1.0/(double)A.size();
	double I=0.0;
	for(i=0;i<A.size();i++){
		nrA.insert(A[i]&theMask);
		nrB.insert(B[i]&theMask);
		pX[A[i]&theMask]=0.0;
		pY[B[i]&theMask]=0.0;
	}
	for(aI=nrA.begin();aI!=nrA.end();aI++)
		for(bI=nrB.begin();bI!=nrB.end();bI++){
			pXY[*aI][*bI]=0.0;
		}
	for(i=0;i<A.size();i++){
		pXY[A[i]&theMask][B[i]&theMask]+=c;
		pX[A[i]&theMask]+=c;
		pY[B[i]&theMask]+=c;
	}
	for(aI=nrA.begin();aI!=nrA.end();aI++)
		for(bI=nrB.begin();bI!=nrB.end();bI++)
			if((pX[*aI]!=0.0)&&(pY[*bI]!=0.0)&&(pXY[*aI][*bI]!=0.0))
				I+=pXY[*aI][*bI]*log2(pXY[*aI][*bI]/(pY[*bI]));
	return -I;
}
double tGame::computeAtomicPhi(vector<int>A,int states){
	int i;
	double P,EIsystem;
	vector<int> T0,T1;
	T0=A;
	T1=A;
	T0.erase(T0.begin()+T0.size()-1);
	T1.erase(T1.begin());
	EIsystem=ei(T0,T1,(1<<states)-1);
	P=0.0;
	for(i=0;i<states;i++){
		double EIP=ei(T0,T1,1<<i);
//		cout<<EIP<<endl;
		P+=EIP;
	}
//	cout<<-EIsystem+P<<" "<<EIsystem<<" "<<P<<" "<<T0.size()<<" "<<T1.size()<<endl;
	return -EIsystem+P;
}



double tGame::computeR(vector<vector<int> > table,int howFarBack){
	double Iwh,Iws,Ish,Hh,Hs,Hw,Hhws,delta,R;
	int i;
	for(i=0;i<howFarBack;i++){
		table[0].erase(table[0].begin());
		table[1].erase(table[1].begin());
		table[2].erase(table[2].begin()+(table[2].size()-1));
	}
	table[4].clear();
	for(i=0;i<table[0].size();i++){
		table[4].push_back((table[0][i]<<14)+(table[1][i]<<10)+table[2][i]);
	}
	Iwh=mutualInformation(table[0],table[2]);
    Iws=mutualInformation(table[0],table[1]);
    Ish=mutualInformation(table[1],table[2]);
    Hh=entropy(table[2]);
    Hs=entropy(table[1]);
    Hw=entropy(table[0]);
    Hhws=entropy(table[4]);
    delta=Hhws+Iwh+Iws+Ish-Hh-Hs-Hw;
    R=Iwh-delta;
  	return R;
}

double tGame::computeOldR(vector<vector<int> > table){
	double Ia,Ib;
	Ia=mutualInformation(table[0], table[2]);
	Ib=mutualInformation(table[1], table[2]);
	return Ib-Ia;
}

double tGame::predictiveI(vector<int>A){
	vector<int> S,I;
	S.clear(); I.clear();
	for(int i=0;i<A.size();i++){
		S.push_back((A[i]>>12)&15);
		I.push_back(A[i]&3);
	}
	return mutualInformation(S, I);
}

double tGame::nonPredictiveI(vector<int>A){
	vector<int> S,I;
	S.clear(); I.clear();
	for(int i=0;i<A.size();i++){
		S.push_back((A[i]>>12)&15);
		I.push_back(A[i]&3);
	}
	return entropy(I)-mutualInformation(S, I);
}
double tGame::predictNextInput(vector<int>A){
	vector<int> S,I;
	S.clear(); I.clear();
	for(int i=0;i<A.size();i++){
		S.push_back((A[i]>>12)&15);
		I.push_back(A[i]&3);
	}
	S.erase(S.begin());
	I.erase(I.begin()+I.size()-1);
	return mutualInformation(S, I);
}

void tGame::computeAllMI(char *filename){
    set<int> target,source;
    int i;
    for(i=0;i<9;i++)
        source.insert(i);
    makeAllSets(filename,target,source);
}

void tGame::makeAllSets(char *filename,set<int> target,set<int> source){
    if(source.size()==0){
        double d;
        set<int>::iterator SI;
        FILE *F=fopen(filename,"a+t");
        for(SI=target.begin();SI!=target.end();SI++){
            cout<<(*SI)+1;
            fprintf(F,"%i",(*SI+1));
        }
        d=doInformationCombination(target);
        fprintf(F," %i  %f\n",target.size(),d);
        cout<<" "<<target.size()<<" "<<d<<endl;
        fclose(F);
    } else {
        set<int>::iterator SI;
        int i;
        SI=source.begin();
        i=(*SI);
        source.erase(SI);
        makeAllSets(filename,target, source);
        target.insert(i);
        makeAllSets(filename,target, source);
    }
}

double tGame::doInformationCombination(set<int> combo){
    double r=0.0;
    vector<int> A,B;
    int i,j;
    int a,b;
    set<int>::iterator SI;
    for(i=0;i<83;i++)
        for(j=0;j<83;j++)
            if(i!=j){
                if(i<j)
                    b=0;
                else
                    b=1;
                a=0;
                for(SI=combo.begin();SI!=combo.end();SI++){


                    a=(a<<1)+scoreTable[i][(*SI)];
                    a=(a<<1)+scoreTable[j][(*SI)];
                }
                A.push_back(a);
                B.push_back(b);
            }
    return mutualInformation(A, B);
}




