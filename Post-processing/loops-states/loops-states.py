#combine both
import numpy as np
import sys
import pandas as pd
import itertools

def findCycle(l, ioi):
    up = len(l)
    for p in range(1, up+1):
        cycle = True
        for i in range(0, len(l)-p):
            if (l[i]!=l[i+p]):
                cycle=False
                break
        if cycle:
            break
    #print (p)
    #print (cycle, end='\t')
    return p

def fillLoopsDf(ad,gen,patterns,reps):
    loops=pd.DataFrame(columns=['count'])

    dfs = list()
    for rep in reps:
        dfs.append( pd.read_csv(ad+"tests_"+str(rep)+"_"+str(gen)+".csv", index_col=[0,1,2,3,4]) )

    #for tone in tones:
    for (ioi,tone) in patterns:
        lTone=list()
        for rep in range(len(reps)):
            df = dfs[rep] #= pd.read_csv(ad+"tests_"+str(rep)+"_2000.csv", index_col=[0,1,2,3,4])
            #ioi=tone*2
            vid=df.loc[ioi,tone,tone,0,7].vid
            v=[int(s) for s in vid.split(',')]
            v=v[4*ioi:]
            lTone.append(findCycle(v, ioi))
        loops.loc[ioi]=[lTone]
        
    del dfs
    return loops

def svStats(ad,reps,gen):
    df=pd.read_csv(ad+"tests_"+str(reps[0])+"_"+str(gen)+".csv")
    df['pattern']=list(zip(df.ioi,df.tone))
    patterns=df.pattern.unique()
    del df

    vis=pd.DataFrame(columns=['total','totTrain','trainPerTrial','totEntrain','entrainPerTrial',
                              'shDecPerTrial','loDecPerTrial'],
                     index=reps)
    
    for rep in reps:
        df=pd.read_csv(ad+"tests_"+str(rep)+"_"+str(gen)+".csv")#,index_col=[0,1,2,3,4])
        df['decState']=df.apply(lambda row: int(row.vid.split(',')[(row.obPos+1)*row.ioi]), axis=1)
        df.set_index(['ioi','tone','obTone','obDelay','obPos'], inplace=True)
        print (rep, end=" ")
        sys.stdout.flush()
        vidTot=[int(s) for v in df.vid for s in v.split(',')]
        total=len(set(vidTot))
        dfTrain=df[df.index.get_level_values('obDelay')==0]
        vidTotTrain=[int(s) for v in dfTrain.vid for s in v.split(',')]

        totTrain=len(set(vidTotTrain))
        trainPerTrial=list()
        entrainPerTrial=list()
        vidTotEntrain=list()
        loDecPerTrial=list()
        shDecPerTrial=list()
        for pat in patterns:
            #print (pat)
            ioi,tone=pat
            vidTrainTr=[int(s) for v in dfTrain.loc[(ioi,tone)].vid for s in v.split(',')]
            trainPerTrial.append(len(set(vidTrainTr)))
            vidEntrain=[int(s) for s in dfTrain.loc[ioi,tone,tone,0,4].vid.split(',')[ioi*4:]]
            entrainPerTrial.append(len(set(vidEntrain)))
            vidTotEntrain=vidTotEntrain+list(set(vidEntrain))
            vidTotEntrain=list(set(vidTotEntrain))
            
            decs = list(set(dfTrain.loc[ioi,tone].decState.tolist()))
            #print (decs)
            
            loDec=list(filter(lambda n: n%2==1, decs))
            shDec=list(filter(lambda n: n%2==0, decs))
            
            loDecPerTrial.append(len(loDec))
            shDecPerTrial.append(len(shDec))
            #print (loDec)
            
        totEntrain=len(set(vidTotEntrain))

        vis.loc[rep]=[total,totTrain,str(trainPerTrial),totEntrain,str(entrainPerTrial),str(shDecPerTrial),str(loDecPerTrial)]
        del df
    
    print ("\n")
    return vis


# 1) loops
ad = sys.argv[1]
loopOutFn = sys.argv[2]
stOutFn = sys.argv[3]
gen = sys.argv[4]
repInit=int(sys.argv[5])
repFin=int(sys.argv[6])
reps = range(repInit, repFin+1)

temp=pd.read_csv(ad+"tests_"+str(repInit)+"_"+str(gen)+".csv",index_col=[0,1])
patterns=list(temp.index.unique())
del temp

print ("cycle detection ...")
loops = fillLoopsDf(ad, gen, patterns, reps)
print ("writing loop file:",loopOutFn)
loops.to_csv(loopOutFn)

# 2) state stats
print ("state statistics ...")
vis=svStats(ad,reps,gen)
print ("writing stat file:",stOutFn)
vis.to_csv(stOutFn)
