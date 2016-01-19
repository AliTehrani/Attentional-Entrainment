/*
 *  tGame.cpp
 *  HMMBrain
 *
 *  Created by Arend on 9/23/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "tGame.h"
#include <math.h>
#include <fstream>
#include <sstream>
using std::ofstream;
using std::ostringstream;

tGame::tGame(){
}

tGame::~tGame(){
}

string tGame::executeGame(tAgent* agent, bool report, int watchLength, int ioiMin, int ioiMax, const double &penaltyRatio){
    const int totalIOI = 20;
    int auditIndex=brainSize-2;//watchLength;
    int outputIndex=brainSize-1;
    double reward=1;
    double penalty=reward*penaltyRatio;
    int action, oddball, oddballCount=0, actToneLen;
    double oddballProb=0.5;
    ostringstream oss;
    agent->fitness=0.0;

    int ioi=ioiMin+rand()%(ioiMax-ioiMin+1);
    int toneLength=1+rand()%(ioi-2);
    if (watchLength==0)
      watchLength=ioi;
    
    if (report)
      oss<<watchLength<<","<<toneLength<<","<<ioi<<","<<totalIOI<<'\n';

    for (int i=0; i<4; i++){
      for (int step=0; step<ioi; step++){
	agent->states[auditIndex]=(step<toneLength);
	
	for (int w=0; w<watchLength; w++)
	  agent->states[w]=0;
	agent->states[step*(ioi+1)%watchLength]=1;
	
	if (report){
	  for (int s=0; s<brainSize-1; s++)
	    oss<<int(agent->states[s]&1)<<",";
	  oss<<int(agent->states[brainSize-1]&1)<<'\n';
	}
	
	agent->updateStates();
      }
    }
      
    for (int i=0; i<totalIOI; i++){
      oddball=0;
      if (randDouble<oddballProb){
	actToneLen=toneLength+(1+rand()%(ioi-toneLength-1));
	oddball=1;
	oddballCount++;
      }
      else
	actToneLen=toneLength;
      
      for (int step=0; step<ioi; step++){
	//hear the tone
	int isTone=(step<actToneLen)? 1 : 0;
	agent->states[auditIndex]=isTone;
	
	//clear the watch
	for (int i=0; i<watchLength; i++)
	  agent->states[i]=0;
	//set the watch
	agent->states[(step*(ioi+1))%watchLength]=1;
	
	//agent->showBrain();
	if (report){
	  for (int i=0; i<watchLength; i++)
	    oss<<int(agent->states[i]&1)<<",";
	  oss<<int(agent->states[auditIndex]&1)<<'\n';
	}
	
	//update brain
	agent->updateStates();
      }
      action=agent->states[outputIndex];
      
      //score brains
      if (oddball==1 && action==1)
	agent->fitness += reward;
      else if ((oddball==1 && action==0) || (oddball==0 && action==1))
	agent->fitness -= penalty;
      
      if (report)
	oss<<action<<","<<agent->fitness<<'\n';
    }

    //cout << oddballCount <<'\n';
    //int test; cin>>test;
    //agent->fitness = (agent->fitness/oddballCount)*100;
    agent->fitness *= 100.0/oddballCount;
    string reportString=oss.str();
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




