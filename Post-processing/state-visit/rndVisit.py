import pandas as pd
import numpy as np
import sys

ad=sys.argv[1]
outFn=sys.argv[2]
gen=sys.argv[3]
reps=range(int(sys.argv[4]), int(sys.argv[5]) + 1)

rnd=pd.DataFrame(index=reps,columns=['fsmTot','fsmLo','fsmSh','decLo','decSh'])

for rep in reps:
    print (rep,'\t',)
    t=pd.read_csv(ad+"tests_"+str(rep)+"_"+str(gen)+".csv")
    t0 = t[t.obDelay==0].copy(deep=True)

    t0['decState']=t0.apply(lambda row: int(row.vid.split(',')[(row.obPos+1)*row.ioi]) , axis=1)
    
    st=[int(s) for v in t0.vid.tolist() for s in v.split(',')]
    #stTot=[int(s) for v in t.vid.tolist() for s in v.split(',')]
    fsm=set(st)
    #fsmTot=set(stTot)
    #print (rep,'\t',len(dec),'\t',len(decTot))
    fsmLo=[d for d in fsm if d%2==1]
    fsmSh=[d for d in fsm if d%2==0 and d!=0]

    dec=t0.decState.unique()
    decLo=[d for d in dec if d%2==1]
    decSh=[d for d in dec if d%2==0 and d!=0]

    rnd.loc[rep]=[len(fsm),len(fsmLo),len(fsmSh),len(decLo),len(decSh)]

rnd.to_csv(outFn)
