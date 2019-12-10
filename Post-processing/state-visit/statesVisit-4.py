import pandas as pd
import numpy as np
import sys
from ast import literal_eval

inDir = sys.argv[1]
visFn = sys.argv[2]
seqFn = sys.argv[3]
gen = sys.argv[4]
repInit = sys.argv[5]
repFin = sys.argv[6]

reps=range(int(repInit), int(repFin) + 1)

####### find patterns
fn = inDir+"tests_"+str(reps[0])+"_"+str(gen)+".csv"
temp = pd.read_csv(fn,index_col=[0,1,2,3])
patterns = list(temp.index.unique())

del temp

####### create df, and indices
ind=list([])
for rep in reps:
    for (ioi,tone,obTone,obDelay) in patterns:
        ind.append((rep,ioi,tone,obTone,obDelay))

#for rep in reps:
#    for (ioi,tone) in patterns:
#        for ob in range(1,ioi):
#            delayRng=list(range(tone-ioi+1,0))+list(range(1, ioi-tone))
#            for delay in delayRng:
#                tup=(rep,ioi,tone,ob,delay)
#                ind.append(tup)

levels=pd.MultiIndex.from_tuples(ind,names=['rep','ioi','tone','ob','delay'])
contexts=pd.DataFrame(index=levels,columns=['trials','fa','ig','unseen'],dtype=int)
contexts.trials=[0]*len(contexts)
contexts.fa=[0]*len(contexts)
contexts.ig=[0]*len(contexts)
contexts.unseen=[0]*len(contexts)

sequences = pd.DataFrame(columns=['rep','ioi','tone','ob','delay','pos','depth','similarity'])

####### read tests files and fill contexts and sequences
for rep in reps:
    print (rep)
    t = pd.read_csv(inDir+"tests_"+str(rep)+"_"+str(gen)+".csv", index_col=[0,1,2,3,4])
    t.sort_index(inplace=True)

    #training states: states visited for on-time trials
    dfTrain = t[t.index.get_level_values('obDelay')==0].copy(deep=True)
    trStates=set([int(s) for v in dfTrain.vid for s in v.split(',')])
    del dfTrain

    ####### go through all trials
    for (ioi,tone,ob,delay,pos) in t.index:
        
        ####### exclude unwanted trials
        ####### delay should not be 0 (early or late only), not too early/late (|delay|<tone)
        if delay==0 or delay<tone-ioi+1 or delay>ioi-tone-1:
            continue
        ## oddball ends outside interval
        if delay+ob<=0:
            continue
        
        contexts.loc[rep,ioi,tone,ob,delay].trials += 1

        ####### misjudgments
        dec = t.loc[ioi,tone,ob,delay,pos].obAction

        ## delayed: decision==longer, advanced: decision==shorter
        if (delay<0 and dec==1) or (delay>0 and dec==0):# or (delay>0 and ob>tone) or (delay<0 and ob<tone):
            continue
        ## oddball that is delayed and shorter, or advanced and longer (otherwise it is not a misjudgment)
        if (delay>0 and ob>tone) or (delay<0 and ob<tone):
            continue
        ## exclude cases that can't technically be compared to ontime cases
        if (delay>0 and ob+delay<tone) or (delay<0 and ob+delay>tone):
            continue

        ####### misjudgment occured
        contexts.loc[rep,ioi,tone,ob,delay].fa += 1

        ## the entire state sequence of the trial
        s1 = [int(s) for s in t.loc[ioi,tone,ob,delay,pos].vid.split(',')] #it should be states rather than vid but in tests_* files header, 'states' and 'vid' are written the other way around (incorrectly)

        ## the portion regarding misjudgment. +1 in first index excludes state=0 at the very beginning, for early oddballs we are interested in portion including the early oddball
        lead = delay if delay<0 else 0
        sInt1 = s1[pos*ioi + 1 + lead : (pos+1)*ioi + 1]
        
        ####### misjudgment from unseen (new) states
        if not (sInt1[-1] in trStates):
            contexts.loc[rep,ioi,tone,ob,delay].unseen+=1

        ####### is it ignore onset? compare (mis)judgment state with judgment states in which ob_out_of_time+delay==ob_ontime
        ig = 0
        positions=list()
        for p in range(4,8):
            s2 = [int(s) for s in t.loc[ioi,tone,delay+ob,0,p].vid.split(',') ]
            sInt2 = s2[p*ioi + 1 + lead : (p+1)*ioi + 1]
            
            ## judgment states of out-of-time and on-time are the same
            if sInt1[-1]==sInt2[-1]:
                positions.append(p)
                ig=1
                #break # if they are the same in one position only that is sufficient, also break out of the for loop so that we compare correct sequence of states (proabably doesn't make a difference but must be here) UPDATE: it does make a difference
        
        depths=list()
        sims=list()
        if (ig==1):
            for p in positions:
                s2 = [int(s) for s in t.loc[ioi,tone,delay+ob,0,p].vid.split(',') ]
                sInt2 = s2[p*ioi + 1 + lead : (p+1)*ioi + 1]
                eq = [int(i==j) for (i,j) in zip(sInt1, sInt2) ]
                ## for depth: find the first 0 in reverse equality sequence
                depth = eq[::-1].index(0) if 0 in eq else len(eq)
                if (depth==0):
                    print (sInt1)
                    print (sInt2)
                    print (eq)

                ## similarity is just how many states are exactly the same
                similarity = np.sum(eq)

                depths.append(depth)
                sims.append(similarity)

            ## only include trials with ignore onset in sequence dataframe, push it to the end of df
            sequences.loc[len(sequences)] = [rep,ioi,tone,ob,delay,pos,np.mean(depths),np.mean(sims)]
        
        contexts.loc[rep,ioi,tone,ob,delay].ig += ig # ig is either 0 or 1, but in different traverses of the index loop we count instances of ignore-onset in different pos (not p btw)


contexts.to_csv(visFn)
sequences.to_csv(seqFn)

