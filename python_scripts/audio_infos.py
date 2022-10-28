import json 
import os


directory = "../dissonance/data/analysis"

avr_le = []
max_le = []
min_le = []
num_le = {}
for file in os.listdir(directory):
    if ".json" in file:
        filepath = directory + os.sep + file
        print(filepath)
        with open(filepath, "r") as data:
            analysis = json.load(data)
        average_level = analysis["average_level"]
        last_levels_above_average = []
        max_levels_above_average = []
        max_level = 0
        for tp in analysis["time_points"]:
            if tp['level'] > max_level:
                max_level = tp['level']
            if tp['level'] > average_level:
                last_levels_above_average.append(tp['level'])
            if tp['level'] <= average_level and len(last_levels_above_average) > 0:
                le = round(max(last_levels_above_average)-average_level)
                if le in num_le:
                    num_le[le] += 1
                else:
                    num_le[le] = 1
                max_levels_above_average.append(le)
                last_levels_above_average = []
        max_level = round(max_level-average_level)
        avr_le.append(sum(max_levels_above_average)/len(max_levels_above_average))
        max_le.append(max(max_levels_above_average))
        min_le.append(min(max_levels_above_average))
        filename = os.fsdecode(file)
        print(f"{filename}: average_level: {average_level}, max: {max_level}, max_le: {max_le[-1]}, max-les: {max_levels_above_average}")
        print("percentages: ", end="")
        for x in max_levels_above_average:
            print(f"{round(x*100/max_level)}%, ", end="")
    
f_max_le = max(max_le)
f_min_le = min(min_le)
f_avr_le = sum(avr_le)/len(avr_le)
print(f"averall max level excedance: {f_max_le}")
print(f"averall min level excedance: {f_min_le}")
print(f"averall average level excedance: {f_avr_le}")
for k,v in num_le.items():
    print(k, v)
