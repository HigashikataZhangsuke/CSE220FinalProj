# CSE220FinalProj
FinalProj Impl of CSE220 2024Fall

Contains the Impl of Paper Access Map Pattern Matching for High Performance Data Cache Prefetch from ICS 09'. 

# Mainly modified parts:
1. The prefetechers of L2. According to the original paper, the l2 preftecher.c in scarab will be modified to record the results of the settings. As described in the paper, we use the same settings (8kb for each hot zone).

2.Dcache stage part, to dynamically access data

3.Param defs

# V1.0 
Finished modify prefetcher code. The next step is to check the dcache stage, as well as make the interface/connections.

# V2.0
Finished All modifications by Chuanhan. 
