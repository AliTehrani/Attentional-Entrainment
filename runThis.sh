#!/bin/bash --login
#PBS -l nodes=1:ppn=1,walltime=01:00:00,mem=2000mb
#PBS -j oe
#PBS -t 1-20

#cd ${PBS_O_WORKDIR}
cd ~/markov/entrainment/learn-rhythmic/longer/

./abeeda $watch $tone $i 0.2 $d/LOD_${PBS_ARRAYID}.csv $d/agent.genome $d/best_brain_${PBS_ARRAYID} $d/genome_${PBS_ARRAYID}.csv $d/phenotype_${PBS_ARRAYID}.dat $d/video_${PBS_ARRAYID}.csv 5000
