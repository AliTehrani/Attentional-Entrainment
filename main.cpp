
#include <stdio.h>
#include <stdlib.h>

#include <vector>
#include <map>
#include <math.h>
#include <time.h>
#include <iostream>
#include "globalConst.h"
#include "tHMM.h"
#include "tAgent.h"
#include "tGame.h"
#include <string.h>
#include <fstream>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

//void findBestRun(tGame *game, vector<tAgent*> agent, string videoDataFile, char* brainFileName, FILE *bestGenome, string phenFilename);
void findBestRun(tGame *game, vector<tAgent*> agent, int argc, char *argv[]);

using namespace std;

//double replacementRate=0.1;
double perSiteMutationRate=0.005;
int update=0; // generation counter
int repeats=10; // # times to evaluate and gather fitness each individual (not necessary for deterministic)
int maxAgent=100; // population size
int totalGenerations=1500;
int numberOfSensors=1;
int numberOfOutputs=1;
double penaltyRatio=0.8;
int watchLength;
int toneLength;
int ioi;
int nrArg=5;

int main(int argc, char *argv[])
{
	vector<tAgent*>agent;
	vector<tAgent*>nextGen; // write-buffer for population
	tAgent *masterAgent; // used to clone and start LoD
	int i,j;
	tGame *game;
	double maxFitness;
	
	watchLength=atoi(argv[1]);
	toneLength=atoi(argv[2]);
	ioi=atoi(argv[3]);
	penaltyRatio = (double)atof(argv[4]);
	int totalGenerations=atoi(argv[11]);//+2;//AT
	/// setup initial population
	int randSeed;
	game=new tGame;
	masterAgent=new tAgent; // can also loadAgent(specs...)
	
	int argi=12;
	if (argc > argi && strcmp(argv[argi], "-r")==0){
	  //randSeed = (argc>11) ? atoi(argv[argi++]) : getpid();//these 10's should be 11 if the populdation is seeded with a saved genome
	  randSeed=atoi(argv[++argi]);
	  argi++;
	}
	else 
	  randSeed=getpid();
	
	if (argc > argi && strcmp(argv[argi],"-s")==0){
	  char *savedAgent=argv[++argi];
	  masterAgent->loadAgent(savedAgent);
	  cout << "Master agent: " << savedAgent << endl;
	}
	else{
	  masterAgent->setupRandomAgent(5000); // create with 5000 genes
	  masterAgent->determinePhenotype();
	}

	srand(randSeed); // need different includes for windows XPLATFORM
	cout << "Random seed: " << randSeed << endl;
	agent.resize(maxAgent);
	
	/*
	penaltyRatio=atoi(argv[2]);
	char *savedAgent=argv[3];
	masterAgent->loadAgent(savedAgent);
	masterAgent->showPhenotype();
	//ostringstream simplifiedDiagram;
	//simplifiedDiagram << savedAgent.str() << "_simplifiedDiagram.dat";
	char *brainFilename="simplified_genome.dot";//simplifiedDiagram.str().c_str();
	masterAgent->saveToDotFileDiagram(brainFilename, numberOfSensors, numberOfOutputs);
	game->executeGame(masterAgent, false, numberOfSensors, penaltyRatio);
	cout << savedAgent << ": " << endl;
	cout << "Fitness of the agent for the best brain, phenotype, and video: " << masterAgent->fitness << "\n";
	return 0;}
	*/
	for(i=0;i<agent.size();i++){
		agent[i]=new tAgent;
		agent[i]->inherit(masterAgent,0.01,0); // {}(*from, mu, t);
	}
	nextGen.resize(agent.size());
	masterAgent->nrPointingAtMe--; // effectively kill first ancestor from whom all inherit

	double avgLearned=0;
	int maxLearned=0;
	//tAgent *bestAgent;//AT
	///the main loop
	while(update<totalGenerations){
		for(i=0;i<agent.size();i++){ // reset: fitness, fitnesses
			agent[i]->fitness=0.0;
			agent[i]->fitnesses.clear();
		}
		for(i=0;i<agent.size();i++){
			for(j=0;j<repeats;j++){
			  game->executeGame(agent[i], false, watchLength, toneLength, ioi, penaltyRatio);
			  agent[i]->fitnesses.push_back((float)agent[i]->fitness);
			}
		}
		// find maximum fitness
		maxFitness=0.0;
		avgLearned=0;
		maxLearned=0;
		for(i=0;i<agent.size();i++){
		  avgLearned += agent[i]->learned.size();
		  //sum up all fintesses in the fitness buffer
		  agent[i]->fitness=0.0;
		  for(j=0;j<repeats;j++)
		    agent[i]->fitness+=agent[i]->fitnesses[j];
		  // normalize by fitness
		  agent[i]->fitness/=(double)repeats;
            
		  if(agent[i]->fitness>maxFitness){
				maxFitness=agent[i]->fitness;
				maxLearned=agent[i]->learned.size();
				//bestAgent = agent[i];//AT
		  }
		}
		avgLearned/=agent.size();
		cout<<update<<" "<<(double)maxFitness<<endl;//<<" "<<avgLearned<<endl;
		//roulette wheel selection
		for(i=0;i<agent.size();i++){
			tAgent *d;
			d=new tAgent;
			do{
			  j=rand()%(int)agent.size();
			} while(randDouble>(pow(1.05, agent[j]->fitness) / pow(1.05, maxFitness)));
            
			d->inherit(agent[j],perSiteMutationRate,update);
			nextGen[i]=d;
		}
		// moves "nextGen" to current population
		for(i=0;i<agent.size();i++){
			agent[i]->retire();
			agent[i]->nrPointingAtMe--;
			if(agent[i]->nrPointingAtMe==0)
				delete agent[i];
			agent[i]=nextGen[i];
		}
		agent=nextGen;
		update++;
	}
	findBestRun(game, agent, argc, argv);//game, agent, videoDataFile, brainFileName, bestGenome, phenFilename);
	//agent[0]->ancestor->ancestor->saveLOD(LOD,genomeFile);
	//agent[0]->ancestor->ancestor->determinePhenotype(); // transcribe genetic material
	//agent[0]->ancestor->ancestor->showPhenotype();
	return 0;
}

//writing simulation data for video
//void findBestRun(tGame *game, vector<tAgent*> agent, string videoDataFile, char* brainFileName, FILE *bestGenome, string phenFilename){//AT->
void findBestRun(tGame *game, vector<tAgent*> agent, int argc, char *argv[]){
  FILE *LOD=fopen(argv[nrArg++],"w+t");
  FILE *genomeFile=fopen(argv[nrArg++],"w+t");	
  char *brainFileName=argv[nrArg++];//AT
  FILE *bestGenome=fopen(argv[nrArg++],"w+t");
  string phenFilename=argv[nrArg++];//AT
  string videoDataFile=argv[nrArg++];//AT

  double maxFitness=agent[0]->fitness;
  tAgent *bestAgent=agent[0];
  for (int i=0; i<agent.size(); i++){
        for (int j=0; j<repeats; j++){
	  game->executeGame(agent[i], false, watchLength, toneLength, ioi, penaltyRatio);
	  agent[i]->fitnesses.push_back((float)agent[i]->fitness);
	}
	agent[i]->fitness=0.0;
	for(int j=0;j<repeats;j++)
	  agent[i]->fitness+=agent[i]->fitnesses[j];
	agent[i]->fitness/=(double)repeats;
		  
	if (agent[i]->fitness>maxFitness){
	  bestAgent = agent[i];
	  maxFitness = agent[i]->fitness;
	}
  }
  cout << "Best agent fitness: " << bestAgent->fitness <<endl;
  string bestVideo;
  for (int j=0; j<repeats; j++){
    string vid=game->executeGame(bestAgent, true, watchLength, toneLength, ioi, penaltyRatio);
    bestAgent->fitnesses.push_back((float)bestAgent->fitness);
    bestVideo+=vid;
  }
  bestAgent->fitness=0.0;
  for(int j=0;j<repeats;j++)
    bestAgent->fitness+=bestAgent->fitnesses[j];
  bestAgent->fitness/=(double)repeats;

  ofstream outFile(videoDataFile.c_str());
  outFile<< bestVideo;
  outFile.close();
  bestAgent->saveToDotFile(brainFileName);
  bestAgent->saveGenome(bestGenome);
  bestAgent->showPhenotype();
  bestAgent->savePhenotype(phenFilename);
  bestAgent->saveLOD(LOD,genomeFile);
  cout << "Fitness of the agent for the best brain, phenotype, and video: " << bestAgent->fitness << "\n";
  
}
