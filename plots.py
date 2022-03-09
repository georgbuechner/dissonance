
import click
import json
import numpy as np
import matplotlib.pyplot as plt
import random

from typing import List, Tuple

def gen_resource_stable_oxygen(
    boast: int, seconds: int, limit: int, curve: int
) -> Tuple[List[float], List[float]]:
    oxygen = [5.5]
    for i in range(seconds-1):
        if i % 2 in [0, 1]:
            cur_oxygen = oxygen[-1]
            gain = np.log((cur_oxygen+0.5)/curve)
            boast = 1+boast/10
            negative_factor = 1-cur_oxygen/limit
            oxygen.append(cur_oxygen + boast * gain * negative_factor)
        else:
            oxygen.append(oxygen[-1])

    return oxygen


@click.command()
@click.option('--boast', default=1)
@click.option('--seconds', default=100)
@click.option('--limit', default=100)
@click.option('--curve', default=3)
@click.option('--data', default="resources")
def run(boast, seconds, limit, curve, data):

    if data == "resources":
        oxygen = gen_resource_stable_oxygen(boast, seconds, limit, curve)
        plt.plot(range(seconds), oxygen, label=f"oxygen @boast={boast}")

    else:
        analysis = {}
        with open(data, "r") as data:
            analysis = json.load(data)
        # bpm = analysis["bpms"]
        # avarage_bpm = analysis["average_bpm"]
        # plt.plot(bpm, label="bpm")
        # plt.plot([avarage_bpm]*len(bpm), label="average bpm")
        average_level = analysis["average_level"]
        levels = []
        bpms = []
        for tp in analysis['time_points']:
            bpms.append(tp['bpm'])
            l = tp['level']
            levels.append(l)
        mid_average = (max(levels)+average_level)/2
        mid_average_low = (mid_average+average_level)/2
        mid_average_high = (max(levels)+mid_average)/2

        plt.plot(levels, label="level")
        plt.plot([average_level]*len(levels), label="average level")
        plt.plot([mid_average_low]*len(levels), label="mid average low")
        plt.plot([mid_average]*len(levels), label="mid average")
        plt.plot([mid_average_high]*len(levels), label="mid average high")

    plt.legend()
    plt.show()

if __name__ == '__main__':
    run()
