import click
import json
import matplotlib.pyplot as plt
import math

# Function to find distance
def shortest_distance(x1, y1, a, b, c):
    return abs((a * x1 + b * y1 + c)) / (math.sqrt(a * a + b * b))

def line(pos_1, pos_2):
    x1, y1 = pos_1
    x2, y2 = pos_2
    a = y2-y1
    b = x1-x2
    c = a*x1 + b*y1
    return [a, b, -c]


# source: https://karthaus.nl/rdp/
def DouglasPeucker(data_points, epsilon):
    # Find the point with the maximum distance
    dmax = 0
    index = 0
    end = len(data_points)-1
    for i in range(1, end-1):
        a, b, c = line(data_points[0], data_points[end])
        d = shortest_distance(data_points[i][0], data_points[i][1], a, b, c)
        if d > dmax:
            index = i
            dmax = d

    result = []

    # If max distance is greater than epsilon, recursively simplify
    if dmax > epsilon:
        # Recursive call
        results_1 = DouglasPeucker(data_points[0:index], epsilon)
        results_2 = DouglasPeucker(data_points[index:end], epsilon)

        # Build the result list
        result = results_1 + results_2
    else:
        result = [data_points[0], data_points[end]]
    # Return the result
    return result

def get_num_zeros(num_pitches, max_elems):
    return math.sqrt(num_pitches/max_elems)

def get_epsilon(num_pitches, max_elems):
    num_zeros = get_num_zeros(num_pitches, max_elems)
    epsilon = 1*10**(num_zeros-1)
    if epsilon > 10000:
        epsilon = 10000
    return epsilon

def reduce_apx(pitches, epsilon):
    pitches_points = []
    for i in range(len(pitches)):
        pitches_points.append([i, pitches[i]])
    pitches_points = DouglasPeucker(pitches_points, epsilon)
    reduced_pitches = []
    for p in pitches_points:
        reduced_pitches.append(p[1])
    print(f"epsilon: {epsilon}, reduced: {len(reduced_pitches)}")
    return reduced_pitches

def reduce(pitches, max_elems):
    epsilon = get_epsilon(len(pitches), max_elems)
    reduced = reduce_apx(pitches, epsilon)
    max_e = math.sqrt(len(pitches))
    min_e = 0
    for _ in range(5):
        if abs((len(reduced)-max_elems)/len(reduced))*100 < 10:
            return reduced
        if len(reduced) > max_elems:
            min_e = epsilon
            epsilon = (epsilon+max_e)/2
        elif len(reduced) < max_elems:
            max_e = epsilon
            epsilon = (epsilon+min_e)/2
        reduced = reduce_apx(pitches, epsilon)
    return reduced

@click.command()
@click.option('--data', default="")
@click.option('--max_elems', default=500)
def run(data, max_elems):
    # get analysis
    analysis = {}
    with open(data, "r") as data:
        analysis = json.load(data)
    pitches = analysis["pitches"]

    # reduced 
    reduced_pitches = reduce(pitches, max_elems)

    avrg = sum(pitches)/len(pitches)
    avrg_reduced = sum(reduced_pitches)/len(reduced_pitches)

    # Print 
    num_pitches = len(pitches)
    num_reduced = len(reduced_pitches)
    diff = num_pitches-num_reduced
    percent = (num_reduced*100)/num_pitches
    print(f"pitches: {num_pitches}, reduced: {num_reduced}, diff: {diff}, percent: {percent}")

    # Plot
    plt.subplot(1, 2, 1) # row 1, col 2 index 1
    plt.plot(pitches, label="pitches")
    plt.plot([avrg]*len(pitches), label="arvg")
    plt.title("pitches")

    plt.subplot(1, 2, 2) # index 2
    plt.plot(reduced_pitches, label="pitches")
    plt.plot([avrg_reduced]*len(reduced_pitches), label="avrg")
    plt.title("reduced")

    plt.legend()
    plt.show()

if __name__ == '__main__':
    run()
