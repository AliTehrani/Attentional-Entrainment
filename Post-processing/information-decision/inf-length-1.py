import pandas as pd
import numpy as np
from math import *
import sys

def entropy(l):
    h=0
    N=len(l)
    for i in set(l):
        pi = l.count(i)/N
        if (pi!=0 and pi!=1):
            h += -pi*log(pi,2)
    return(h)


def condEnt(y,x):#h(y|x)
    xy=list(zip(x,y))
    h=0
    for ij in set(xy):
        pij = xy.count(ij)/len(xy)
        pj = x.count(ij[0])/len(x)
        h += -pij*log(pij/pj, 2)
    return h

def information(x,y):
    if (len(x)!=len(y)):
        print ("array lengths don't match")
        return

    xy=list(zip(x,y))
    N=len(xy)
    I=0
    for ij in set(xy):
        pxy=xy.count(ij)/N
        px=x.count(ij[0])/N
        py=y.count(ij[1])/N
        I+=pxy*log(pxy/(px*py) ,2)
        
    return I

ad=sys.argv[1]
outFn = sys.argv[2]
gen=int(sys.argv[3])
repInit = int(sys.argv[4])
repFin = int(sys.argv[5])

ind=[]
reps=range(repInit,repFin+1)
temp = pd.read_csv(ad+"tests_"+str(repInit)+"_"+str(gen)+".csv")
temp['pattern'] = list(zip(temp.ioi,temp.tone))
patterns=temp.pattern.unique()
print (patterns)
#tones=range(5,21)
#oddballs=['all','shorter','longer']
#for rep in reps:
#    for tone in tones:
#        for endPoint in range(1,tone*2):
#            for timeStep in range(-tone+1,tone*2):
#                ind.append((rep,tone,endPoint,timeStep))
            

#index=pd.MultiIndex.from_product([reps, tones, oddballs],names=['rep','tone'])
index=pd.MultiIndex.from_product([reps, patterns],names=['rep','pattern'])
inf = pd.DataFrame(index=index, columns=['H(length)','H(onset)','H(endPoint)','H(decision)','I(dec:len)','I(dec:ons)','I(dec:end)'])

for rep in reps:
    print (rep)
    tst = pd.read_csv(ad+"tests_"+str(rep)+"_"+str(gen)+".csv")
    tst['decState'] = tst.apply(lambda row: row.vid.split(',')[(row.obPos+1)*row.ioi], axis=1)
    tst['endPoint'] = tst.obTone+tst.obDelay
    tst.drop(['vid','states'], inplace=True, axis=1)

    for pat in patterns:
        ioi,tone=pat

        tst_ioi = tst[(tst.ioi==ioi) & (tst.tone==tone)]
        #tst_sh=tst_ioi[(tst_ioi.endPoint>0) & (tst_ioi.endPoint<tone) & (tst_ioi.obDelay<=0)]
        #tst_lo=tst_ioi[(tst_ioi.endPoint>0) & (tst_ioi.endPoint<tone) & (tst_ioi.obDelay<=0)]

        tst_all=tst_ioi[tst_ioi.endPoint>0] #all onsets, all oddball durations as long as part of oddball occur during interval
        tst_sh=tst_ioi[(tst_ioi.endPoint>0) & (tst_ioi.obTone<tst_ioi.tone)]

        length=tst_all.obTone.tolist()#.decState.unique()
        onset=tst_all.obDelay.tolist()
        endpoints=tst_all.endPoint.tolist()
        decision=tst_all.decState.tolist()

        hLen = entropy(length)
        hOns = entropy(onset)
        hEnd = entropy(endpoints)
        hDec = entropy(decision)

        #I_dec_len = hDec - condEnt(decision,length)
        #I_dec_ons = hDec - condEnt(decision,onset)
        #I_dec_end = hDec - condEnt(decision,endpoints)

        I_dec_len = information(decision, length)
        I_dec_ons = information(decision,onset)
        I_dec_end = information(decision,endpoints)

        inf.loc[rep, pat] = [hLen, hOns, hEnd, hDec, I_dec_len, I_dec_ons, I_dec_end]#[I,hY,hYX,N,x.count(0)/N,x.count(1)/N,len(set(y))]
        
        del tst_ioi

inf.to_csv(outFn)
