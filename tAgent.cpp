/*
 *  tAgent.cpp
 *  HMMBrain
 *
 *  Created by Arend on 9/16/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "tAgent.h"

tAgent::tAgent(){
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

void tAgent::resetBrain(void){
	for(int i=0;i<brainSize;i++)
		states[i]=0;
#ifdef useANN
	ANN->resetBrain();
#endif
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
void tAgent::saveLOD(FILE *statsFile,FILE *genomeFile){
	if(ancestor!=NULL)
		ancestor->saveLOD(statsFile,genomeFile);
#ifdef useANN
	fprintf(genomeFile,"%i	",ID);
	fprintf(statsFile,"%i	%i	%i	%f	%i	%f	%i	%i\n",ID,born,(int)genome.size(),fitness,correct,incorrect);
	ANN->saveLOD(genomeFile);
#else	
	fprintf(statsFile,"%i,%f\n",born,fitness);
	//for (set<int>::iterator it=learned.begin(); it!=learned.end(); it++)
	//fprintf(statsFile,"%i,",(*it));
	//fprintf(statsFile,"\b\",\"");
	//for (set<int>::iterator it=notLearned.begin(); it!=notLearned.end(); it++)
	//fprintf(statsFile,"%i,",(*it));
	//fprintf(statsFile,"\b\"\n");
	if(!retired){
	  for(int i=0;i<genome.size();i++)
            fprintf(genomeFile,"%i  ",genome[i]);
	  fprintf(genomeFile,"\n");
	}
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
	fprintf(f,"0_t0,1_t0,2_t0,3_t0,4_t0,5_t0,6_t0,7_t0,8_t0,9_t0,10_t0,11_t0,12_t0,13_t0,14_t0,15_t0,,0_t1,1_t1,2_t1,3_t1,4_t1,5_t1,6_t1,7_t1,8_t1,9_t1,10_t1,11_t1,12_t1,13_t1,14_t1,15_t1\n");
	for(i=0;i<65536;i++){
		for(j=0;j<16;j++){
			fprintf(f,"%i,",(i>>j)&1);
			states[j]=(i>>j)&1;
		}
		updateStates();
		for(j=0;j<16;j++){
			fprintf(f,",%i",states[j]);
		}
		fprintf(f,"\n");
	}
}

void tAgent::saveGenome(FILE *f){
	int i;
	for(i=0;i<genome.size();i++)
		fprintf(f,"%i	",genome[i]);
	fprintf(f,"\n");
}




