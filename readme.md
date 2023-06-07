# QoS-OLSRD 2.0
<b>Major Features:</b>

<b>QoS Metric(in olsr.c): </b> 
* Considers stable links between nodes only
* Current QoS calculated based on inverse tan of average link cost and number of neighbors 
* Weighted average of previous QoS and Current QoS 

<b> Cluster head selection (in mpr.c): </b>
* Selects cluster head being the neighbor with max QoS
* Executed periodically by each node in the network 
* Change head to new head only if the improvement in QoS is more than 10%

<b> Find Cluster head (in mpr.c): </b>
* Selects the max QoS existing cluster head between the neighbors
* Executed when a node loses its cluster head or joins the network in the middle of the selection period

<b> MPR Selection (in mpr.c): </b>
* Selects MPRs for two and three hop away cluster heads 
* Executed periodically at cluster heads

<b> Makefile.linux was updated to install the NL80211 dependencies </b>
<b> The executable binary to be used is the qos-olsrd</b>
