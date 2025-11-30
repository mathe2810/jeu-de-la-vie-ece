import random

width = 140
height = 40
filename = "glider_rain_safe.txt"

# définition du glider
glider = ["*..", "..*", "***"]
glider_width = len(glider[0])
glider_height = len(glider)

# création de la grille vide
grid = [["." for _ in range(width)] for _ in range(height)]

# nombre de gliders à placer
num_gliders = 30

# fonction pour vérifier si l'espace est libre
def is_free(x, y):
    for dy in range(glider_height):
        for dx in range(glider_width):
            if grid[y+dy][x+dx] != ".":
                return False
    return True

# placer les gliders sans chevauchement
placed = 0
attempts = 0
max_attempts = 1000

while placed < num_gliders and attempts < max_attempts:
    x = random.randint(0, width - glider_width)
    y = random.randint(0, height - glider_height)
    if is_free(x, y):
        for dy in range(glider_height):
            for dx in range(glider_width):
                grid[y+dy][x+dx] = glider[dy][dx]
        placed += 1
    attempts += 1

if placed < num_gliders:
    print(f"Warning: only placed {placed} gliders after {attempts} attempts.")

# sauvegarde dans un fichier
with open(filename, "w") as f:
    for row in grid:
        f.write("".join(row) + "\n")

print(f"{placed} gliders placés dans {filename}")
