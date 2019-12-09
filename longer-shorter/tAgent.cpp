/*
 *  tAgent.cpp
 *  HMMBrain
 *
 *  Created by Arend on 9/16/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "tAgent.h"

tAgent::tAgent(tAgentSetter *setter){
    brainUpdates=setter->brainUpdates;
    inputNodes=setter->inputNodes;
    outputNodes=setter->outputNodes;
  

	int i;
	nrPointingAtMe=1;
	ancestor=NULL;
	for(i=0;i<brainSize;i++){
		states[i]=0;
		newStates[i]=0;
	}
	saved=false;
	hmmus.clear();
	nrOfOffspring=0;
	retired=false;
#ifdef useANN
	ANN=new tANN;
#endif
}

tAgent::~tAgent(){
	// Recursive Destructor of useless related agents
	for(int i=0;i<hmmus.size();i++)
		delete hmmus[i]; // Destroy all markov units
	if(ancestor!=NULL){ // recursively destroy ancestors if necessary
		ancestor->nrPointingAtMe--;
		if(ancestor->nrPointingAtMe==0)
			delete ancestor;
	}
#ifdef useANN
	delete ANN;
#endif
}

void tAgent::setupRandomAgent(int nucleotides){
	int i;
	genome.resize(nucleotides);
	for(i=0;i<nucleotides;i++)
		genome[i]=127;//rand()&255;
	seedWithStartCodons(); // TODO what is this?
	determinePhenotype(); // transcribe genetic material
#ifdef useANN
	ANN->setup();
#endif
}
void tAgent::loadAgent(char* filename){
	// Init agent's genome from saved data
	// and create Phenotype
	FILE *f=fopen(filename,"r+t");
	int i;
	genome.clear();
	while(!(feof(f))){
		fscanf(f,"%i	",&i);
		genome.push_back((unsigned char)(i&255));
	}
        
	fclose(f);
    	determinePhenotype();
	born=0;
}

void tAgent::seedWithStartCodons(void){
	// Ensure >=4 start codons exist in the genome at least 100 from end
	int i,j;
	for(i=0;i<genome.size();i++)
		genome[i]=rand()&255;
	for(i=0;i<16;i++) //@AT increase number of provided start codons in the genome from 4 to 12
	{
		j=rand()%(genome.size()-100);
		genome[j]=42; // create start codon part 1
		genome[j+1]=(255-42); // start codon part 2
		for(int k=2;k<20;k++) 
			genome[j+k]=rand()&255;
	}
}

void tAgent::inherit(tAgent *from,double mutationRate,int theTime){
	int nucleotides=from->genome.size();
	int i,s,o,w;
	vector<unsigned char> buffer;
	born=theTime;
	ancestor=from;
	from->nrPointingAtMe++;
	from->nrOfOffspring++;
	genome.clear();
	genome.resize(from->genome.size());
	for(i=0;i<nucleotides;i++)
		if(((double)rand()/(double)RAND_MAX)<mutationRate)
			genome[i]=rand()&255;
		else
			genome[i]=from->genome[i];
	if((((double)rand()/(double)RAND_MAX)<0.05)&&(genome.size()<20000)){
		//duplication
		w=15+rand()&511;
		s=rand()%(genome.size()-w);
		o=rand()%genome.size();
		buffer.clear();
		buffer.insert(buffer.begin(),genome.begin()+s,genome.begin()+s+w);
		genome.insert(genome.begin()+o,buffer.begin(),buffer.end());
	}
	if((((double)rand()/(double)RAND_MAX)<0.02)&&(genome.size()>1000)){
		//deletion
		w=15+rand()&511;
		s=rand()%(genome.size()-w);
		genome.erase(genome.begin()+s,genome.begin()+s+w);
	}
	determinePhenotype();
	fitness=0.0;
#ifdef useANN
	ANN->inherit(ancestor->ANN,mutationRate);
#endif
}

void tAgent::determinePhenotype(void){
	// transcribe, synthesize, etc.
	int i;
	tHMMU *hmmu;
	if(hmmus.size()!=0)
		for(i=0;i<hmmus.size();i++)
			delete hmmus[i];
	hmmus.clear();
	for(i=0;i<genome.size();i++){
		if((genome[i]==42)&&(genome[(i+1)%genome.size()]==(255-42))){
			hmmu=new tHMMU;
			hmmu->setupQuick(genome,i);
			hmmus.push_back(hmmu);
		}
        
		if((genome[i]==43)&&(genome[(i+1)%genome.size()]==(255-43))){
			hmmu=new tHMMU;
			hmmu->setupQuick(genome,i);
			hmmus.push_back(hmmu);
		}
	}
}

void tAgent::retire(void){
	// saves every 32nd generation?
    if((born&31)!=1) 
        retired=true;
}

unsigned char * tAgent::getStatesPointer(void){
	return states;
}

void tAgent::setStates(vector<int> t){
  for (int s=0; s<brainSize; s++)
    states[s]=t[s];
}

void tAgent::setSensors(vector<int> sensors){
  int s=0;
  for (set<int>::iterator it=inputNodes.begin(); it!=inputNodes.end(); it++){
    states[(*it)]=sensors.at(s);
    s++;
  }
}

int tAgent::getAction(){
  int action=0;
  for (set<int>::iterator it=outputNodes.begin(); it!=outputNodes.end(); it++)
    action+=states[(*it)];
  return action;
}

void tAgent::resetBrain(void){
	for(int i=0;i<brainSize;i++)
		states[i]=0;
#ifdef useANN
	ANN->resetBrain();
#endif
}

void tAgent::trembleBrain(int T){
    resetBrain();
    for (int t=0; t<T; t++){
        vector<int> rndIn;
	for (int i=0; i<inputNodes.size(); i++)
 	    rndIn.push_back(rand()%2);
	setSensors(rndIn);	
	updateStates();
    }
}

void tAgent::updateStates(void){
#ifdef useANN
	ANN->update(&states[0]);
#else
	int i;
	for(i=0;i<hmmus.size();i++)
		hmmus[i]->update(&states[0],&newStates[0]);
	for(i=0;i<brainSize;i++){
		states[i]=newStates[i];
		newStates[i]=0;
	}
#endif
}

void tAgent::showBrain(void){
	// diagnostic Function
	for(int i=0;i<brainSize;i++)
		cout<<(int)states[i];
	cout<<endl;
}

int tAgent::calcState(set<int> nodes){
  int state=0;
  for (set<int>::iterator it=nodes.begin(); it!=nodes.end(); it++){
      //The reason I kept the original brain node index is to be able to translate any FSM state to a brain state regardless of which brain states are ignored
    state += pow(2, brainSize-1 - (*it))* (int)states[*it];
  }
  return state;
}

int tAgent::calcRawState(){
  int state=0;
  for (int s=0; s<brainSize; s++)
    state += pow(2, brainSize-1 - s)* (int)states[s];
  return state;
}

void tAgent::initializePhysical(int newx, int newy, int newdirection){
	int i,j;
	unsigned char dummy;
	x=newx;
	y=newy;
	direction=newdirection;
}

tAgent* tAgent::findLMRCA(void){
	tAgent *r,*d;
	if(ancestor==NULL)
		return NULL;
	else{
		r=ancestor;
		d=NULL;
		while(r->ancestor!=NULL){
			if(r->ancestor->nrPointingAtMe!=1)
				d=r;
			r=r->ancestor;
		}
		return d;
	}
}

void tAgent::saveFromLMRCAtoNULL(FILE *statsFile,FILE *genomeFile){
	if(ancestor!=NULL)
		ancestor->saveFromLMRCAtoNULL(statsFile,genomeFile);
	if(!saved){ 
		fprintf(statsFile,"%i	%i	%i	%f	%i	%f	%i	%i\n",ID,born,genome.size(),fitness,correct,incorrect);
		fprintf(genomeFile,"%i	",ID);
		for(int i=0;i<genome.size();i++)
			fprintf(genomeFile,"	%i",genome[i]);
		fprintf(genomeFile,"\n");
		saved=true;
	}
	if((saved)&&(retired)) genome.clear(); 
}

// Save line of descent
void tAgent::saveLOD(FILE *statsFile){//,FILE *genomeFile){
	if(ancestor!=NULL)
	  ancestor->saveLOD(statsFile);//,genomeFile);
#ifdef useANN
	//fprintf(genomeFile,"%i	",ID);//@AT
	fprintf(statsFile,"%i	%i	%i	%f	%i	%f	%i	%i\n",ID,born,(int)genome.size(),fitness,correct,incorrect);
	//ANN->saveLOD(genomeFile);//@AT
#else	
	fprintf(statsFile,"%i,%.2f\n",born,fitness);
	//for (set<int>::iterator it=learned.begin(); it!=learned.end(); it++)
	//fprintf(statsFile,"%i,",(*it));
	//fprintf(statsFile,"\b\",\"");
	//for (set<int>::iterator it=notLearned.begin(); it!=notLearned.end(); it++)
	//fprintf(statsFile,"%i,",(*it));
	//fprintf(statsFile,"\b\"\n");
	
	/*@AT
	if(!retired){
	  for(int i=0;i<genome.size();i++)
            fprintf(genomeFile,"%i  ",genome[i]);
	  fprintf(genomeFile,"\n");
	  }*/
#endif
	
}

void tAgent::showPhenotype(void){
	// diagnostic function
	for(int i=0;i<hmmus.size();i++)
		hmmus[i]->show();
	cout<<"------"<<endl;
}

// @AT ->
void tAgent::savePhenotype(string phenFilename){
  ofstream phenotypeFile(phenFilename.c_str());
  phenotypeFile<<"fitness: "<<this->fitness<<"\n\n";
  for (int i=0;i<hmmus.size();i++)
    hmmus[i]->save(phenotypeFile);
  phenotypeFile<<"------"<<endl;
}
// <- @AT

void tAgent::saveToDotFile(char *filename){
	FILE *f=fopen(filename,"w+t");
	int i,j,k;
	fprintf(f,"digraph brain {\n");
	fprintf(f,"	ranksep=2.0;\n");
	for(i=0;i<9;i++)
		fprintf(f,"	%i [shape=invtriangle,style=filled,color=red];\n",i);
	for(i=9;i<18;i++)
		fprintf(f,"	%i [shape=invtriangle,style=filled,color=orange];\n",i);
	for(i=18;i<29;i++)
		fprintf(f,"	%i [shape=circle,color=blue];\n",i);
	for(i=29;i<32;i++)
		fprintf(f,"	%i [shape=circle,style=filled,color=green];\n",i);
	for(i=0;i<hmmus.size();i++){
	//	fprintf(f,"	{\n");
		for(j=0;j<hmmus[i]->ins.size();j++){
			for(k=0;k<hmmus[i]->outs.size();k++)
				fprintf(f,"	%i	->	%i;\n",hmmus[i]->ins[j],hmmus[i]->outs[k]);
		}
	//	fprintf(f,"	}\n");
	}
	fprintf(f,"	{ rank=same; 0;  1;  2;  3;  4;  5;  6;  7; 8;}\n"); 
	fprintf(f,"	{ rank=same; 9; 10; 11; 12; 13; 14; 15; 16; 17; }\n"); 
	fprintf(f,"	{ rank=same; 18; 19; 20; 21; 22; 23; 24; 25; 26; 27; 28;}\n"); 
	fprintf(f,"	{ rank=same; 29; 30; 31; }\n"); 
	fprintf(f,"}\n");
	fclose(f);
}

void tAgent::saveToDotFileFullLayout(char *filename){
	FILE *f=fopen(filename,"w+t");
	int i,j,k;
	fprintf(f,"digraph brain {\n");
	fprintf(f,"	ranksep=2.0;\n");
	for(i=0;i<hmmus.size();i++){
		fprintf(f,"MM_%i [shape=box]\n",i);
		for(j=0;j<hmmus[i]->ins.size();j++)
			fprintf(f,"	t0_%i -> MM_%i\n",hmmus[i]->ins[j],i);
		for(k=0;k<hmmus[i]->outs.size();k++)
			fprintf(f,"	MM_%i -> t1_%i\n",i,hmmus[i]->outs[k]);
		
	}
	fprintf(f,"}\n");
}

// @AT ->
void tAgent::saveToDotFileDiagram(char *filename, int numberOfSensors, int numberOfOutputs){
  FILE *f=fopen(filename, "w+t");
  int i,j;
  fprintf(f, "digraph brain {\n");

  for(i=0;i<numberOfSensors;i++) // sensors
    fprintf(f,"	%i [shape=invtriangle,style=filled,color=red];\n",i);
  for(i=numberOfSensors;i<brainSize-numberOfOutputs;i++) // hidden states
    fprintf(f,"	%i [shape=invtriangle,style=filled,color=orange];\n",i);
  for(i=brainSize-numberOfOutputs;i<brainSize;i++) // outputs
    fprintf(f,"	%i [shape=circle,style=filled,color=green];\n",i);

  for(i=0;i<hmmus.size();i++) // gates
    fprintf(f,"\tnode [label=\"%s\", shape=invhouse, color=gray] g%i%i%i;\n",hmmus[i]->determineGateType().first.c_str(),hmmus[i]->ins[0],hmmus[i]->ins[1],hmmus[i]->outs[0]);

  for(i=0;i<hmmus.size();i++){
    //	fprintf(f,"	{\n");
    string inputColor[2]={"black","black"}; 
    for(j=0;j<hmmus[i]->ins.size();j++){
      if (hmmus[i]->determineGateType().second != -1)
	inputColor[hmmus[i]->determineGateType().second]="red";
      fprintf(f,"\t{edge[color=%s];	%i	->	g%i%i%i;}\n", inputColor[j].c_str(),hmmus[i]->ins[j],hmmus[i]->ins[0],hmmus[i]->ins[1],hmmus[i]->outs[0]);
    } 
    for(j=0;j<hmmus[i]->outs.size();j++)
      fprintf(f,"	g%i%i%i 	->	%i;\n",hmmus[i]->ins[0],hmmus[i]->ins[1],hmmus[i]->outs[0],hmmus[i]->outs[j]);
    
    fprintf(f,"\t{ rank=same; ");
    for(j=0;j<hmmus[i]->ins.size();j++)
      fprintf(f," %i; ", hmmus[i]->ins[j]);
    fprintf(f, "}\n");
    //	fprintf(f,"	}\n");
  }
  fprintf(f,"}\n");
  fclose(f);
}
// <- @AT

void tAgent::saveLogicTable(FILE *f){
	// Explore all input-output mappings and save
	int i,j;
	//fprintf(f,"N0,N1,N2,N3,N4,N5,N6,N7,N9,N10,N11,N12,N13,N14,N15,,n0,n1,n2,n3,n4,n5,n6,n7,n8,n9,n10,n11,n12,n13,n14,n15\n");

	for (i=0;i<brainSize;i++)
	  if (alwaysZero.find(i)==alwaysZero.end())
	    fprintf(f,"N%i,",i);

	for (i=0;i<brainSize;i++)
	  fprintf(f,",n%i",i);
	fprintf(f,"\n");
	
	//for(i=0;i<65536;i++){
	int zeros=alwaysZero.size();
	//cout <<"\t\t\t\t\t\t\t\t\t"<< zeros << '\n';
	int z=0;
	for (i=0; i<pow(2,brainSize-zeros);i++){
	        z=0;
		for(j=0;j<brainSize-zeros;j++){
		        if (alwaysZero.find(j)!=alwaysZero.end()){
			  states[j]=0;
			  z++;
			}
			fprintf(f,"%i,",(i>>j)&1);
			states[j+z]=(i>>j)&1;
		}
		for (int b=0; b<brainUpdates; b++)
		  updateStates();
		for(j=0;j<brainSize;j++){
			fprintf(f,",%i",states[j]);
		}
		fprintf(f,"\n");
	}
}

// @AT ->

void tAgent::getLogicTable(vector<vector<int> > &t0, vector<vector<int> > &t1){
  for (int i=0; i<pow(2, brainSize); i++){
    vector<int> r0;
    for (int j=0; j<brainSize; j++){
      r0.push_back(int((i>>j)&1));
      states[j]=int((i>>j)&1);
    }
    t0.push_back(r0);
    for (int u=0; u<brainUpdates; u++)
      updateStates();
    vector<int> r1;
    for (int j=0; j<brainSize; j++)
      r1.push_back(int(states[j]&1));
    t1.push_back(r1);
  } 
}

void tAgent::saveStatesMap(string fn){
  ofstream file(fn.c_str());
  file<<"ioi,tone,obTone,obDelay,obPos,step,pos,state,visits\n";
  for (map<pair<string,int>,int>::iterator it=visStates.begin(); it!=visStates.end(); it++)
    file<<it->first.first<<it->first.second<<","<<it->second<<'\n';
}

void tAgent::saveFSM(string fn){
  ofstream file(fn.c_str());
  file<<"digraph brain{\n";
  for (map<pair<long,long>, set<int> >::iterator it=fsm.begin(); it!=fsm.end(); it++){
    set<int> labels=it->second;
    ostringstream labOss;
    for (set<int>::iterator l=labels.begin(); l!=labels.end(); l++)
      labOss<<(*l)<<",";
    string labStr=labOss.str();
    labStr=labStr.substr(0,labStr.size()-1);

    long state1=it->first.first, state2=it->first.second;
    if (fsmColors.find(state2)!=fsmColors.end()){
      string col=(state2%2) ? "red" : "blue";
      string shape=(state2%2) ? "triangle" : "invtriangle";
      file<<"\t"<<state2<<"[shape="<<shape<<",color="<<col<<",style=filled];\n";
    }
    file<<"\t"<<state1<<"->"<<state2<<"\t[label=\""<<labStr<<"\"];\n";
  }
  file<<"}\n";
  /*  ofstream file(fn.c_str());
  file<<"digraph brain{\n";
  for (map<pair<int,int>, pair<int,int> >::iterator it=fsm.begin(); it!=fsm.end(); it++){
    if (it->second.second==1)
      file<<"\t"<<it->second.first<<"[color=blue,style=filled];\n";
    file<<"\t"<<it->first.first<<"->"<<it->second.first<<"\t[label=\""<<it->first.second<<"\"];\n";
  }

  file<<"\tedge[color=blue, dir=none, penwidth=2, headport=n, tailport=n]\n";
  for (map<int, int>::iterator it=orig.begin(); it!=orig.end(); it++)
    file<<"\t"<<it->first<<"->"<<it->second<<";\n";

  file<<"\tedge[color=red, dir=none, penwidth=2, headport=s, tailport=s]\n";
  for (map<int, int>::iterator it=ott.begin(); it!=ott.end(); it++)
    file<<"\t"<<it->first<<"->"<<it->second<<";\n";

  file<<"\tedge[color=green, dir=none, penwidth=2, headport=e, tailport=e]\n";
  for (map<int, int>::iterator it=lo.begin(); it!=lo.end(); it++)
    file<<"\t"<<it->first<<"->"<<it->second<<";\n";

    file<<"}\n";
*/
}

vector<int> tAgent::attention(){
  vector<int> transitions;
  for (map<pair<long,long>, set<int> >::iterator it=fsm.begin(); it!=fsm.end(); it++){
    transitions.push_back(it->second.size());
  }
  return transitions;
  /*
  int cnt=0;
  for (map<pair<int, int>, pair<int, int> >::iterator it=fsm.begin(); it!=fsm.end(); it++){
    int input=it->first.second;
    pair<int,int> key_prime=make_pair(it->first.first, 1-input);
    if (fsm.find(key_prime)!=fsm.end() && fsm[key_prime].first==it->second.first){
      cnt++;
      //cout << "State:\t" << key_prime.first << endl;
    }
  }
  return cnt/2;
  */
}
// <- @AT

void tAgent::saveGenome(FILE *f){
	int i;
	for(i=0;i<genome.size();i++)
		fprintf(f,"%i	",genome[i]);
	fprintf(f,"\n");
}

void tAgent::findActiveNeurons(){//const vector<vector<int> > &t0, const vector<vector<int> > &t1){
  vector<vector<int> > t0, t1;
  getLogicTable(t0,t1);

  vector<int> t1_ctrl(brainSize), inState, outState, inStateAct, outStateAct;
  set<int> actNodes, inertNodes;//, alwaysZero, alwaysOne;//, inputNodes, outputNodes;

  //take out nodes that are always zero/one after update for any brain state. 
  //INPUT NODES are not included in this test because they may always be 0 after a brain update, but environment still writes on them so they may still affect the output state of the brain. 
  for (int i=inputNodes.size(); i<brainSize; i++){
    t1_ctrl.at(i)=0;
    for (int j=0; j<pow(2, brainSize); j++)
      t1_ctrl.at(i)+=t1.at(j).at(i);
    
    if (t1_ctrl.at(i)==0){
      cout << "\t\tnode " << i << " is always 0\n";
      alwaysZero.insert(i);
      inertNodes.insert(i);
    }
    /*else if(t1_ctrl.at(i)==pow(2, brainSize)){
      cout << "\t\tnode " << i << " is always 1\n";
      alwaysOne.insert(i);
      inertNodes.insert(i);
      } */     
    else
      actNodes.insert(i);
  }
  
  actNodes.insert(inputNodes.begin(), inputNodes.end());

  //take out redundant nodes, i.e. if neuron state (0 or 1) NEVER changes the update state, i.e. in every other possible states 2^(size-1)  
  //calculate state for all active neurons
  bool skip;
  for (int i=0; i<t0.size(); i++){
    skip=false;
    for (set<int>::iterator node=alwaysZero.begin(); node!=alwaysZero.end(); node++)
      if (t0.at(i).at(*node)==1)
	skip=true;
    if (skip)
      continue;
    
    setStates(t0.at(i));
    inState.push_back(calcState(actNodes));

    setStates(t1.at(i));
    outState.push_back(calcState(actNodes));
  }

  //Check if a neuron state does not matter in all states (this includes all input neurons, and hidden and output neurons that are written to)
  int i=0;
  int size=actNodes.size();
  for (set<int>::iterator node=actNodes.begin(); node!=actNodes.end(); node++){
    int inc=pow(2,i);//2^0=1
    bool isInert=true;
    for (int j=0; j<=pow(2, size)-2*inc; j+=2*inc){//2^(size - (i+1))
      for (int k=0; k<inc; k++){//2^i
	if (outState.at(j+k)!=outState.at(j+k+inc) ){
	  isInert=false;
	  break;
	}
      }
      if (!isInert)
	break;
    }
    if (isInert){
      cout << "\t\tnode " << (*node) << " is inert\n";
      inertNodes.insert(*node);
    }
    i++;
  }

  //take out inert nodes, they can be some of INPUT NODES as well (can they?)
  for (set<int>::iterator inert=inertNodes.begin(); inert!=inertNodes.end(); inert++)
    actNodes.erase(*inert);

  actNodes.insert(outputNodes.begin(), outputNodes.end());

  for (set<int>::iterator in=inputNodes.begin(); in!=inputNodes.end(); in++)
    actNodes.erase(*in);
  //copy(actNodes.begin(), actNodes.end(), ostream_iterator<int>(cout,","));
  //cout << endl;
  cout << "\tactive nodes set size: "<< actNodes.size() << "\tbrain state size: " << pow(2, actNodes.size())<<'\n';

  actNeurons=actNodes;
}
