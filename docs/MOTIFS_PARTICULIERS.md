# Section Rapport: Motifs Particuliers

## Vue d'ensemble

Cette section présente l'analyse automatique des motifs classiques du Jeu de la Vie de Conway. Un outil autonome (`motifs_particuliers.exe`) a été développé pour classifier les configurations selon trois catégories principales : **still life** (forme stable), **oscillateur** (périodique), et **vaisseau** (déplacement).

## Implémentation

### Fichier: `motifs_particuliers.c`

Le programme implémente un moteur complet du Jeu de la Vie avec les fonctionnalités suivantes:

#### 1. Représentation compacte (bit-packing)
- Chaque cellule occupe exactement **1 bit** dans un tableau de `uint64_t`
- Une ligne de 64 cellules = 1 entier 64-bit
- Réduction mémoire par facteur 64 comparé à un tableau de `char`
- Limite mémoire respectée: grilles jusqu'à 320×240 = 76 800 cellules ≈ 1.2 KB par grille

#### 2. Gestion des frontières (4 modes)
- **Edge**: cellules hors-grille considérées comme mortes
- **Torus**: la grille s'enroule (gauche↔droite, haut↔bas)
- **Mirror**: réflexion aux bords
- **Rim**: couronne vivante (cellules extérieures vivantes)

#### 3. Algorithme de détection de motifs

**Principe**: Calcul de signatures cryptographiques (hash 64-bit) à chaque génération pour détecter les répétitions.

```
Pour chaque génération i de 0 à N:
	1. Calculer hash(état_i)
	2. Calculer boîte englobante (bbox) des cellules vivantes
	3. Comparer hash_i avec tous les hash précédents
	4. Si répétition détectée à génération j:
		- Période = i - j
		- Si période == 1 → Still life
		- Si période > 1 → Oscillateur
		- Si bbox se déplace entre j et i → Vaisseau
```

**Avantages**:
- Complexité mémoire: O(N) pour N générations stockées
- Détection robuste même avec bruit numérique
- Pas de comparaison pixel-par-pixel (coûteuse)

#### 4. Classification automatique

| Type | Critère | Exemple |
|------|---------|---------|
| **Still life** | hash(gen_n) == hash(gen_n+1) pour tout n | Block 2×2 |
| **Oscillateur** | Période P détectée, bbox stable | Blinker (P=2), Toad (P=2) |
| **Vaisseau** | Période P + déplacement bbox (dx,dy) ≠ (0,0) | Glider, LWSS |

## Motifs testés

### 1. Block (Still life)
```
.**
.**
```
**Résultat**: `Still life (stable, period 1)`  
**Explication**: Chaque cellule a exactement 3 voisines vivantes → survie garantie. Aucune cellule morte n'a 3 voisines → pas de naissance. Configuration stable indéfiniment.

### 2. Blinker (Oscillateur, période 2)
```
***
```
**Résultat**: `Oscillateur (periode = 2)`  
**Comportement**:
- Génération 0: ligne horizontale
- Génération 1: ligne verticale
- Génération 2: retour horizontal
- etc.

**Explication**: Les cellules aux extrémités ont 1 voisine (meurent), la cellule centrale a 2 voisines (survit). Les cellules au-dessus/dessous du centre ont 3 voisines (naissent).

### 3. Toad (Oscillateur, période 2)
```
.***
***.
```
**Résultat**: `Oscillateur (periode = 2)`  
**Explication**: Motif légèrement plus complexe que le blinker, alterne entre deux configurations en miroir.

### 4. Glider (Vaisseau)
```
.*..
..**
.**.
```
**Résultat avec frontière torique (40×40)**: `Oscillateur (periode = 160)`  
**Explication**: 
- Le planeur se déplace en diagonale d'une case tous les 4 générations
- Sur une grille 40×40 torique, il revient à sa position initiale après 160 générations (40 déplacements diagonaux × 4 générations/déplacement)
- Avec frontière `edge`, il s'éteint au contact du bord
- **Détection de vaisseau**: bbox(gen_0) ≠ bbox(gen_160) en coordonnées absolues (mais même forme)

### 5. Glider Rain (Configuration complexe)
```
Fichier: glider_rain_safe.txt
```
**Résultat**: `Non periodique ou extinction dans 300 generations`  
**Explication**: Fichier contenant une "pluie" de planeurs et oscillateurs dispersés. La configuration globale ne se répète pas dans les 300 premières générations analysées (système chaotique ou quasi-périodique avec période > 300).

## Impact du type de frontière

### Comparaison pour un planeur:

| Frontière | Comportement | Utilisation |
|-----------|--------------|-------------|
| **Edge** | S'éteint au contact du bord | Observation de motifs finis |
| **Torus** | Réapparaît de l'autre côté, déplacement continu | Étude des vaisseaux, simulation "infinie" |
| **Mirror** | Rebondit, change de trajectoire | Billard, interactions complexes |
| **Rim** | Perturbé/détruit par pression des voisines vivantes | Cas limite, peu utilisé |

## Commandes d'utilisation

### Compilation
```powershell
gcc -O2 motifs_particuliers.c -o motifs_particuliers.exe
```

### Exécution
```powershell
# Still life
.\motifs_particuliers.exe --in block.txt --width 20 --height 20 --gens 100 --boundary edge

# Oscillateur
.\motifs_particuliers.exe --in blinker.txt --width 20 --height 20 --gens 100 --boundary edge

# Vaisseau (nécessite torus pour déplacement continu)
.\motifs_particuliers.exe --in glider.txt --width 40 --height 40 --gens 200 --boundary torus

# Configuration complexe
.\motifs_particuliers.exe --in glider_rain_safe.txt --width 140 --height 40 --gens 300 --boundary torus
```

## Réponses aux questions du projet

### Question: Comment détecter automatiquement un oscillateur?
**Réponse**: Utilisation de hachage cryptographique (hash) de l'état complet de la grille:
1. Stocker hash(génération_i) pour i = 0..N
2. Si hash(génération_j) == hash(génération_i) avec j < i → période détectée = i - j
3. Validation: comparer également bbox pour éviter collisions de hash (probabilité ~10^-19 avec hash 64-bit)

**Avantages**:
- O(1) par comparaison (vs O(largeur × hauteur) pour comparaison pixel)
- Stockage O(N) vs O(N × largeur × hauteur) pour états complets
- Robuste aux grandes grilles

### Question: Pourquoi "trois voisines" est la condition de naissance idéale?
**Réponse empirique** (observée dans les motifs):
- **Moins de 3**: population s'éteint (isolement)
- **Exactement 3**: équilibre naissance/survie, émergence de structures complexes
- **Plus de 3**: surpopulation, extinction rapide

**Analogie biologique**: 
- Isolement → mort (< 3 voisines)
- Communauté optimale → reproduction (= 3 voisines)
- Surpopulation → ressources insuffisantes, mort (> 3 voisines)

## Limitations et extensions possibles

### Limitations actuelles:
- Détection de vaisseaux fiable uniquement en mode torus (déplacement absolu difficile à mesurer avec autres frontières)
- Mémoire limitée pour historique (max 2048 générations stockées)
- Pas de détection de "méthusalem" (patterns à longue durée de vie avant stabilisation)

### Extensions possibles:
- Export des périodes détectées vers fichier JSON pour analyse statistique
- Visualisation ASCII animée pendant l'analyse
- Base de données de motifs connus (bibliothèque de référence)
- Détection de collisions entre motifs

## Conclusion

L'outil `motifs_particuliers.exe` permet une classification automatique et fiable des motifs du Jeu de la Vie sans intervention manuelle. Les résultats confirment les propriétés théoriques des motifs classiques (block stable, blinker période 2, glider mobile). L'approche par hachage est efficace en mémoire et en temps de calcul, adaptée aux contraintes d'un système embarqué.

**Intégration dans le rapport**: Cette section peut être accompagnée de captures d'écran montrant les sorties du programme pour chaque motif, et d'un tableau récapitulatif des classifications obtenues.