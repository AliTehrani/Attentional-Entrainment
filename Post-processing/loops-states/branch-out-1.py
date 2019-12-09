import pandas as pd
import sys
import ast
import numpy as np

ad=sys.argv[1]
loopFn=sys.argv[2]

loops = pd.read_csv(loopFn, converters={'count':ast.literal_eval},index_col=0)

gen=int(sys.argv[3])
repInit=int(sys.argv[4])
repFin=int(sys.argv[5])
outFn=sys.argv[6]

reps=range(repInit, repFin+1)
iois=range(10,26)
index=pd.MultiIndex.from_product([reps,iois],names=['rep','ioi'])
loops_new=pd.DataFrame(index=index,columns=['loop'])
for i,rep in enumerate(reps):
    for ioi in iois:
        loops_new.loc[rep,ioi]=[loops['count'].loc[ioi][i]]

del loops

dfs=list()
for rep in range(1,51):
    print (rep)
    l=loops_new.loc[rep]
    iois=l[l.loop==l.index].loop.tolist()

    t=pd.read_csv(ad+"tests_"+str(rep)+"_2000.csv",index_col=[0,1,2,3,4])
    t=t[t.index.get_level_values('obDelay')==0]
    t.index=t.index.droplevel(3)

    t=t[t.index.get_level_values('ioi').isin(iois)]
    t['branchPoint']=np.NAN
    for (ioi,tone,ob,pos) in t.index:
        if (tone==ob):
            continue
        l1=[int(s) for s in t.loc[ioi,tone,tone,pos].vid.split(',')][ioi*pos+1:ioi*(pos+1)+1]
        l2=[int(s) for s in t.loc[ioi,tone,ob,pos].vid.split(',')][ioi*pos+1:ioi*(pos+1)+1]
        d=[i==j for (i,j) in zip(l1,l2)]
        branchOut=d.index(0) if 0 in d else -1
        #l.append(branchOut-ob)
        t['branchPoint'].loc[(ioi,tone,ob,pos)]=(branchOut%ioi)

    t.drop(['vid','states'], axis=1, inplace=True)
    tMean = t.mean(level=[0,1,2])
    tMean['rep']=rep
    tMean.set_index('rep', append=True, inplace=True)
    tMean=tMean.reorder_levels(['rep','ioi','tone','obTone'])
    dfs.append(tMean.dropna())

    del l,t,tMean

res=pd.concat(dfs)
res.to_csv(outFn)
