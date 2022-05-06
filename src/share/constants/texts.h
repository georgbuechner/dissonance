#ifndef SRC_CONSTANTS_TEXTS_H_
#define SRC_CONSTANTS_TEXTS_H_

#include <vector>
#include <string>
#include "share/constants/codes.h"

namespace texts {

  typedef std::vector<std::string> paragraph_t;
  typedef std::vector<paragraph_t> paragraphs_t;

  typedef std::pair<paragraph_t, std::vector<std::string>> paragraph_field_t;
  typedef std::vector<paragraph_field_t> paragraphs_field_t;

  const paragraph_t test = {
    "DISSONANCE",
    "test field: "
  };

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
      "Each player starts with a NUCLEUS with control over a few cells surrounding it.",
      "By gathering different resources (iron, oxygen, potassium, ...) you can create SYNAPSES to",
      "generate potential, advancing towards the enemy's nucleus. By activating cells",
      "you control, these cells can neutralize incoming potential.",
      "",
      "You randomly gain iron every few seconds. Iron can be used to activate the process of",
      "gathering new resources or to boost their production. But your resource production is",
      "also dependent on your current oxygen level and on how close the resource is to it's",
      "limit. So be careful when building neurons: the more resources are bound in",
      "neurons, the less resources you gain",
      "",
      "Once you started gaining DOPAMINE and SEROTONIN, you can develop advanced",
      "technologies, allowing you to f.e. increase your resource limits or target specific enemy",
      "neurons and hence destroy enemy synapses or activated neurons or even block the",
      "enemy's resource-production.",
      "",
      "Once you gained enough resources you can expand your control over the brain by",
      "building more nucleus. Use these, your build potential and the strategist",
      "inside you to overcome dissonance in your favor!"
    },
    {
      "When dissonance starts, remember you should boost oxygen and activate production of glutamate, ",
      "to start defending yourself.",
      "Also keep in mind, that there are two kinds of potential: ",
      "EPSP (strong in attack) and IPSP (blocks buildings and enemy potential); you should start with EPSP."
    },
  };

  const paragraphs_t help = {
    {
      "##### HELP #####",
      "",
      " --- POTENTIALS --- ",
      "EPSP: used to destroy enemy neurons. Adds small amount of potential.",
      "IPSP: used to a) block enemy neurons or b) 'swallow' potential of enemy epsp",
      "MACRO: used to destroy enemy neurons. Adds a total of 50 damage to all nearby neurons",
      "as long as potential is still availible. Is destroyed by first hit of activated neuron",
      "",
      " --- NEURONS --- ",
      "ACTIVATED NEURON: block enemy potential. Desceases potential of incoming potentials. range ~5.",
      "SYNAPSE: used to create potentials (epsp/ ipsp/ macro)",
      "NUCLEUS: increase resource-limits (10%), you can build neurons inside of the nucleus' range. ",
      "Nucleus can be built anywhere on the map.",
      "RESOURCE NEURON: cannot be built. Are automatically created when more than 2 iron is distributed to a resource",
      "Can be destroyed. If it is destroyed, all iron distributed to this resource is lost."
    },
    {
      "##### HELP #####",
      "",
      "--- COSTS (potentials/ neurons) ----", 
      "",
      "ACTIVATE NEURON: oxygen=8.9, glutamate=19.1",
      "NUCLEUS: iron=1, oxygen=8.2, potassium=8.2, chloride=8.2, glutamate=8.2, dopamine=16.4, serotonin=16.4",
      "SYNAPSE: oxygen=13.4, potassium=6.6",
      "EPSP: potassium=4.4",
      "IPSP: potassium=3.4, chloride=6.8",
    },
    {
      "##### HELP #####",
      "",
      "--- COSTS (technologies) ----", 
      "", 
      "Every technology consumes 1 iron. The resource cost are costs*(level+1).",
      "(meaning: doubled for second level and tripled for last level)",
      "", 
      "attack strategies (tactical):",
      "CHOOSE WAY (increase way-points for settings way): iron=1, dopamine=17.7",
      "SWARM ATTACK (launch swarm attacks: +3): iron=1, dopamine=19.9",
      "",
      "resources:",
      "RESOURCE LIMITS++ (max allowed resources + 20%): iron=1, dopamine=18.5, serotonin=17.9",
      "RESOURCE SLOWDOWN-- (resource curve slowdown - 1): iron=1, dopamine=21.0, serotonin=21.2",
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
      "Iron is used for (boosting) resource production. At least 2 iron must be distributed to a resource, in "
        "order for production of this resource to start.",
      "",
      "You should start investing into ACTIVATED NEURONS to defend yourself: for this you need OXYGEN and GLUTAMATE.",
      "",
      "To start building units, you first need to build a synapse.",
      "EPSP aims to destroy enemy neurons, while IPSP blocks buildings.",
      "",
      "You can enter a number (1..9) followed by 'e' or 'i' to create multiple epsps/ ipsps at once.",
      "",
      "Also remember you gain less resources the more resources you have, so keep you resources low!",
      "Finally: keep track of your bound resources, the more resources are bound into neurons, the less you gain!"
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
      "We added a song which should be fairly easy, while you're still learning.",
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

  const paragraphs_field_t tutorial_get_oxygen = {
    {{
      "TUTORIAL",
      "",
      "DISSONANCE has a slightly complicated but also explicit resource-system:",
      "\"iron\" (" SYMBOL_IRON ") increases resource-gain.",
      "\"oxygen\" (" SYMBOL_OXYGEN ") is needed for buildings (here: \"neurons\") and increases resource-gain.",
      "\"potassium\" (" SYMBOL_POTASSIUM ") and \"chloride\" (" SYMBOL_CHLORIDE ") are needed for units "
        "(here: \"potentials\").",
      "\"glutamate\" (" SYMBOL_GLUTAMATE ") is needed for defence towers (here: \"activated neurons\")",
      "\"dopamine\" (" SYMBOL_DOPAMINE ") and \"serotonin\" (" SYMBOL_SEROTONIN ") are needed for technologies.",
    }, {}},
    {{
      "TUTORIAL",
      "",
      "The current amount of oxygen determines the overall resource-gain.",
      "Iron can be freely distributed between all your resources.",
      "The more iron is distributed to one resources, the faster the amount of this resource will grow.",
      "Before not at least two iron are distributed to one resource, the growth is 0.",
      "",
      "So the positive effects of resource-gain are iron distributed to the resource in question and oxygen.",
      "The negative effects are how close the resource is to it's limit. This also includes resources bound in neurons.",
      "",
      "The formula looks something like this: ",
      "iron * log(oxygen+0.5) * (1 - (free+bound)/limit)",
      "",
     }, {}},
    {{
      "TUTORIAL",
      "",
      "This is the exact formula, but it will be explained later so you don't need to understand everything now:",
      "",
      "(boost * gain * negative-faktor) / slowdown",
      "",
      "where...",
      "... boost = 1 + distributed-iron/10",
      "... gain = |log(current-oxygen+0.5)| ",
      "... negative factor = 1 - (free+bound)/limit",
      "... slowdown is normally '3' but it can be reduced up to 0.5 with the technology 'resource slowdown--'",

    }, {}},
    {{
      "TUTORIAL",
      "",
      "Since 0 oxygen also means no resource-gain at all,",
      "you should start with distributing at least 2 iron to oxygen",
      "to get your resource-production running.",
    }, {}},
    {{
      "TUTORIAL",
      "",
      "Before we finally start, a few notes concerning the map:",
      "",
      "The map resembles a brain. All dots are neurons.",
      "The part of the brain which you control surrounds your nucleus (a " SYMBOL_DEN " in the same",
      "color as your name which you should see at the top)",
      "The other X is the nucleus of your enemy you are at dissonance with.",
    }, {"field_only_den"}},
    {{
      "TUTORIAL",
      "",
      "Surrounding your nucleus you will see 'strange' symbols. These are resource-neurons",
      "each resembling one resource. They will change color as soon as the resource is activated (two",
      "iron distributed to this resource)",
      "",
      "iron " SYMBOL_IRON "   oxygen " SYMBOL_OXYGEN "   potassium " SYMBOL_POTASSIUM "   glutamate  " SYMBOL_GLUTAMATE 
        "   dopamine " SYMBOL_DOPAMINE "   serotonin " SYMBOL_SEROTONIN,
      "(In the second image oxygen has been activated, and in the third oxygen, glutamate and potassium)"
    }, {"field_player", "field_player_oxygen_activated", "field_player_oxygen_potassium_glutamat_activated"}},
    {{
      "TUTORIAL",
      "",
      "Resource-neurons can be subject to enemy attacks and thus be destroyed just as any other",
      "neurons too.",
    }, {}},
    {{
      "TUTORIAL",
      "",
      "Just as selecting the audio, use 'j'/'k' to cycle through your resources on the right side.",
      "Use '+'/'-' to add or remove iron from a resource.",
    }, {}},
  };

  const paragraphs_field_t tutorial_get_glutamat = {
    {{
      "TUTORIAL",
      "",
      "Nice, now that your resource-production has started you should start gathering GLUTAMATE",
      "to build up a defensive layer of activated neurons blocking incoming enemy potential: ",
      "distribute at least 2 iron to GLUTAMATE.",
    }, {}}
  };

  const paragraphs_field_t tutorial_build_activated_neurons = {
    {{
      "TUTORIAL",
      "",
      "Good job!",
      "Now wait till you have enough glutamate and then activate at least THREE neurons.",
      "The bar at the top shows you which hotkeys to use for which neurons, potentials or actions.",
      "For ACTIVATED NEURONS use 'A'.",
      "Also the field in the bar will turn blue once you have sufficient resources."
    }, {}}
  };

  const paragraphs_field_t tutorial_first_build = {
    {{
      "TUTORIAL",
      "",
      "Great, you gathered all necessary resources. Now you can activate a neuron",
      "to build your defensive layer.",
      "",
      "Once again you must use the 'h'/'j'/'k'/'l'-keys, this time to select a position on the map",
      "h (left), j (down), k (up), l (right)",
    }, {}},
    {{
      "TUTORIAL",
      "",
      "You can build/ activate neurons only in the range of your nucleus.",
      "The range is highlighted in green and the blue 'x' shows the currently selected position.",
      "(you can later expand your range of influence in the brain by researching technologies",
      "or building new nucleus across the map)",
    }, {"field_select_neuron_position_1", "field_select_neuron_position_2", "field_select_neuron_position_3"}},
    {{
      "TUTORIAL",
      "",
      "Place THREE activated neurons.",
     }, {}}
  };

  const paragraphs_field_t tutorial_get_potassium = {
    {{
      "TUTORIAL",
      "",
      "Good, now we can focus on other things. But keep checking your activated neurons",
      "they might get destroyed. Also you should check your nucleus life. If too many",
      "enemies are coming through you should consider adding activated neurons!",
    }, {}},
    {{
      "TUTORIAL",
      "",
      "The next thing we will need is POTASSIUM.",
      "Potassium is used for everything connected to creating potentials (attack units).",
      "Distribute at least 2 iron to POTASSIUM!",
    }, {}}
  };
  
  const paragraphs_field_t tutorial_build_synapse = {
    {{
      "TUTORIAL",
      "",
      "Potentials are created in a SYNAPSE.",
      "Build a synapse just like you built an activated neuron.",
      "The top bar will indicate when you have enough resources. The hotkey is 'S'."
    }, {}}
  };

  const paragraphs_field_t tutorial_build_potential = {
    {{
      "TUTORIAL",
      "",
      "Great! Now you can build potentials (EPSP and IPSP) to attack your enemy.",
      "Also, by sending potentials to the enemy's nucleus that part of the brain will be revealed.",
    }, {}},
    {{
      "TUTORIAL",
      "",
      "For a start build 1 EPSP by pressing 'e' to get an idea of the enemy's sphere of influence.",
    }, {}},
    {{
      "TUTORIAL",
      "",
      "While you are waiting for your epsp to arrive at the enemy's nucleus you should check whether",
      "new activated neurons are needed to keep up your defence. Also you can start activating",
      "other resources.",
    }, {}}
  };

  const paragraphs_field_t tutorial_select_target = {
    {{
      "TUTORIAL",
      "",
      "Your epsps have arrived at the enemy's location!",
      "Now that you can see which neurons your enemy has created, you can specifically target",
      "these neurons.",
      "",
      "Press 's' to select the synapse menu, then press 'e' to select [e]psp target.",
      "Then you can select the target for all epsps created at this synapse.",
      "If you can see the enemy's synapse try to destroy it. Otherwise select a resource-neuron.",
    }, {}},
    {{
      "TUTORIAL",
      "",
      "If you already have multiple synapses you will first have to select one synapse.",
      "After hitting 's', all your synapses are shown on the map as a, b,..., n. Simply press the corresponding",
      "letter to select the synapse. You can choose different targets for each synapse",
      "and different targets for epsp and ipsp (we will come to ipsp later).",
      "",
      "The map shows how the first of two SYNAPSES is selected by first pressing 's' (select synapse(s))",
      "and then 'a' (select first synapse)."
    }, {"multi_synapse_selection_1", "multi_synapse_selection_2", "multi_synapse_selection_3"}}
  };
  const paragraphs_field_t tutorial_strong_attack = {
    {{
      "TUTORIAL",
      "",
      "Now you can launch a new attack to destroy the selected neuron.",
      "Instead of pressing 'e' multiple times you can also press any number (1-9) followed from 'e'.",
      "This will create multiple epsps at once.",
    }, {}}
  };

  const paragraphs_field_t tutorial_build_ipsp = {
    {{
      "TUTORIAL",
      "",
      "With CHLORIDE you can build IPSP.",
      "Ipsp can be used in two ways: ",
      "a) to block any enemy neurons: activated neurons, synapses, resource-neurons.",
      "b) since ipsp 'swallow' enemy epsp, they can be used to block enemy attacks.",
    }, {}},
    {{
      "TUTORIAL",
      "",
      "Most of the time ipsps are used to block the enemy's ACTIVATED NEURONs just before your epsp arrives.",
      "Blocked activated neurons cannot neutralize epsp.",
      "This way you can overcome even large defensive layers.",
      "If you use ipsp to block enemy-neurons you will most certainly select the ipsp's target. ",
      "Press 's' to select the synapse menu, then press 'i' to select [i]psp target.",
      "Also it might be useful to create multiple synapses each with a different target for ipsps.",
      "(ipsp and epsp targets are different, while way-points (discussed later) are shared)",
    }, {}},
    {{
      "TUTORIAL",
      "",
      "Build ipsp by hitting 'i' or a number (1-9) followed by 'i'.",
      "Ipsp targets are selected just as epsp targets: 1. select synapse ('s'), 2. hit 'i' to set [i]psp target.",
    }, {}}
  };

  const paragraphs_field_t tutorial_technologies_dopamine = {
    {{
      "TUTORIAL",
      "",
      "Now that you have started gathering DOPAMINE you can research first technologies.",
      "",
      "Dopamine is most important for battle-related technologies. There are two technologies which can be",
      "researched with only dopamine (meaning: without serotonin): tactical technologies",
      "a) 'choose way': select 1-3 way-points, f.e. in order to bypass the enemy's defensive layers",
      "b) 'swarm attack': if swarm attack is activated, all epsps are released at once, ",
      "as soon as a critical amount is reached: lvl-0: 1, lvl-1: 4, lvl-2: 7, lvl-3: 10",
    }, {}},
  };

  const paragraphs_field_t tutorial_technologies_seretonin = {
    {{
      "TUTORIAL",
      "",
      "Now that you have started gathering SEROTONIN you are one step closer to researching ALL technologies.",
      "Technologies can help you to increase your influence, boost resource-production, strengthen your potentials",
      "or add battle-tactics.",
    }, {}},
  };

  const paragraphs_field_t tutorial_technologies_all = {
    {{
      "TUTORIAL",
      "",
      "Finally you can research ALL technologies. Apart from the already mentioned tactical technologies.",
      "There are three major 'areas' for technologies: ",
      "resource-upgrade, potential-upgrade, activated neuron-upgrade."
    }, {}},
  };

  const paragraphs_field_t tutorial_technologies_how_to = {
    {{
      "TUTORIAL",
      "",
      "To research a technology, use 't' to switch between (iron-)distribution (resources) and technologies",
      "Then use 'j'/'k'-keys to select a technology and research it by pressing '[enter]'",
      "At the bottom screen a short description of each technology is given while selected.",
      "",
      "Also you can always press '?' to see the help screen, where you can view all costs (also for technologies).",
      "But be careful, only during the TUTORIAL and SINGLE_PLAYER the game is paused while viewing help!",
    }, {}}
  };

  const paragraphs_field_t tutorial_final_attack = {
    {{
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
    }, {}},
    {{
      "TUTORIAL",
      "",
      "When you're ready, press 'y'"
    }, {}},
  };

  const paragraphs_field_t tutorial_final_attack_set_way = {
    {{
      "TUTORIAL",
      "",
      "If you have researched the CHOOSE WAY technology you can optimize the way of your potentials.",
      "However, if you're not totally comfortable with the controls you may skip this advice.",
      "",
      "Select a synapse with 's' and then the synapse you want to change the way for, with a, b, ...",
      "(all your synapses should be marked on the map with a, b, ...)",
      "",
      "Select 's' for '[s]elect way points'. You can now set as many way points as your research-level allows (1-3).",
      "Try to select the way-points so that your potentials pass the least amount of enemy activated neurons.",
      "After each way-point is selected you can see the current way of your epsp marked in purple.",
      "Applied ways are used for epsp AND ipsp (of course their targets might be different).",
    }, {}},
    {{
      "TUTORIAL",
      "",
      "When you're ready, press 'y'"
    }, {}},
  };

  const paragraphs_field_t tutorial_final_attack_set_targets = {
    {{
      "TUTORIAL",
      "",
      "First, for each synapse we select the ipsp targets. Try to make each synapse target one of the enemy's "
        "activated neurons.",
      "Of course this will not work for any number of enemy activated neurons. In that case, you should focus",
      "on the first few activated neurons on the way to the epsp's target.",
      "'s' followed by 'a' to select first synapse, then 'i' (for '[i]psp target') and select first activated neuron.",
      "'s' followed by 'b' to select second synapse, then 'i' (for '[i]psp target') and select second activated neuron.",
      "...",
      "(repeat for all your synapses)",
      "",
      "Finally make sure to set the epsp target of one synapse to the enemy's nucleus: ",
      "'s' followed by 'a' to select first synapse, then 'e' (for '[e]psp target') and select the enemy's nucleus.",
      "if you have researched SWARM ATTACK, you should toggle swarm attack on for this synapse.",
      "",
    }, {}},
    {{
      "TUTORIAL",
      "",
      "When you're ready, press 'y'"
    }, {}},
  };

  const paragraphs_field_t tutorial_final_attack_launch_attack = {
    {{
      "TUTORIAL",
      "",
      "When all this is set up, you are ready to launch your attack:",
      "1. quickly launch all ipsps: '2' 'i' 'a' (first synapse), '2' 'i' 'b' (second synapse), ... ",
      "2. quickly launch epsp attack: '9' 'e' 'a'",
    }, {}},
    {{
      "TUTORIAL",
      "",
      "These attacks are not easy to coordinate. It will take some time to become familiar with the using the hotkeys",
      "and estimating the duration of your ipsps as well as the time it takes your potentials to arrive.",
    }, {}}
  };

  const paragraphs_field_t tutorial_bound_resources = {
    {{
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
      "F.e. if you have only a small amount of potassium, you will gain potassium very fast.",
      "The closer it gets to it's limit the slower potassium fills up.",
      "(standard-limit=100, glutamate=150, dopamine=70 and serotonin=70)",
      "",
      "The crucial aspect is the BOUND proportion of a resource.",
      "All resources which are used for creating neurons are then bound inside of this neuron until the neuron"
        " is destroyed.",
      "This basically means: the more f.e. ACTIVATED NEURONS you have the less OXYGEN and GLUTAMATE you gain.",
      "(the same goes for SYNAPSEs or NUCLEUS)",
    }, {}},
    {{
      "TUTORIAL",
      "",
      "If you have come to a point where you seem to be getting no more resources at all, you have three options:",
      "a) research the 'resource limits++'-technology, which increases the limits for each resource (20%).",
      "b) by building new nucleus you also get increased resource limits (10%)",
      "c) destroy own neurons (friendly fire)"
    }, {}},
    {{
      "TUTORIAL",
      "",
      "Maybe you already noticed but to see not only the 'free' amount of a resource but also the bound and boost part,",
      "you can select a resource with 'j'/'k'-keys and a more detailed description of the resource will be shown",
      "at the bottom of the screen (just as with technologies).",
    }, {}}
  };

  const paragraphs_field_t tutorial_first_attack = {
    {{
      "TUTORIAL",
      "",
      "Your enemy has launched an attack!",
      "Units (potentials) are symbolized either by letters: a, b, ..., z (epsp) or numbers: 1,2,..., 9 (ipsp).",
      "The enemy's epsps add potential to your neurons. If a certain potential is reached this neuron is destroyed.",
      "Also: you cannot place new neurons on top of a destroyed neuron!",
      "If your nucleus is destroyed, you have lost.",
      "You can see the current potential of your nucleus at the top of the screen.",
      "",
      "The map below shows some EPSP advancing from the enemy's SYNAPSE to an imaginary target outside of the map."
    }, {"field_attack"}},
    {{
      "TUTORIAL",
      "",
      "During the tutorial and single-player you may always hit [space] to examine the situation.",
    }, {}}
  };

  const paragraphs_field_t tutorial_first_damage = {
    {{
      "TUTORIAL",
      "",
      "Your enemy's epsp have reached your nucleus!",
      "Potential is now added to your nucleus. If your nucleus' potential reaches '9/9' you have lost!",
      "Consider building more ACTIVATED NEURONS."
    }, {"field_attack_2"}}
  };
}

#endif
