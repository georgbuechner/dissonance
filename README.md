# Table of contents
1. [Dissonance](#dissonance)
2. [Premise](#premise)
3. [Installation](#installation)
	1. [Requirements](#requirements)
	2. [Installation](#install)
		- [Quick-guide](#quick-guide)
		- [Detailed installation guide](#detailed-guide)
	1. [Usage](#usage)
	2. [Logfiles](#logfiles)
	3. [Tests](#tests)
	4. [Uninstall](#uninstall)
	5. [Known problems](#known-problems)
4. [Game details](#game-details)
	1. [Resources](#resources)
	2. [Neurons and potential](#neurons-and-potential)
	3. [Technologies](#technologies)
	3. [Costs](#costs)
5. [Acknowledgements](#Acknowledgements)


## Dissonance 

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
also dependent on your current oxygen level and on how close the resource is to it's 
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
- C++ compiler:  
  Linux: [gcc](https://gcc.gnu.org/) (available in most package managers),   
  MacOs: [clang](https://clang.llvm.org/) (available with `brew install clang`)  
- [conan](https://conan.io/) (available in most package managers, but also: `pip install conan`)
- [aubio](https://github.com/aubio/aubio)

Aubio installation: 
- Ubuntu: `sudo apt-get install aubio-tools libaubio-dev libaubio-doc`  
- For other linux distros and MacOs a `make aubio` step is included in this project
- If none of the above is working for you, checkout the official aubio-download page: 
  https://aubio.org/download

You might also need to install some media libraries to play audio which is not in `.wav` 
format. So if loading `.mp3` files or other audio-files like `.ogg` is not
working for you, please refer to the
[aubio-documentation](https://aubio.org/manual/latest/installing.html#external-libraries)
(however at least `.wav` and `.mp3` should work if the installation was successful).


### Installation<a name="install"></a>

#### Quick-quide<a name="quick-guide"></a>
```python
git clone git@github.com:georgbuechner/dissonance.git
cd dissonance

# ubuntu:
sudo apt-get install aubio-tools libaubio-dev libaubio-doc

# MacOs and other linux-distros:
make aubio  

make install
```

#### Detailed installation quide<a name="detailed-guide"></a>

Clone project: 
```
git clone git@github.com:georgbuechner/dissonance.git
```

Now `cd` into the project
```
cd dissonance
```

If *aubio* is not available in your systems package-manager, build *aubio* from
source:
```
make aubio
```
In case you experience the error `/usr/bin/env: ‘python’: No such file or directory`:
- Check if `python` is correctly installed. 
- If `python3` is installed, you may want to create a symlink with the following
  command: `sudo ln -sf /usr/bin/python3 /usr/bin/python`

Finally you install *dissonance* by running:
```c
make install  // (re-run this step for updates also) 
```

These steps install `dissonance` system-wide and create `.dissonance` in the home folder to
store settings and analysed musical data.

### Usage

To play, simply run `dissonance` in your command line. 

You can run `dissonance -r`, to create the map based on your
current terminal size. Doing this will however change the game experience and
two identical songs will no longer produce an identical map and experience. 

### Logfiles

If not changed manually, logfiles will be stored at `~/.dissonance/logs/` in the
format `[timestamp]_logfile.txt` f.e. `2021-10-13-01-44-47_logfile.txt`.

You can delete these manually f.e. (`rm ~/.dissonance/logs/2021-10*` will delete all
logs created in October 2021), or you can start the game with `dissonance -c`
respectively `dissonance --clear-log` which will delete all logfiles.

By default logging is set to `warn`, leading to very small log-files containing only
the most relevant information. Consider including these files if you are filing
an issue. You may also increase the log-level with `dissonance -l` respectively
`dissonance --log-level` (f.e. `dissonance -l "debug"`). 

### Tests

To run tests, run 
```
./build/bin/tests
```

### Uninstall
To uninstall `dissonance`, run:
```
make uninstall
```

If you installed *aubio* with make, you can uninstall, using:
```
make uninstall_aubio
```

### Known problems

#### MacOs installation: c++ compiler detected by cmake differs from conan profile.

Possible error:
```
CMake Error at conanbuildinfo.cmake:582 (message):
  Incorrect 'gcc', is not the one detected by CMake: 'Clang'
```

Currently it is expected that clang is the primary compiler, when installing
dissonance on MacOs (more specifically: cmake detects clang as compiler).
Thus the conan-profile should also be setup for clang. If this is not the case
(because you f.e. modified the conan default profile) you can revert this, by
running:
```
conan profile new default --detect
```

Of course this can only work, if clang is actually your main c++ compiler (conan
detects clang).

Afterwards re-run `make install`.

It is very likely that it is possible to compiler `dissonance` with gcc on MacOs
also, however, Makefile and CMakeList is not optimised for this, and you main
need to figure some problems out yourself.

#### Audio glitching/ noise on Arch Linux. 
As the documentation of [miniaudio](https://miniaud.io/docs/manual/index.html) 
correctly points out, these issues might be fixed by applying the fix mentioned
in the arch-linux wiki: https://wiki.archlinux.org/title/PulseAudio/Troubleshooting#Glitches,_skips_or_crackling.

## Game Details 

### Resources 
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
| iron (ζ) | Resources |
| oxygen (ο) | Resources/ Production |
| potassium (κ) | Attack | 
| chloride (γ) | Attack |
| glutamate (η) | Defence |
| dopamine (δ) | Technologies | 
| serotonin (σ) | Technologies |
-------------------------------------

### Neurons and Potential
| name            | group   | description   | costs   | 
|:---------------:|:-------:|---------------| ------- |
| nucleus (Χ)     | expansion | nucleus from which everything is controlled. | ζ, ο, κ, γ, η, δ, σ |
| activated neuron (Φ) | defence/ neuron | activates a cell, thus reducing the potential of incoming EPSP or IPSP. | ο, η | 
| Synapse (Ξ)  | attack/ neuron  | creates potential (epsp, ipsp). | ο, κ | 
| [1..9] epsp            | attack/ potential | potential traveling to enemy neurons, aiming to *destroy* these. | κ | 
| [a..z] ipsp            | attack/ potential | potential traveling to enemy neurons, aiming to *block* these or enemy epsp. | κ, γ | 


### Technologies
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


### Costs

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


## Acknowledgements
I explicitly want to thank all the great developers who help write and maintain the
awesome libraries makeing `dissonance` possible:
- [aubio](https://github.com/aubio/aubio) the fantastic library for analysing audio.
- [miniaudio](https://github.com/aubio/aubio), which grants the possibility
  to acctually *play* audio-files.
- [lyra](https://github.com/bfgroup/Lyra) the beautiful command line argument
  parser.
- [catch2](https://github.com/catchorg/Catch2) (although I apologize for not
  having written enough tests for this project so far!)
- [nlohmann/json](https://github.com/catchorg/Catch2), the insanely good
  json-library (which probably everyone is using?)
- [spdlog](https://github.com/catchorg/Catch2), a great logging library.
- and finally [ncurses](https://github.com/mirror/ncurses)! 
