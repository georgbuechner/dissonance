# Dissonance 

## Premise 
Several parts of the brain are in dissonance and started attacking each-other
with strong potentials, aiming to destroy each-other. 

Each player starts with a nucleus. One nucleus has control over a few cells surrounding it.
By gathering different resources (iron, oxygen, 
potassium, ...) you can create Synapses to generate potential, advancing towards the 
enemies nucleus. When a certain amount of potential has reached the enemies nucleus, 
your enemy is destroyed. By activating cells you control, these cells can
neutralize incoming potential.

You randomly gain iron every few seconds (the more the game advances the less
iron you gain). Iron can be used to activate the process of gathering new
resources or to boost your oxygen production. Depending on your current oxygen
level, you gain more or less resources. Oxygen is also needed to create Synapses
or to activate sells for your defences. But be careful: the more oxygen you
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


### Game Details 

#### Resources 
| name        | group   | description   | production | effect |
|:-----------:|:-------:|---------------| ------ | ----- |
| FE (iron)   | Resources| metalloprotein | automatic income every 30sec (random factor) | used for gaining O2; when converted Hemoglobin, allows gaining new resources. |
| O2 (oxygen) | Resources/ Production | ---| increases by amount of FE2 (iron) invested. | used for production; increases gain of other resources. |
| K (potassium) | Attack | --- | If K production is activated by Hemoglobin, automatically gained depending on O2 level | used to generate EPSP |
| KCI (chloride) | Attack | --- | If KCI production is activated by Hemoglobin, automatically gained depending on O2 level | used to generate IPSP |
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
| name            | description   | steps |
|:---------------:|---------------| ------ |
| choose way | chose one neuron the potential passes for each available step; applicable to each synapse separately | 1, 2, 3 steps |
| swarm attack | potential waits till a certain amount is reached before advancing to the target; activate/ deactivate for each synapse separately | 5, 10, 15 potentials |
| chose target | allows a synapse to specify a target | 1, 3, all synapse can use this ability |

#### Questions
- how does voltage increase (suggestions: binding oxygen, destroying enemy
  position, occupying enemy neurons). Each player could also have the possibility
  to decide for the one or the other technique.
