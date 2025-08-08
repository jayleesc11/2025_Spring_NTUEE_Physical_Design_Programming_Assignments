import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
import matplotlib.colors as mcolors

def read_rectangles(filename):
    rectangles = []
    with open(filename, 'r') as file:
        for _ in range(5):
            next(file)
        for line in file:
            parts = line.strip().split()
            if len(parts) == 5:
                name = parts[0]
                x1, y1, x2, y2 = map(float, parts[1:])
                rectangles.append((name, x1, y1, x2, y2))
    return rectangles

rectangles = read_rectangles('outputs/ami33.rpt')
# rectangles = read_rectangles('outputs/ami49.rpt')
# rectangles = read_rectangles('outputs/apte.rpt')
# rectangles = read_rectangles('outputs/hp.rpt')
# rectangles = read_rectangles('outputs/xerox.rpt')

fig, ax = plt.subplots()
colors = list(plt.rcParams['axes.prop_cycle'].by_key()['color'])

for i, (name, x1, y1, x2, y2) in enumerate(rectangles):
    width = x2 - x1
    height = y2 - y1
    color = colors[i % len(colors)]
    rect = Rectangle((x1, y1), width, height, fill=True, facecolor=color, edgecolor='black', alpha=0.5)
    ax.add_patch(rect)
    
    ax.text(x1 + width/2, y1 + height/2, name, 
            horizontalalignment='center', verticalalignment='center',
            color='black', fontweight='bold')

x_max = max(rect[3] for rect in rectangles)
y_max = max(rect[4] for rect in rectangles)
ax.set_xlim(0, x_max * 1.1)
ax.set_ylim(0, y_max * 1.1)

plt.title('Floorplan result')
plt.xlabel('X')
plt.ylabel('Y')
plt.grid(True)
plt.show()