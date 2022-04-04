#ifndef SRC_CONSTANTS_TEXTS_H_
#define SRC_CONSTANTS_TEXTS_H_

#include <vector>
#include <string>

namespace texts {

  typedef std::vector<std::string> paragraph_t;
  typedef std::vector<paragraph_t> paragraphs_t;
  const paragraphs_t welcome_reduced = {
    {
      "Welcome to DISSONANCE",
      "",
      "This is a beta-version. Please report issues at:",
      "https://github.com/georgbuechner/dissonance/issues"
    },
    {
      "Welcome to DISSONANCE",
      "",
      "Several parts of the brain are in dissonance and started attacking each other",
      "with strong potentials, aiming to destroy the others nucleus."
    }
  };

  const paragraphs_t welcome = {
    {
      "Welcome to DISSONANCE",
      "",
      "This is a beta-version. Please report issues at:",
      "https://github.com/georgbuechner/dissonance/issues"
    },
    {
      "Welcome to DISSONANCE",
      "",
      "Several parts of the brain are in dissonance and started attacking each other",
      "with strong potentials, aiming to destroy the others nucleus."
    },
    {
      "Welcome to DISSONANCE",
      "",
      "Each player starts with a nucleus with control over a few cells surrounding it.",
      "By gathering different resources (iron, oxygen, potassium, ...) you can create synapses to",
      "generate potential, advancing towards the enemies nucleus. By activating cells",
      "you control, these cells can neutralize incoming potential.",
      "",
      "You randomly gain iron every few seconds. Iron can be used to activate the process of",
      "gathering new resources or to boost their production. But your resource production i",
      "also dependent on your current oxygen level and on how close the resource is to its",
      "limit. So be careful when building neurons: the more resources are bound i",
      "neurons, the less resources you gain",
      "",
      "Once you started gaining dopamine and serotonin, you can develop advanced",
      "technologies, allowing you to f.e. increase your resource limits or target specific enemy",
      "neurons and hence destroy enemy synapses or activated neurons or even block the",
      "enemies resource-production.",
      "",
      "Once you gained enough resources you can expand your control over the brain by",
      "building more nucleus'. Use these, your build potential and the strategist",
      "inside you to overcome dissonance in your favor!"
    },
    {
      "When dissonance starts, remember you should boost oxygen and activate production of glutamate, to start defending yourself.",
      "Also keep in mind, that there are two kinds of potential: ",
      "EPSP (strong in attack) and IPSP (blocks buildings); you should start with EPSP."
    },
  };

  const paragraphs_t help = {
    {
      "##### HELP #####",
      "",
      "--- COSTS (potentials/ neurons) ----", 
      "",
      "ACTIVATE NEURON: oxygen=8.9, glutamate=19.1",
      "SYNAPSE: oxygen=13.4, potassium=6.6",
      "EPSP: potassium=4.4",
      "IPSP: potassium=3.4, chloride=6.8",
    },
    {
      "##### HELP #####",
      "",
      "--- COSTS (technologies) ----", 
      "", 
      "attack strategies (tactical):",
      "CHOOSE WAY (select way/ way-points for neurons): iron=1, dopamine=17.7",
      "SWARM ATTACK (launch swarm attacks +3): iron=1, dopamine=19.9",
      "",
      "resources:",
      "RESOURCE LIMITS++ (max allowed resources: each): iron=1, dopamine=18.5, serotonin=17.9",
      "RESOURCE SLOWDOWN-- (resource curve slowdown): iron=1, dopamine=21.0, serotonin=21.2",
      "",
      "potential-upgrade:",
      "EPSP/ IPSP POTENIAL++ (increases potential of ipsp/ epsp): iron=1, potassium=10.0, dopamine=16.0, serotonin=11.2",
      "EPSP/ IPSP SPEED-- (increases speed of ipsp/ epsp): iron=1, potassium=10.0, dopamine=19.0 serotonin=13.2",
      "IPSP DURATION++ (increases duration of ipsp): iron=1, potassium=10.0, dopamine=17.5, serotonin=12.2",
      "",
      "activated-neuron upgrade",
      "ACTIVATED NEURON POTENIAL++ (increases potential of activated-neuron): "
        "iron=1, glutamate=15.9, dopamine=14.5, serotonin=17.6",
      "ACTIVATED NEURON SPEED++ (increases speed of activated-neuron): "
        "iron=1, glutamate=15.9, dopamine=16.5, serotonin=6.6",
      "",
      "expansion:",
      "RANGE OF NUCLEUS (increases range of nucleus): iron=1, dopamine=13.5, serotonin=17.9",
    },
    {
      "##### HELP #####",
      "",
      "--- TIPS ----", 
      "Iron is used for (boosting) resource production. At least 2 iron must be distributed to a resource, in order for production of this resource to start.",
      "",
      "You should start investing into activated neurons to defend yourself: for this you need oxygen and glutamate.",
      "",
      "To start building units, you first need to build a synapse.",
      "EPSP aims to destroy enemy buildings, while IPSP blocks buildings.",
      "",
      "You can first enter a number x∈{1..9} and the select epsp/ ipsp. This will automatically try to create x epsp/ ipsp.",
      "",
      "Also remember you gain less resources the more resources you have, so keep you resources low!",
      "Finally: keep track of your bound resources, the more resources are bound, the less you gain!"
    }
  };

  const paragraphs_t tutorial_start = {
    { 
      "Welcome to DISSONANCE tutorial",
      "",
      "You will start a normal game but we will give you hints and",
      "try to explain while you're playing."
    },
    {
      "TUTORIAL",
      "",
      "The first you'll have to do is to select a song to play along to.",
      "Your choice of music will influence the map and the style of the AI.",
      "We added a song which should be fairly easier, while you're still learning.",
      "",
      "However also the size of your terminal and the selected font will affect the map",
      "and as a direct effect the game's difficulty. You should generally consider playing on full screen.",
      "The game has been tested with a resolution of 1920x1080. However, higher resolutions allow a",
      "prettier printing of the side-bar.",
    },
    {
      "TUTORIAL",
      "",
      "The 'tutorial-track' can be found in the folder 'examples', it's called 'Hear My Call'",
      "Select this song, or if you feel a little 'adventurous' or in need for dissonance",
      "select any other song you like.",
    },
    {
      "TUTORIAL",
      "",
      "Use 'j'/'k' keys to select vertically, use 'h'/'l' to go up or down then directory-tree.",
      "You should remember this kind of navigation for other menus too.",
      "You can add music libraries from your computer to select songs from using '+'"
    },
  };

  const paragraphs_t tutorial_get_oxygen = {
    {
      "TUTORIAL",
      "",
      "DISSONANCE has a slightly complicated but also explicit resource-system:",
      "\"iron\" increases resource-gain.",
      "\"oxygen\" is needed for buildings (here: \"neurons\") and increases resource-gain.",
      "\"potassium\" and \"chloride\" are needed for units (here: \"potentials\").",
      "\"glutamate\" is needed for defence towers (here: \"activated neurons\")",
      "\"dopamine\" and \"serotonin\" are needed for technologies.",
    },
    {
      "TUTORIAL",
      "",
      "The current amount of oxygen determines the current overall resource-gain.",
      "Iron can be freely distributed between all your resources.",
      "The more iron is distributed to one resources, the faster the amount of this resource will grow.",
      "Before not at least two iron are distributed to one resource, the growth is 0.",
    },
    {
      "TUTORIAL",
      "",
      "Since 0 oxygen also means no resource-gain at all,",
      "you should start with distributing at least 2 iron to oxygen",
      "to get your resource-production running",
    },
    {
      "TUTORIAL",
      "",
      "Before we finally start, A few notes concerning the map:",
      "",
      "The map resembles a brain. All dots are neurons.",
      "The part of the brain which you control surrounds your nucleus (a 'X' in the same",
      "color as your name which you should see at the top)",
      "The other X is the nucleus of your enemy you are at dissonance with.",
    },
    {
      "TUTORIAL",
      "",
      "Surrounding your nucleus you will shortly see 'strange' symbols. These are resource-neurons",
      "each resembling one resource. They will change color as soon as the resource is activated (two",
      "iron distributed to this resource)",
      "Resource-neurons can be subject to enemy attacks and thus be destroyed just as any other",
      "neurons too.",
    },
    {
      "TUTORIAL",
      "",
      "Just as selecting the audio, use 'j'/'k' to cycle through your resources on the right side.",
      "Use '+'/'-' to add or remove iron from a resource.",
    },
  };

  const paragraphs_t tutorial_get_glutamat = {
    {
      "TUTORIAL",
      "",
      "Nice, now that your resource-production has started you should start gathering GLUTAMATE",
      "to build up a defensive layer of activated neurons blocking incoming enemy potential: ",
      "distribute at least 2 iron to GLUTAMATE.",
    }
  };

  const paragraphs_t tutorial_build_activated_neurons = {
    {
      "TUTORIAL",
      "",
      "Good job!",
      "Now wait till you have enough glutamate and then activate at least THREE neurons.",
      "The bar at the top shows you which hotkeys to use for which neurons, potentials or actions.",
      "For ACTIVATED NEURONS use 'A'.",
      "Also the field in the bar will turn blue once you have sufficient resources."
    }
  };

  const paragraphs_t tutorial_first_build = {
    {
      "TUTORIAL",
      "",
      "Great, you gathered all necessary resources now you can activated a neuron",
      "to build your defensive layer.",
      "",
      "Once again you must use the 'h'/'j'/'k'/'l'-keys, this time to select a position on the map",
      "h (left), j (down), k (up), l (right)",
    },
    {
      "TUTORIAL",
      "",
      "You can build/ activate neurons only in the range of your nucleus.",
      "The range is highlighted in green.",
      "(you can later expand your range of influence in the brain by researching technologies",
      "or building new nucleus across the map)",
    }
  };

  const paragraphs_t tutorial_get_potassium = {
    {
      "TUTORIAL",
      "",
      "Good, now we can focus on other things. But keep checking your activated neurons",
      "they might get destroyed. Also you should check your nucleus life. If too many",
      "enemies are coming through you should consider adding activated neurons!",
    },
    {
      "TUTORIAL",
      "",
      "The next thing we will need is POTASSIUM.",
      "Potassium is used for everything connected to creating potentials (attack units).",
      "Distribute at least 2 iron to POTASSIUM!",
    }
  };
  
  const paragraphs_t tutorial_build_synapse = {
    {
      "TUTORIAL",
      "",
      "Potentials are created in a SYNAPSE.",
      "Build a synapse just like you built an activated neuron.",
      "The top bar will indicate when you have enough resources. The hotkey is 'S'."
    }
  };

  const paragraphs_t tutorial_build_potential = {
    {
      "TUTORIAL",
      "",
      "Great! Now you can build potentials (EPSP and IPSP) to attack your enemy.",
      "Also, by sending potentials to the enemy's nucleus that part of the brain will be reviled.",
    },
    {
      "TUTORIAL",
      "",
      "For a start build 1 EPSP by pressing 'e' to get an idea of the enemies sphere of influence.",
    },
    {
      "TUTORIAL",
      "",
      "While you are waiting for your epsp to arrive at the enemies nucleus you should check whether",
      "new activated neurons are needed to keep up your defence. Also you can start activating",
      "other resources.",
    }
  };

  const paragraphs_t tutorial_select_target = {
    {
      "TUTORIAL",
      "",
      "Now that you can see which neurons your enemy has created you can specifically target",
      "these neurons.",
      "",
      "Press 's' to select the synapse menu, then press 'e' to select [e]psp target.",
      "Then you can select the target for all epsps created at this synapse.",
      "If you can see the enemy's synapse try to destroy it. Otherwise select a resource-neuron.",
    },
    {
      "TUTORIAL",
      "",
      "If you already have multiple synapses you will first have to select one synapse.",
      "After hitting 's', all your synapses are shown on the map as a, b,..., n. Simply press the corresponding",
      "letter to select the synapse. You can choose different targets for each synapse",
      "and different targets for epsp and ipsp (we will come to ipsp later).",
    }
  };
  const paragraphs_t tutorial_strong_attack = {
    {
      "TUTORIAL",
      "",
      "Now you can launch a new attack to destroy the selected neuron.",
      "Instead of pressing 'e' multiple times you can also press any number (1-9) followed from 'e'.",
      "This will create multiple epsps at once.",
    }
  };

  const paragraphs_t tutorial_build_ipsp = {
    {
      "TUTORIAL",
      "",
      "With CHLORIDE you can build IPSP.",
      "Ipsp can be used in two ways: ",
      "a) to block any enemy neurons: activated neurons, synapses, resource-neurons.",
      "b) since ipsp 'swallow' enemy epsp, they can be used to block enemy attacks.",
    },
    {
      "TUTORIAL",
      "",
      "Most of the time ipsps are used to block the enemy's ACTIVATED NEURONs just before your epsp arrives.",
      "Blocked activated neurons cannot neutralize epsp.",
      "This way you can overcome even large defensive layers."
    },
    {
      "TUTORIAL",
      "",
      "Build ipsp by hitting 'i' or a number (1-9) followed by 'i'.",
      "Ipsp targets are selected just as epsp targets: 1. select synapse ('s'), 2. hit 'i' to set [i]psp target.",
    }
  };

  const paragraphs_t tutorial_technologies_dopamine = {
    {
      "TUTORIAL",
      "",
      "Now that you started gathering DOPAMINE you can research first technologies.",
      "",
      "Dopamine is most important for battle-related technologies. There are two technologies which can be",
      "researched with only dopamine (meaning: without serotonin): tactical technologies",
      "a) 'choose way': select 1-3 way-points, f.e. in order to bypass the enemy's defensive layers",
      "b) 'swarm attack': if swarm attack is activated, all epsps are released at once, ",
      "as soon as a critical amount is reached: lvl-0: 1, lvl-1: 4, lvl-2: 7, lvl-3: 10",
    },
  };

  const paragraphs_t tutorial_technologies_seretonin = {
    {
      "TUTORIAL",
      "",
      "Now that you started gathering SEROTONIN you are one step closer to researching ALL technologies.",
      "Technologies can help you to increase your influence, boost resource-production, strengthen your potentials",
      "or add battle-tactics.",
    },
  };

  const paragraphs_t tutorial_technologies_all = {
    {
      "TUTORIAL",
      "",
      "Finally you can research ALL technologies. Apart from the already mentioned tactical technologies.",
      "There are three major 'areas' for technologies: ",
      "resource-upgrade, potential-upgrade, activated neuron-upgrade."
    },
  };

  const paragraphs_t tutorial_technologies_how_to = {
    {
      "TUTORIAL",
      "",
      "To research a technology, use 't' to switch between (iron-)distribution (resources) and technologies",
      "Then use 'j'/'k'-keys to select a technology and research it by pressing '[enter]'",
      "At the bottom screen a short description of each technology is given while selected.",
      "",
      "Also you can always press 'h' to see the help screen, where you can view all costs (also for technologies).",
    }
  };

  const paragraphs_t tutorial_final_attack = {
    {
      "TUTORIAL",
      "",
      "You have stated gathering the most resources and are familiar with the most important tactics.",
      "We will now show you a possible way of designing a more complex attack.",
      "",
      "Prerequisites: ",
      "- Build at least two SYNAPSEs (ideally one for each enemy ACTIVATED NEURON).",
      "- Gather at least 50 POTASSIUM and 30 CHLORIDE",
      "- Research CHOOSE WAY technology (optional)",
      "- Research SWARM ATTACK technology (optional)",
    },
    {
      "TUTORIAL",
      "",
      "When you're ready, press 'y'"
    },
  };

  const paragraphs_t tutorial_final_attack_set_way = {
    {
      "TUTORIAL",
      "",
      "If you have researched the CHOOSE WAY technology you can optimize the way of your potentials.",
      "However, if you're not totally comfortable with the controls you may skip this advice.",
      "",
      "Select a synapse with 's' and then the synapse you want to change the way for, with a, b, ...",
      "(all your synapses should be marked on the map with a, b, ...)",
      "",
      "Select 's' for '[s]elect way points'. You can now set as many way points as your research-level allows (1-3).",
      "Try to select the way-point so that your potentials pass the least amount of enemy activated neurons.",
      "After each way-point is selected you can see the current way of your epsp marked in purple.",
      "Applied ways are used for epsp AND ipsp (of course their targets might be different).",
    },
    {
      "TUTORIAL",
      "",
      "When you're ready, press 'y'"
    },
  };

  const paragraphs_t tutorial_final_attack_set_targets = {
    {
      "TUTORIAL",
      "",
      "First, for each synapse we select the ipsp targets. Try to make each synapse target one of the enemies "
        "activated neurons.",
      "Of course this will not work for any number of enemy activated neurons. In that case, you should focus",
      "on the first few activated neurons on the way to the epsp's target.",
      "'s' followed by 'a' to select first synapse, then 'i' (for '[i]psp target') and select first activated neuron.",
      "'s' followed by 'b' to select second synapse, then 'i' (for '[i]psp target') and select second activated neuron.",
      "...",
      "(repeat for all your synapses)",
      "",
      "Finally make sure to set the epsp target of one synapse to the enemies nucleus: ",
      "'s' followed by 'a' to select first synapse, then 'e' (for '[e]psp target') and select the enemies nucleus.",
      "if you have researched SWARM ATTACK, you should toggle swarm attack on for this synapse.",
      "",
    },
    {
      "TUTORIAL",
      "",
      "When you're ready, press 'y'"
    },
  };

  const paragraphs_t tutorial_final_attack_launch_attack = {
    {
      "TUTORIAL",
      "",
      "When all this is set up, you are ready to launch your attack:",
      "1. quickly launch all ipsps: '2' 'i' 'a' (first synapse), '2' 'i' 'b' (second synapse), ... ",
      "2. quickly launch epsp attack: '9' 'e' 'a'",
    },
    {
      "TUTORIAL",
      "",
      "These attacks are not easy to coordinate. It will take some time to become familiar with the using the hotkeys",
      "and estimating the duration of your ipsps as well as the time it takes your potentials to arrive.",
    }
  };

  const paragraphs_t tutorial_bound_resources = {
    {
      "TUTORIAL",
      "",
      "Good thing that you are increasing your defence! However you should keep the following in mind:",
      "This is the formula for your research-gain per resource: ",
      "",
      "(boost * gain * negative-faktor) / slowdown",
      "",
      "where...",
      "... boost = 1 + distributed-iron/10",
      "... gain = |log(current-oxygen+0.5)| ",
      "... negative factor = 1 - (free+bound)/limit",
      "... slowdown is normally '3' but it can be reduced up to 0.5 with the technology 'resource slowdown--'",
      "",
      "We have already talked about resource-gain being dependent on oxygen and distributed iron.",
      "But it is additionally dependent on how close any resource is to it's limit.",
      "F.e. if you have only a small amount of POTASSIUM, you will gain POTASSIUM very fast.",
      "The closer it gets to it's limit the slower POTASSIUM fills up.",
      "(standard-limit=100, glutamate=150, dopamine=70 and serotonin=70)",
      "",
      "The crucial aspect is the BOUND proportion of a resource.",
      "All resources which are used for creating neurons are then bound inside of this neuron until the neurons"
        " is destroyed.",
      "This basically means: the more f.e. ACTIVATED NEURONS you have the less OXYGEN and GLUTAMATE you gain.",
      "(the same goes for SYNAPSEs or NUCLEUS)",
    },
    {
      "TUTORIAL",
      "",
      "If you have come to a point where you seem to be getting no more resources at all, you have two options:",
      "a) research the 'resource limits++'-technology, which increases the limits for each resource (20%).",
      "b) by building new nucleus you also get increased resource limits (10%)",
      "c) destroy own neurons (friendly fire)"
    },
    {
       "TUTORIAL",
      "",
      "Maybe you already noticed but to see not only the 'free' amount of a resource but also the bound and boost part,",
      "you can select a resource with 'j'/'k'-keys and a more detailed description of the resource will be shown",
      "at the bottom of the screen (just as with technologies).",
    }
  };
}

#endif
