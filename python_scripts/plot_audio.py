import json
import matplotlib.pyplot as plt

raw_analysis = []
with open("data/pitches.json", "r") as data:
    raw_analysis = json.load(data)

analysis = {}
for elem in raw_analysis:
    analysis[elem[0]] = elem[1]

maxi = -1
most_common = 0
for key, value in analysis.items():
    if value > maxi:
        most_commen = key
        maxi = value

pitches = []
with open("data/all_pitches.json", "r") as data:
    pitches= json.load(data)
average = sum(pitches)/len(pitches)
print(average)


plt.plot([average]*len(pitches), label="average")
plt.plot([most_common]*len(pitches), label="most common")
plt.plot(pitches, label="pitches")
plt.show()
