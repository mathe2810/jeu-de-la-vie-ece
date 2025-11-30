# Section Rapport: Étape 5 - Contraintes Temps Réel

## Objectif

Mesurer les performances du moteur du Jeu de la Vie pour vérifier le respect des contraintes temps réel d'un système embarqué. Cible: **16.7 ms par génération** (équivalent à 60 Hz, fréquence de rafraîchissement vidéo standard).

## Métriques mesurées

### 1. Temps moyen par génération
Moyenne arithmétique du temps de calcul sur N générations.
- **Formule**: `T_moyen = Σ(T_i) / N` pour i = 1..N
- **Signification**: Performance nominale attendue en conditions normales

### 2. Pire cas (worst case)
Temps maximum observé parmi toutes les générations.
- **Formule**: `T_pire = max(T_1, T_2, ..., T_N)`
- **Signification**: Garantie de délai pour systèmes temps réel strict (hard real-time)

### 3. Jitter (gigue temporelle)
Écart entre le pire cas et la moyenne.
- **Formule**: `Jitter = T_pire - T_moyen`
- **Signification**: Variabilité du temps d'exécution (stabilité)
  - Jitter faible → comportement prévisible
  - Jitter élevé → pics imprévisibles (cache miss, interruptions OS, etc.)

## Implémentation

### Fichier: `realtime_perf.c`

Programme autonome mesurant les performances sans affichage graphique (overhead minimal).

#### 1. Timer haute résolution

**Windows (MinGW/Cygwin)**:
```c
#include <time.h>
struct timespec t0, t1;
clock_gettime(CLOCK_MONOTONIC, &t0);
next_generation(world);
clock_gettime(CLOCK_MONOTONIC, &t1);
double ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
```

**Précision**: nanoseconde (1e-9 s), résolution typique ~100 ns sur Windows moderne.

**Alternative**: `QueryPerformanceCounter()` (API Windows native) offre précision comparable.

#### 2. Méthodologie de mesure

```
Pour chaque génération i de 1 à N:
    1. Démarrer timer haute résolution
    2. Calculer next_generation() (sans I/O)
    3. Arrêter timer
    4. Enregistrer durée T_i
    5. Mettre à jour T_max si T_i > T_max
    6. Accumuler T_somme += T_i

Après N générations:
    T_moyen = T_somme / N
    Jitter = T_max - T_moyen
```

**Remarque importante**: Pas d'affichage pendant la boucle de mesure pour éviter contamination par I/O (printf() peut prendre 1-10 ms).

## Résultats obtenus

### Configuration testée
- **Grille**: 320 × 240 pixels (76 800 cellules)
- **Générations**: 1000 itérations
- **Frontière**: Torique (mode le plus coûteux en calculs de modulo)
- **Fichier d'entrée**: `glider_rain_safe.txt` (configuration complexe avec multiples motifs)
- **Compilateur**: GCC 13.x avec optimisation `-O2`
- **Plateforme**: Windows x64 (MinGW/Cygwin)

### Résultats bruts

```
Mesures (width=320 height=240 gens=1000 boundary=1):
Moyenne:   1.335 ms
Pire cas:  1.769 ms
Jitter:    0.434 ms
Cible temps réel 60 Hz: 16.7 ms par génération -> OK
```

### Interprétation

| Métrique | Valeur | Statut | Analyse |
|----------|--------|--------|---------|
| **Moyenne** | 1.335 ms | ✅ Excellent | **12.5× plus rapide** que la cible 60 Hz |
| **Pire cas** | 1.769 ms | ✅ Excellent | Marge de sécurité: 16.7 - 1.769 = **14.9 ms** |
| **Jitter** | 0.434 ms | ✅ Faible | Variabilité < 33% de la moyenne (stable) |

**Conclusion**: Le moteur respecte largement la contrainte temps réel. Marge suffisante pour:
- Affichage ASCII (2-5 ms typique pour 320×240)
- I/O fichier (quelques ms par sauvegarde)
- Systèmes multi-tâches (interruptions OS)

### Fréquence maximale atteignable

Avec temps moyen de 1.335 ms par génération:
- **Fréquence théorique max**: 1000 / 1.335 ≈ **749 Hz**
- **Avec pire cas**: 1000 / 1.769 ≈ **565 Hz**

→ Le système peut supporter jusqu'à **9.4× la fréquence cible** (565 Hz / 60 Hz) en conditions dégradées.

## Analyse des performances

### Facteurs contribuant aux bonnes performances

1. **Bit-packing efficace**
   - 64 cellules par uint64_t
   - Opérations sur entiers 64-bit (1 cycle CPU moderne)
   - Cache-friendly: 1 ligne de cache = 64 octets = 512 cellules

2. **Optimisation compilateur (-O2)**
   - Loop unrolling
   - Vectorisation SIMD (SSE2/AVX si disponible)
   - Inlining des fonctions get_cell/set_cell

3. **Pas d'allocation dynamique dans la boucle**
   - Grilles pré-allouées (grid + scratch)
   - Simple swap de pointeurs entre générations

4. **Complexité algorithmique optimale**
   - O(largeur × hauteur) par génération (incompressible)
   - Pas de branches inutiles dans count_neighbors()

### Sources du jitter (0.434 ms)

- **Cache miss**: première itération charge les données en cache L3 → L2 → L1
- **Préemption OS**: scheduler Windows peut interrompre le processus (~0.1-1 ms)
- **Turbo Boost CPU**: fréquence variable selon température/charge
- **Branch predictor**: motifs imprévisibles dans grid aléatoire

### Optimisations possibles (si nécessaire)

Si la cible était plus stricte (ex: 1 ms au lieu de 16.7 ms):

1. **Multi-threading** (Étape 8 bonus)
   - Diviser grille en bandes horizontales
   - 4 threads → gain théorique ×3.5 (overhead ~12%)

2. **SIMD intrinsics**
   - AVX2: traiter 4×uint64_t = 256 cellules par instruction
   - Gain potentiel: ×2 à ×3

3. **Lookup tables**
   - Pré-calculer next_state[voisines][état_actuel]
   - Éliminer branches if/else

4. **Padding de mémoire**
   - Aligner les lignes sur 64 octets (taille ligne cache)
   - Éviter false sharing en multi-thread

## Comparaison avec cibles temps réel standard

| Application | Fréquence cible | Contrainte par frame | Notre performance |
|-------------|----------------|----------------------|-------------------|
| **Vidéo 60 Hz** | 60 FPS | 16.7 ms | ✅ 1.335 ms (marge ×12.5) |
| **Jeux vidéo 120 Hz** | 120 FPS | 8.3 ms | ✅ 1.335 ms (marge ×6.2) |
| **VR 90 Hz** | 90 FPS | 11.1 ms | ✅ 1.335 ms (marge ×8.3) |
| **Contrôle industriel** | 1000 Hz | 1 ms | ⚠️ 1.335 ms (juste au-dessus) |

**Remarque**: Pour applications industrielles critiques (1 kHz), optimisations SIMD + multi-thread nécessaires.

## Commandes d'utilisation

### Compilation
```powershell
gcc -O2 realtime_perf.c -o realtime_perf.exe
```

### Exécution - Configuration standard (320×240)
```powershell
.\realtime_perf.exe --width 320 --height 240 --gens 1000 --boundary torus --in glider_rain_safe.txt
```

### Exécution - Grille plus petite (test rapide)
```powershell
.\realtime_perf.exe --width 100 --height 100 --gens 500 --boundary edge
```

### Exécution - Grille maximale (stress test)
```powershell
# Attention: peut dépasser 64 KiB de mémoire allouée
.\realtime_perf.exe --width 640 --height 480 --gens 100 --boundary torus
```

## Analyse mémoire (vérification contrainte 64 KiB)

### Configuration 320×240
- **Cellules totales**: 320 × 240 = 76 800
- **Bits nécessaires**: 76 800 bits
- **Octets par grille**: 76 800 / 8 = 9 600 octets = 9.375 KiB
- **Deux grilles** (current + scratch): 9.375 × 2 = **18.75 KiB**
- **Marge restante**: 64 - 18.75 = **45.25 KiB** (70% de réserve)

✅ **Contrainte respectée**: mémoire utilisée << 64 KiB limite

### Taille maximale théorique
- **Budget**: 64 KiB = 65 536 octets
- **Pour 2 grilles**: 65 536 / 2 = 32 768 octets par grille
- **Cellules max**: 32 768 × 8 = 262 144 cellules
- **Exemple**: 512 × 512 = 262 144 cellules → **taille maximale exacte**

## Graphique recommandé pour le rapport

Suggérez d'inclure un histogramme montrant:
- Axe X: Numéro de génération (1-1000)
- Axe Y: Temps (ms)
- Points: temps mesuré par génération
- Ligne horizontale rouge: cible 60 Hz (16.7 ms)
- Ligne horizontale verte: temps moyen (1.335 ms)
- Zone grisée: jitter (1.335 - 1.769 ms)

**Outil suggéré**: Excel, Python matplotlib, ou gnuplot pour visualisation.

## Réponse à la question du projet

### Question: Que faire si le programme est trop lent ou trop rapide?

**Si trop lent** (temps > 16.7 ms):
1. **Diagnostiquer**: profiler avec `gprof` ou `valgrind --tool=callgrind`
2. **Optimiser**:
   - Activer `-O3` au lieu de `-O2`
   - Réduire appels à get_cell_boundary() (coûteux en mode torus)
   - Utiliser lookup tables pour règles Conway
   - Multi-threading (si 4+ cœurs disponibles)
3. **Réduire la grille**: 160×120 au lieu de 320×240 (gain ×4 théorique)

**Si trop rapide** (comme notre cas):
1. **Avantage**: marge pour ajouter fonctionnalités (affichage, détection motifs)
2. **Option**: augmenter la grille (640×480 reste sous 64 KiB)
3. **Utilisation**: introduire sleep/vsync pour synchroniser sur 60 Hz réel
   ```c
   double target_ms = 16.7;
   if (elapsed_ms < target_ms) {
       usleep((target_ms - elapsed_ms) * 1000); // attendre le reste
   }
   ```

## Conclusion

Le moteur implémenté surpasse largement les exigences temps réel:
- **×12.5 plus rapide** que la cible 60 Hz en moyenne
- **Jitter faible** (0.434 ms) garantit stabilité
- **Mémoire optimisée**: 18.75 KiB utilisés sur 64 KiB disponibles

Ces performances démontrent l'efficacité de:
- La représentation bit-packed (facteur 64 vs tableau char)
- L'optimisation compilateur `-O2`
- L'absence d'allocations dynamiques dans la boucle critique

**Intégration dans le rapport**: Cette section peut être accompagnée d'un tableau récapitulatif des mesures et d'une réflexion sur les optimisations appliquées (ou non nécessaires vu les marges obtenues).
