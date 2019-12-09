
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
#include <sstream>
#include <iterator>
#include <algorithm>
#include <numeric>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

void findBestRun(tGame *game, vector<tAgent*> agent, int argc, char *argv[], int generation);
void testAgent(tGame *game, tAgent* agent, int argc, char *argv[]);

using namespace std;

//double replacementRate=0.1;
double perSiteMutationRate=0.005;
//int update=0; // generation counter
int maxAgent=100; // population size
int totalGenerations=1500;
double penaltyRatio=0.8;
int ioiMin,ioiMax;
int nrArg=1;
int argi=11;
int rElitism=5;
int repeats;
double maxScore=0, rewSh, rewEq, rewLo;

int main(int argc, char *argv[])
{
        long long t0=clock();
	vector<tAgent*>agent;
	vector<tAgent*>nextGen; // write-buffer for population
	tAgent *masterAgent; // used to clone and start LoD
	int i,j;

	int brainUpdates=1;//atoi(argv[1]);
	//ioiMin=atoi(argv[nrArg++]);
	char* trPatFn=argv[nrArg++];
	//ioiMax=atoi(argv[nrArg++]);
	char* outGenFn=argv[nrArg++];
	rewSh = atof(argv[nrArg++]);
	rewLo = atof(argv[nrArg++]);
	rewEq = atof(argv[nrArg++]);
	totalGenerations=atoi(argv[argi-1]);//+2;//AT

	vector<int> writeOutputGens;
	ifstream wog(outGenFn);
	long g;
	while(wog>>g)
	  writeOutputGens.push_back(g);

	//agent setter: setup that all agents share
	const int nrIns=1, nrOuts=1;
	int ins[nrIns]={0}, outs[nrOuts]={15};
	tAgentSetter *agentSetter=new tAgentSetter(brainUpdates,nrIns,nrOuts,ins,outs);

	//setup game trials
	ifstream pFile(trPatFn);
	vector<pair<int,int> > patterns;
	vector<vector<int> > patternObs;
	vector<int> repPat;
	string line;
	while(getline(pFile,line)){
	  istringstream iss(line);
	  int ioi, tone, ob, nrTrPat;
	  vector<int> obs;
	  iss >> ioi >> tone >> nrTrPat;
	  patterns.push_back(make_pair(ioi,tone));
	  repPat.push_back(nrTrPat);
	  while(iss>>ob)
	    obs.push_back(ob);
	  patternObs.push_back(obs);

	  //cout << ioi << " " << tone << endl << '\t';
	  //copy(obs.begin(), obs.end(), ostream_iterator<int>(cout, " "));
	  //cout << endl;
	}
	tGame *game=new tGame(patterns,patternObs,rewSh,rewLo,rewEq);
	cout << "train trials shorter:\t" << game->trainTrialsSh.size() << "\ntrain trials longer:\t" << game->trainTrialsLo.size() << endl;
	for (auto tr: game->trainTrialsSh){
	    cout << tr.size() <<endl;
	    for (auto t: tr)
		cout << t.decCorrect << " ";
	    cout << endl;
	}
	for (auto tr: game->trainTrialsLo){
	    cout << tr.size() <<endl;
	    for (auto t: tr)
		cout << t.decCorrect << " ";
	    cout << endl;
	}

	//calculate max reward
	/*
	for (int i=0; i<game->trainTrials.size(); i++){
	  maxScore+=game->trainTrials[i].rew;//trialRew;//game->trainTrials[i].rew + (game->trainTrials[i].nrIOI-game->trainTrials[i].skipIOI-1);
	  }*/

	/// setup initial population
	int randSeed;
	masterAgent=new tAgent(agentSetter); // can also loadAgent(specs...)
	
	//set random seed
	if (argc > argi && strcmp(argv[argi], "-r")==0){
	  randSeed=atoi(argv[++argi]);
	  argi++;
	}
	else 
	  randSeed=getpid();	
	srand(randSeed); // need different includes for windows XPLATFORM
	cout << "Random seed: " << randSeed << endl;

	//seeding master agent
	if (argc > argi && strcmp(argv[argi],"-s")==0){
	  char *savedAgent=argv[++argi];
	  masterAgent->loadAgent(savedAgent);
	  cout << "Master agent: " << savedAgent << endl;
	  argi++;
	}
	else{
	  masterAgent->setupRandomAgent(5000); // create with 5000 genes
	  masterAgent->determinePhenotype();
	}

	if (argc > argi && strcmp(argv[argi],"-test")==0){
	  testAgent(game,masterAgent,argc,argv);
	  long long t1=clock();
	  cout << "time to run this experiment: " << (t1-t0)*1e-6 <<endl;
	  return 0;
	}

	agent.resize(maxAgent);
	
	for(i=0;i<agent.size();i++){
	  agent[i]=new tAgent(agentSetter);
		agent[i]->inherit(masterAgent,0.01,0); // {}(*from, mu, t);
	}
	nextGen.resize(agent.size());
	masterAgent->nrPointingAtMe--; // effectively kill first ancestor from whom all inherit

	/*
	int nrTrialsLo=0, nrTrialsSh=0, nrTrialsEq=0;
	for (int i=0; i<game->trainTrialsSh.size(); i++){
	  nrTrialsSh+=game->trainTrialsSh[i].size();
	  nrTrialsLo+=game->trainTrialsLo[i].size();
	}
	//repeats=game->trainTrialsSh.size()/5;
	//maxScore=game->trainTrialsSh.size()*rewSh/5 + game->trainTrialsSh.size()*rewLo/5;
	repeats=nrTrialsSh/5;*/
	//int nrTrPat=repeats/patterns.size();
	//maxScore=rewSh*repeats+rewLo*repeats;
	//cout << "repeats: " << repeats*2 << "\tnr trials per pattern: " << nrTrPat*2 << endl;
	/*int trialInd[]={88,176,280};
	int maxScores[3];
	for (int c=0,ti=0; ti<3; ti++,c++){
	  double maxSc=0;
	  for (int i=0; i<trialInd[ti]; i++){
	    maxSc+=game->trainTrials[i].rew;
	  }
	  maxScores[c]=maxSc;
	  }*/
	double maxFitness;
	//int evPhase=0;
	///the main loop
	int update;
	for (update=0; update<totalGenerations; update++){
	  //evPhase=update%3;//int(update*3/totalGenerations);
		for(i=0;i<agent.size();i++){ // reset: fitness, fitnesses
			agent[i]->fitness=0.0;
			agent[i]->fitnesses.clear();
		}
		//random_shuffle(game->trainTrialsSh.begin(), game->trainTrialsSh.end());
		//random_shuffle(game->trainTrialsLo.begin(), game->trainTrialsLo.end());
		vector<tTrial> trials;
		for (int p=0; p<patterns.size(); p++){
		  random_shuffle(game->trainTrialsSh[p].begin(), game->trainTrialsSh[p].end());
		  random_shuffle(game->trainTrialsLo[p].begin(), game->trainTrialsLo[p].end());
		  trials.insert(trials.end(), game->trainTrialsSh[p].begin(), game->trainTrialsSh[p].begin()+repPat[p]);
		  cout << "\ntrials' size:\t" << trials.size() << '\t';  
		  trials.insert(trials.end(), game->trainTrialsLo[p].begin(), game->trainTrialsLo[p].begin()+repPat[p]);
		  cout << "\ttrials' size:\t" << trials.size() << endl;  
		}

		int shCnt=0, loCnt=0;
		for (auto tr: trials){
		    if (tr.decCorrect==0)
			shCnt++;
		    else
			loCnt++;
		}
		cout << "trials with decCorrect==0:\t" << shCnt << "trials with decCorrect==1:\t" << loCnt << endl;

		/*
		for (int t=0; t<trials.size(); t++){
		  cout << trials[t].ioi << " " << trials[t].tone << " " << trials[t].obTone << endl;
		}
		getchar();
		*/
		maxScore=0;
		for (int t=0; t<trials.size(); t++)
		  maxScore+=trials[t].rew;

		for(i=0;i<agent.size();i++){
		        for(j=0;j<trials.size();j++){
			  game->executeGame(agent[i], false, false, trials[j]);//shorter trials
			  agent[i]->fitnesses.push_back((float)agent[i]->fitness);
			}
		}
		maxFitness=0.0;
		for(i=0;i<agent.size();i++){
		  //sum up all fintesses in the fitness buffer
		  agent[i]->fitness=0.0;
		  for(j=0;j<agent[i]->fitnesses.size();j++)
		    agent[i]->fitness+=agent[i]->fitnesses[j];
		  // normalize by fitness
		  //agent[i]->fitness/=(double)repeats;
		  agent[i]->fitness *= 100.0/maxScore;//maxScores[evPhase];

		  //find best fitness
		  if(agent[i]->fitness>maxFitness){
		    maxFitness=agent[i]->fitness;
		  }
		}
		// find elites
		vector<int> elites;
		for (int r=0;r<rElitism;r++){
		    double eliteFitness=0.0;
		    int elite=0;
		    for (i=0;i<agent.size();i++){
		      /*
		        bool skip=false;
			for (j=0;j<elites.size();j++)
			  if (i==elites[j])
			    skip=true;
			if (skip)
			  continue;
		      */
		        if (find(elites.begin(), elites.end(), i) != elites.end())
			    continue;

			if(agent[i]->fitness>eliteFitness){
			  eliteFitness=agent[i]->fitness;
			  elite=i;
			}
		    }
		    elites.push_back(elite);
		    tAgent *d=new tAgent(agentSetter);
		    d->inherit(agent[elite],0,update);
		    nextGen[r]=d;
		}
		//copy(elites.begin(), elites.end(),ostream_iterator<int>(cout,","));
		//cout << endl;
		  
		//avgLearned/=agent.size();
		cout<<update<<" "<<(double)maxFitness<<endl;//<<" "<<avgLearned<<endl;
		//roulette wheel selection
		for(i=rElitism;i<agent.size();i++){
			tAgent *d;
			d=new tAgent(agentSetter);
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

		if (find(writeOutputGens.begin(), writeOutputGens.end(), update)!=writeOutputGens.end())
		  findBestRun(game, agent, argc, argv, update);
	}

	findBestRun(game, agent, argc, argv, update);

	long long t1=clock();
	cout << "time to run this experiment: " << (t1-t0)*1e-6 <<endl;
	return 0;
}

//writing simulation data for output files
void findBestRun(tGame *game, vector<tAgent*> agent, int argc, char *argv[], int generation){
  cout << "find best agent\n";

  //find best agent in population
  double maxFitness=0.0;//agent[0]->fitness;
  int bestAgent=0;
  maxScore=0;
  for (int j=0; j<game->trainTrialsSh.size(); j++)
    for (int k=0; k<game->trainTrialsSh[j].size(); k++){
      maxScore+=game->trainTrialsSh[j][k].rew;
    }

  for (int j=0; j<game->trainTrialsLo.size(); j++)
    for (int k=0; k<game->trainTrialsLo[j].size(); k++){
      maxScore+=game->trainTrialsLo[j][k].rew;
    }

  for (int i=0; i<agent.size(); i++){
        agent[i]->fitness=0;
	agent[i]->fitnesses.clear();
	//evaluate agent
	for (int j=0; j<game->trainTrialsSh.size(); j++){
	  for (int k=0; k<game->trainTrialsSh[j].size(); k++){
	    game->executeGame(agent[i], false, false, game->trainTrialsSh[j][k]);
	    agent[i]->fitnesses.push_back(agent[i]->fitness);
	  }
	}
	for (int j=0; j<game->trainTrialsLo.size(); j++){
	  for (int k=0; k<game->trainTrialsLo[j].size(); k++){
	    game->executeGame(agent[i], false, false, game->trainTrialsLo[j][k]);
	    agent[i]->fitnesses.push_back(agent[i]->fitness);
	  }
	}

	agent[i]->fitness=accumulate(agent[i]->fitnesses.begin(), agent[i]->fitnesses.end(), 0.0);
	agent[i]->fitness *= 100.0/maxScore;
	
	if (agent[i]->fitness>maxFitness){
	  bestAgent = i;
	  maxFitness = agent[i]->fitness;
	}
  }
  cout << "best agent fitness " << maxFitness << endl;
  //test best agent again to write output files
  /*
  string testVideo;
  repeats=game->testTrials.size();

  //find active neurons (it is needed for state visit map and fsm)
  agent[bestAgent]->findActiveNeurons();

  for (int j=0; j<repeats; j++){
    string vid=game->executeGame(agent[bestAgent], true, false, game->testTrials[j]);
    //cout << j << ": " << "<" << game->trainTrials.at(j).ioi <<"," << game->trainTrials.at(j).tone << "> "<< agent[bestAgent]->fitness << endl;
    testVideo+=vid;
  }

  agent[bestAgent]->fitness=accumulate(agent[bestAgent]->fitnesses.begin(), agent[bestAgent]->fitnesses.end(), 0.0);
  agent[bestAgent]->fitness *= 100.0/maxScore;
  
  //write output files
  ostringstream tFn;
  string testsFn=argv[9];
  tFn << testsFn.c_str() << generation << ".csv";
  //tFn<<testsFn.c_str()<<"_"<<generation;
  ofstream testFile(tFn.str().c_str());
  testFile<<"ioi,tone,obTone,obDelay,obPos,obAction,vid,states\n";
  testFile << testVideo;
  testFile.close();
  */
  ostringstream genomeFn;
  string gFn=argv[7];
  genomeFn << gFn.c_str() << "_" << generation << ".csv";
  FILE * genomeFile=fopen(genomeFn.str().c_str(), "w+t");
  agent[bestAgent]->saveGenome(genomeFile);

  if (generation<totalGenerations)
    return;

  FILE *LOD=fopen(argv[nrArg++],"w+t");
  //FILE *genomeFile=fopen(argv[nrArg++],"w+t");
  //char *stateVisitFn=argv[nrArg++];//AT
  nrArg++;//FILE *bestGenome=fopen(argv[nrArg++],"w+t");
  string fsmFn=argv[nrArg++];//AT
  //string testsFn=argv[nrArg++];//AT

  cout << "Best agent fitness: " << agent[bestAgent]->fitness <<endl;
  agent[bestAgent]->saveLOD(LOD);//,genomeFile);
  //agent[bestAgent]->saveGenome(bestGenome);
  
  //agent[bestAgent]->saveStatesMap((string)stateVisitFn);
  agent[bestAgent]->saveFSM((string)fsmFn);
  vector<int> transitions=agent[bestAgent]->attention();
  int nonZeroTr=0;
  for (vector<int>::iterator tr=transitions.begin(); tr!=transitions.end(); tr++)
    if (*tr>1)
      nonZeroTr++;
  cout << transitions.size() << '\t' << nonZeroTr << endl;
  //agent[bestAgent]->saveLogicTable(bestGenome);
  //agent[bestAgent]->savePhenotype((string)fsmFn);
}

void testAgent(tGame *game, tAgent* agent, int argc, char *argv[]){
  nrArg++;//FILE *LOD=fopen(argv[nrArg++],"w+t");
  //char *stateVisitFn=argv[nrArg++];//AT
  //nrArg++;//
  FILE *bestGenome=fopen(argv[nrArg++],"w+t");
  string fsmFn=argv[nrArg++];//AT
  string testsFn=argv[nrArg++];//AT

  //agent->saveLOD(LOD);//,genomeFile);
  //agent->saveGenome(bestGenome);

  //find active neurons (it is needed for state visit map)
  agent->findActiveNeurons();

  //test agent to write output files
  string testVideo;
  repeats=game->testTrials.size();
  for (int j=0; j<repeats; j++){
    string vid=game->executeGame(agent, true, false, game->testTrials[j]);
    //cout << j << ": " << "<" << game->trainTrials.at(j).ioi <<"," << game->trainTrials.at(j).tone << "> "<< agent[bestAgent]->fitness << endl;
    testVideo+=vid;
  }
  //write output files
  ofstream testFile(testsFn.c_str());
  testFile<<"ioi,tone,obTone,obDelay,obPos,obAction,vid,states\n";
  testFile << testVideo;
  testFile.close();

  //agent->saveStatesMap((string)stateVisitFn);
  agent->saveFSM((string)fsmFn);
  agent->saveLogicTable(bestGenome);
  //agent->savePhenotype((string)fsmFn);

  vector<int> transitions=agent->attention();
  int nonZeroTr=0;
  for (vector<int>::iterator tr=transitions.begin(); tr!=transitions.end(); tr++)
    if (*tr>1)
      nonZeroTr++;
  cout << transitions.size() << '\t' << nonZeroTr << endl;


  cout << "number of gates:\t" << agent->hmmus.size() << endl;
}

