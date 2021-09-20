
import click
import json
import numpy as np
import matplotlib.pyplot as plt
import random

from typing import List, Tuple

def gen_resource_stable_oxygen(
        boast: int, seconds: int, o_limit: int, r_limit: int, curve: int
) -> Tuple[List[float], List[float]]:
    resource = [0]
    oxygen = [5.5]
    bound_oxygen = [0]
    iron = [0]
    reached = False
    for i in range(seconds-1):
        oxygen.append(oxygen[-1] + (boast * (o_limit-(oxygen[-1]+bound_oxygen[-1]))/(curve*o_limit)/1.5))
        resource.append(resource[-1] + (np.log(oxygen[-1]) * (r_limit-resource[-1])/(curve*r_limit))/1.5)
        # if i % 100 == 0 and oxygen[-1] > 10:
        #     bound_oxygen.append(bound_oxygen[-1]+10)
        #     cur = oxygen.pop()
        #     oxygen.append(cur-10)
        # else:
        bound_oxygen.append(bound_oxygen[-1])
        if i % 100 == 0 and random.randrange(int(oxygen[-1])) == 0:
            iron.append(iron[-1]+1)
        else:
            iron.append(iron[-1])
        if resource[-1] > 40 and not reached:
            print(f"at curve={curve} reached 40 after {i} seconds")
            reached = True
    return oxygen, resource, bound_oxygen, iron


@click.command()
@click.option('--boast', default=1)
@click.option('--seconds', default=100)
@click.option('--o_limit', default=100)
@click.option('--r_limit', default=100)
@click.option('--curve', default=5)
@click.option('--data', default="resources")
def run(boast, seconds, o_limit, r_limit, curve, data):

    if data == "resources":
        oxygen, resources, bound_oxygen, iron = gen_resource_stable_oxygen(
            boast, seconds, o_limit, r_limit, curve
        )
        # oxygen_b, resources_b = gen_resource_stable_oxygen(
        #     boast, seconds, o_limit, r_limit, 2
        # )
        # oxygen_c, resources_c = gen_resource_stable_oxygen(
        #     boast, seconds, o_limit, r_limit, 1 
        # )
        plt.plot(range(seconds), oxygen, label=f"oxygen @boast={boast}")
        plt.plot(range(seconds), bound_oxygen, label=f"bound oxygen")
        plt.plot(range(seconds), resources, label="dopamine")
        plt.plot(range(seconds), iron, label="iron")
        # plt.plot(range(seconds), oxygen_b, label=f"oxygen b @boast={boast}")
        # plt.plot(range(seconds), resources_b, label="dopamine b")
        # plt.plot(range(seconds), oxygen_c, label=f"oxygen c @boast={boast}")
        # plt.plot(range(seconds), resources_c, label="dopamine c")

    else:
        analysis = {}
        with open(data, "r") as data:
            analysis = json.load(data)
        bpm = analysis["bpms"]
        levels = analysis["levels"]
        avarage_level = analysis["average_level"]
        avarage_bpm = analysis["average_bpm"]
        plt.plot(bpm, label="bpm")
        plt.plot([avarage_bpm]*len(bpm), label="average bpm")
        plt.plot(levels, label="level")
        plt.plot([avarage_level]*len(levels), label="average level")

    plt.legend()
    plt.show()

if __name__ == '__main__':
    run()
