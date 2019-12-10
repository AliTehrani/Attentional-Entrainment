import pandas as pd
import numpy as np
from scipy import stats
from scipy.optimize import curve_fit
from math import *
import itertools
import os.path
import sys

def readResponses(ad,reps,gen):
    repsInd=[reps.index(rep)+1 for rep in reps]
    
    fn=ad+"tests_"+str(reps[0])+"_"+str(gen)+".csv"
    if (os.path.exists(fn)):
        df=pd.read_csv(fn)
    else:
        print (fn,"does not exist")
        return
    df["pattern"]=list(zip(df.ioi,df.tone))
    patterns=df.pattern.unique()
    tup=list()
    for pat in patterns:
        ioi,tone=pat
        for delay in range(tone-ioi+1,ioi-tone):
            for rep in repsInd:
                tup.append((pat,delay,rep))

    levels=pd.MultiIndex.from_tuples(tup,names=['pattern','delay','rep'])
    res=pd.DataFrame(index=levels,columns=['obTones','response'])
    for ind,rep in enumerate(reps):
        print(rep, end=' ')
        sys.stdout.flush()
        fn=ad+"tests_"+str(rep)+"_"+str(gen)+".csv"
        
        if (not os.path.exists(fn)):
            print (fn+" does not exist")
            continue
        df=pd.read_csv(fn)
        #df['obAction']=1-df.obAction
        for pat in patterns:
            ioi,tone=pat
            dfp=df[(df.ioi==ioi) & (df.tone==tone)].reset_index(drop=True)
            for delay in range(tone-ioi+1,ioi-tone):#dfp.obDelay.unique():
                dfd=dfp[dfp.obDelay==delay].reset_index(drop=True)
                responses=np.mean([dfd[dfd.obPos==obPos].obAction.tolist() for obPos in [4,5,6,7]],axis=0)
                obTones=np.array(dfd[dfd.obPos==4].obTone.tolist())
                
                res.loc[pat,delay,ind+1]=[obTones,responses]
                
                #print (res.loc[pat,delay,ind+1])

    return res.sort_index()

def sigmoid(x,k,x0):
    return 1.0/(1+np.exp(-k*(x-x0)))

#new
def psyCurveRep(res):
    levels=res.index.droplevel(level=2).drop_duplicates()
    psyRep=pd.DataFrame(index=levels,columns=['PSE','PseStd','JND','JndStd','PseJndCov','relJND','DDF','CE'],dtype=float)
    for (pat,delay) in psyRep.index:
        ioi,tone=pat
        if (len(res.loc[pat].loc[delay].obTones.tolist()[0])<2):
            continue
        obTones=list(res.loc[pat].loc[delay,1].obTones)#[0]
        responses=list(np.mean(res.loc[pat].loc[delay].response.tolist(),axis=0))
        rSigmas=list(stats.sem(res.loc[pat].loc[delay].response.tolist(),axis=0))
        tre=list(zip(obTones,responses,rSigmas))
        #tre=list(filter(lambda arg: arg[2] != 0, tre))

        # remove zeros from the beginning
        for init, (ob,resp,sig) in enumerate(tre):
            if resp!=0:
                break
        # remove ones from the end
        for fin, (ob,resp,sig) in enumerate(tre[::-1]):
            if resp!=1:
                break

        fin=len(tre)-fin
        tre=tre[init:fin]

        obTones,responses,rSigmas=[list(t) for t in zip(*tre)]

        # correct ones and zeros in the middle
        errMin = np.min(list(filter(lambda el: el>0, rSigmas)))
        for ind, (ob,resp,sig) in enumerate(tre):
            if (resp==0):
                responses[ind]=0.01
                rSigmas[ind]=errMin
                print ("response correction in",pat,delay,"at",ob,"\tnew response, sigma:", 0.01, errMin)
            if (resp==1):
                responses[ind]=0.99
                rSigmas[ind]=errMin
                print ("response correction in",pat,delay,"at",ob,"\tnew response, sigma:", 0.99, errMin)

        if (len(tre)<2):
            print ("less than 2 datapoints for",pat,delay)
            if tre[0][0]==tone:
                print ("\tperformace is 100% in",pat,delay)
            continue

        if (responses.count(0)!=0 or responses.count(1)!=0):
            print ("responses contain 0 or 1")

        try:
            x0_0=obTones[min(range(len(responses)), key=lambda i: abs(responses[i]-0.5))]
            popt,pcov=curve_fit(sigmoid,obTones,responses,sigma=rSigmas,absolute_sigma=True,p0=(1,x0_0),maxfev=10000)
            k,x0=popt
            kStd,pseStd=np.sqrt(np.diag(pcov))
            pseJndCov=pcov[0,1]
            jnd=log(3.0)/k
            jndStd=log(3.0)/(k*k)*kStd
            relJND=jnd/tone
            ddf=tone/x0
            ce=x0-tone
            psyRep.loc[pat,delay]=[x0,pseStd,jnd,jndStd,pseJndCov,relJND,ddf,ce]
        except RuntimeError:
            print (pat,delay,"curve_fit failed")
            continue
    return psyRep

ad,fnRes,fnPsy,gen=sys.argv[1],sys.argv[2],sys.argv[3],sys.argv[4]
reps=range(int(sys.argv[5]), int(sys.argv[6])+1)

print ("Reading responses from tests files ...")
res=readResponses(ad,reps,gen)
print ("\nWriting responses to",fnRes)
res.to_csv(fnRes)

print ("Generating psychometric curves from responses ...")
psy=psyCurveRep(res.dropna())
print ("\nWriting psychometric data to",fnPsy)
psy.to_csv(fnPsy)
#print(res.head(20))
