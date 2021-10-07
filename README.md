# Dissonance 

## Premise 
Several parts of the brain are in dissonance and started attacking each-other
with strong potentials, aiming to destroy the others nucleus. 

Each player starts with a nucleus. One nucleus has control over a few cells surrounding it.
By gathering different resources (iron, oxygen, 
potassium, ...) you can create Synapses to generate potential, advancing towards the 
enemies nucleus. When a certain amount of potential has reached the enemies nucleus, 
your enemy is destroyed. By activating cells you control, these cells can
neutralize incoming potential.

You randomly gain iron every few seconds. Iron can be used to activate the process of 
gathering new resources or to boost your oxygen production. Depending on your current oxygen
level, you gain more or less resources. Oxygen is also needed to create Synapses
or to activate neurons for your defences. But be careful: the more oxygen you
spend on building advanced neurons (Synapses/ activated neurons) the less
resources you gain per seconds!

Once you started gaining dopamine and serotonin, you can develop advanced
technologies, allowing you f.e. to target specific enemy neurons and hence destroy
enemy synapses or activated neurons. Other technologies or advanced
use of potentials may allow you to:
- ...choose the path of you potential, to avoid passing to many enemy cells.
- ...let you potentials attack in swarms
- ...extend the range of neurons you control
- ...block enemy neurons using IPSP (inhibitory postsynaptic potential)
instead of EPSP (excitatory postsynaptic potential)
- ...weaken incoming enemy potential

In general technologies cost either dopamine or serotonin. In addition the
current voltage level determines how fast technologies are researched. 

## Installation 

### Requirements 
- gcc/11.1.0 (availible in most package manegers)  
- conan (f.e. `pip install conan`)

You might also need to install some media libraries to play audio which is not in `.wav` 
format. For this please refere the aubio page.  - [aubio](https://aubio.org/manual/latest/installing.html#external-libraries) 

### Installation

Clone project: 
```
git clone git@github.com:georgbuechner/fast_bsg.git
```

cd into the project
```
cd fast_bsg
```

And then you can simply install by running:
```
make install
```

Installs `dissonance` system wide and create `.dissonance` in the home folder to
store settings and analysed musical data.

### Usage

To play, simply run `dissonance` in you terminal. 

You can run `dissonance` with the `-r` option, to create the map based on your
current terminal size. Doing this will however change the game experience and
two identical songs will no longer produce an identical map and experience. 


### Game Details 

#### Resources 
- ideal distribute oxygen to resources to increase resource-gain.

| name        | group   | description   | production | effect |
|:-----------:|:-------:|---------------| ------ | ----- |
| FE (iron)   | Resources| metalloprotein | automatic income every 30sec (random factor) | used for gaining O2; when converted Hemoglobin, allows gaining new resources. |
| O2 (oxygen) | Resources/ Production | ---| increases by amount of FE2 (iron) invested. | used for production; increases gain of other resources. |
| K (potassium) y | Attack | --- | If K production is activated by Hemoglobin, automatically gained depending on O2 level | used to generate EPSP |
| KCI (chloride) y | Attack | --- | If KCI production is activated by Hemoglobin, automatically gained depending on O2 level | used to generate IPSP |
| Glu (glutamate) | Defence | --- | If Glu production is activated by Hemoglobin, automatically gained depending on O2 level | used to activate neuron |
| DA (dopamine) | Tech | --- | If Da production is activated by Hemoglobin, automatically gained depending on O2 level | used to gain advanced technologies |
| 5-HT (serotonin) | Tech | --- | If 5-HT production is activated by Hemoglobin, automatically gained depending on O2 level | used to gain advanced technologies |
-------------------------------------

#### Neurons and Transmitters
| name            | group   | description   | costs   | effect |
|:---------------:|:-------:|---------------| ------- | ------ |
| Activated Neuron | Defence/ Neuron | Activates a cell, creating a defensive layer. | O2, Glu | reduces activity of incomming EPSP or IPSP. |
| Synapse         | Attack/ Neuron  | Creates potential (EPSP, IPSP) | O2, K/ KCI | Creates either EPSP or IPSP, determines direction of potential. |
| EPSP            | Attack/ potential | Potential traveling to enemy neurons | K | weakens buildings (Nucleus, Synapses or NMDA-receptors) greatly, lowers potential of the first enemy potential met. |
| IPSP            | Attack/ potential | Potential traveling to enemy neurons | K, KCI | weakens buildings (Nucleus, Synapses or NMDA-receptors) slightly, knocks first enemy potential met. |


#### Tech
| name            | type | description   | steps |
|:---------------:|------|--------| ------ |
| choose way | attack | chose one neuron the potential passes for each available step; applicable to each synapse separately. | 1, 2, 3 steps |
| swarm attack | attack | potential waits till a certain amount is reached before advancing to the target; activate/ deactivate for each synapse separately | 5, 10, 15 potentials |
| chose target | attack | allows a synapse to specify a target | 1 (ipsp), 2 (ipsp+epsp)|
| total oxygen | resources | increases maximum oxygen | 120, 140, 160 |
| lower curve | resources | lower curve effekt when gaining resources | 3, 2, 1 |
| stored voltage | potential-upgrade | increases voltage in one potential | +1, +2, +3 |
| speed | potential-upgrade | increases speed of ipsp and epsp | +20ms, +40ms, +60ms |
| ipsp duration | potential-upgrade | increases duration of ipsp in enemy neuron | 100ms, 200ms 


#### Questions
- how does voltage increase (suggestions: binding oxygen, destroying enemy
  position, occupying enemy neurons). Each player could also have the possibility
  to decide for the one or the other technique.
