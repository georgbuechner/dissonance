# Dissonance 
A command line and keyboard based strategy-game, where audio-input determines the
AI-strategy and lays the seed for the map-generation.

## Premise 
Several parts of the brain are in dissonance and started attacking each other
with strong potentials, aiming to destroy the others nucleus. 

Each player starts with a nucleus with control over a few cells surrounding it.
By gathering different resources (iron, oxygen, potassium, ...) you can create synapses to 
generate potential, advancing towards the enemies nucleus. By activating cells 
you control, these cells can neutralize incoming potential.

You randomly gain iron every few seconds. Iron can be used to activate the process of 
gathering new resources or to boost their production. But your resource production is 
also dependent on your current oxygen level and on how close the resource is to its 
limit. So be careful when building neurons: the more resources are bound in
neurons, the less resources you gain!

Once you started gaining dopamine and serotonin, you can develop advanced
technologies, allowing you to f.e. increase your resource limits or target specific enemy 
neurons and hence destroy enemy synapses or activated neurons or even block the
enemies resource-production.

Once you gained enough resources you can expand your control over the brain by
building more nucleus'. Use these, your build potential and the strategist
inside you to overcome dissonance in your favor!

## Installation 

### Requirements 
- gcc/>10.0 (available in most package managers)  
- conan (f.e. `pip install conan`)

You might also need to install some media libraries to play audio which is not in `.wav` 
format. For this please refer to the
[aubio](https://aubio.org/manual/latest/installing.html#external-libraries)
documentation.

### Installation

#### Quick-quide:
```
git clone git@github.com:georgbuechner/dissonance.git
cd dissonance
make aubio
make install
```

#### Detailed installation quide 

Clone project: 
```
git clone git@github.com:georgbuechner/dissonance.git
```

Now `cd` into the project
```
cd dissonance
```

Then install aubio (for analysing sound):
```
make aubio
```
In case you experience the error `/usr/bin/env: ‘python’: No such file or directory`:
- Check if python is correctly installed. 
- If python3 is install you may want to create a symlink with the following
  command: `sudo ln -sf /usr/bin/python3 /usr/bin/python`

On MacOs: 
- if you see `cp /usr/local/lib/libaubio.so.5: No such file or
directory` you can ignore this. 

Finally you install *dissonance* by running:
```c
make install  // (re-run this step for updates also) 
```

These steps install `dissonance` system wide and create `.dissonance` in the home folder to
store settings and analysed musical data.

### Usage

To play, simply run `dissonance` in you terminal. 

You can run `dissonance` with the `-r` option, to create the map based on your
current terminal size. Doing this will however change the game experience and
two identical songs will no longer produce an identical map and experience. 


### Game Details 

#### Resources 
Resources all follow a certain structure: 
- `free`: currently available units of resource.
- `bound`: units bound in certain neurons (in case of iron: iron distribute to resources).
- `limit`: max resource limit. 
- `boost`: distributed iron to this resource.

The gain of resources is determined by the amount of iron distributed to a
resource, the current oxygen and how close the resource is to it's limit. Iron
basically follows the same system, only that it has a constant boost factor and
is only generated when certain notes are played (more accurately: when in a
certain frame only notes outside of the key where played - *high dissonance*).
In addition, the slowdown factor reduces the current gain, this provides the
option to reduce the slowdown curve with technologies.

Formula: `(boost * gain * negative-faktor)/slowdown` where...
- ...boost is calculated with `1 + boost/10`
- ...gain is calculated with `|log(current-oxygen+0.5)|`
- ...negative-factor is calculated with `1 - (free+bound)/limit`
- ...slowdown is `3` when game starts and can be reduced up to `0.5` with technologies.


| name        | group   |
|:-----------:|:-------:|
| &#03B6; iron (FE) | Resources |
| &#03B8; oxygen (O2) | Resources/ Production |
| &#03BA; potassium (K) | Attack | 
| &#03B3; chloride (KCI) | Attack |
| &#03B7; glutamate (Glu) | Defence |
| &#03B4; dopamine (DA) | Technologies | 
| &#03C3; serotonin (5-HT) | Technologies |
-------------------------------------

#### Neurons and Potential
| name            | group   | description   | costs   | 
|:---------------:|:-------:|---------------| ------- |
| &#1455B; nucleus | expansion | nucleus from which everything is controlled. | 02, Glu, K, KCI, Da, 5-HT |
| &#0398; activated neuron | defence/ neuron | activates a cell, thus reducing the potential of incoming EPSP or IPSP. | O2, Glu | 
| &#0394; Synapse         | attack/ neuron  | creates potential (epsp, ipsp). | O2, K | 
| [1..9] epsp            | attack/ potential | potential traveling to enemy neurons, aiming to *destroy* these. | K | 
| [a..z] ipsp            | attack/ potential | potential traveling to enemy neurons, aiming to *block* these or enemy epsp. | K, KCI | 


#### Technologies
Most technologies are automatically applied. In the case of the technologies
belonging to the group *attack*, new technologies allow you to change the
configuration of you synapses. These configuration can be different for each
synapses, potential launched from different synapses can go different ways and have
different target (targets for epsp and ipsp can also be selected independent
from each other).

| name            | group | description   | steps |
|:---------------:|------|--------| ------ |
| choose way | attack | specify way-points to plan your attacks more accurately | 3 (1, 2, 3 way-points) |
| swarm attack | attack | synapse release a "swarm" of epsps | 3 (4, 7, 10) |
| chose target | attack | allows a synapse to specify a target | 2  (1: ipsp, 2:ipsp+epsp) |
| total resources | resources | increases maximum of each resource by 20% | 3 |
| slowdown curve | resources | lowers resource-slowdown | 3 |
| epsp/ipsp potential | potential-upgrade | increases potential of ipsp and epsp |  3|
| epsp/ipsp speed | potential-upgrade | increases speed of ipsp and epsp | 3 |
| ipsp duration | potential-upgrade | enemy neurons are blocked for a longer time | 3 |
| activated-neuron potential | activated-neuron upgrade| increases potential-slowdown | 3|
| activated-neuron speed | activated-neuron upgrade| increase speed of triggered slowdown| 3|
| nucleus range | expansion | increases range of all controlled nucleus' | 3 |


#### Costs

In general the costs are applied to the `free` units of each resource. For the
following entries, in addition the costs are added to the `bound` field:
- nucleus
- activated neuron
- synapse

For the following technologies/ neurons the costs are increased:
- technologies: `costs = costs*cur-step`
- nucleus: `costs = costs*(number-of-nucleus)`

| name    | iron | oxygen | potassium | chloride | glutamate | dopamine | serotonin |
|:--------|------|--------|-----------|----------|-----------|----------|-----------|
| Neuron: nucleus | 1 |30 | 30        | 30       | 30        | 30       | 30        |
| Neuron: activated neuron | - | 8.9 | - | -     | 19.1      | -        | -         |
| Neuron: synapse | - | 13.4 | 6.6    | -        | -         | -        | -         | 
| Potential: epsp | - | - | 4.4       | -        | -         | -        | -         |
| Potential: ipsp | - | - | 3.4       | 6.8      | -         | -        | -         |
| Tech: choose way | 1 | -| -         |-         | -         | 17.7     | -         |
| Tech: swarm      | 1 | -| -         |-         | -         | 19.9     | -         |
| Tech: target     | 1 | -| -         |-         | -         | 16.5     | -         |
| Tech: total resources | 1 | -| -    |-         | -         | 18.5     | 17.9      |
| Tech: slowdown   | 1 | -| -         |-         | -         | 21.0     | 21.2      |
| Tech: epsp/ipsp potential| 1 |-| 10 |-         | -         | 16.0     | 11.2      |
| Tech: epsp/ipsp speed    | 1 |-| 10 |-         | -         | 19.0     | 13.2      |
| Tech: ipsp duration      | 1 |-| 10 |-         | -         | 17.5     | 12.2      |
| Tech: activated-neuron potential | 1| - | - |  | 15.9      | 14.5     | 17.6      |
| Tech: activated-neuron speed | 1 | -| - | -    | 15.9      | 16.5     | 6.6       |
| Tech: nucleus range | 1 | 10 | -    | -        | -         | 13.5     | 17.9      |


## Acknowledgements:
I explicitly want to thank all the great developers who help develop the
great libraries that make `dissonance` possible:
- [aubio](https://github.com/aubio/aubio) the fantastic library for analysing audio.
- [miniaudio](https://github.com/aubio/aubio), which grants the possibility
  to acctually *play* audio-files.
- [lyra](https://github.com/bfgroup/Lyra) the beautiful command line argument
  parser.
- [catch2](https://github.com/catchorg/Catch2), although I apologize for not
  having written enough tests for this project so far!
- [nlohmann/json](https://github.com/catchorg/Catch2), the insanely good
  json-library (which probably everyone is using?)
- [spdlog](https://github.com/catchorg/Catch2), a great logging library.
- and finally [ncurses](https://github.com/mirror/ncurses)! 
