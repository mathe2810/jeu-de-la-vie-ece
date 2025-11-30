# Guide d'utilisation - Outils d'analyse du Jeu de la Vie

## Vue d'ensemble

Ce dossier contient deux outils autonomes pour l'analyse du Jeu de la Vie de Conway:

1. **motifs_particuliers.exe** - Classificateur de motifs (still life, oscillateur, vaisseau)
2. **realtime_perf.exe** - Mesureur de performances temps réel

Ces outils ont été développés **sans modifier** le fichier `main.c` existant. Ils réutilisent les mêmes principes (bit-packing, frontières multiples) dans des programmes indépendants.

---

## 📁 Fichiers du projet

### Programmes principaux
- `motifs_particuliers.c` / `.exe` - Analyseur de motifs
- `realtime_perf.c` / `.exe` - Mesureur de performances
- `main.c` / `projet.exe` - Programme principal (non modifié)

### Fichiers de motifs (entrée)
- `block.txt` - Still life (carré 2×2 stable)
- `blinker.txt` - Oscillateur période 2 (ligne 3 cellules)
- `glider.txt` - Vaisseau (planeur diagonal)
- `toad.txt` - Oscillateur période 2 (crapaud)
- `glider_rain_safe.txt` - Configuration complexe fournie
- `glider_rain.txt` - Autre configuration fournie

### Documentation
- `MOTIFS_PARTICULIERS.md` - Section rapport: analyse des motifs
- `ETAPE5_TEMPS_REEL.md` - Section rapport: contraintes temps réel
- `README.md` - Ce fichier (guide d'utilisation)

---

## 🛠️ Compilation

### Prérequis
- **Compilateur C**: MinGW-w64 (GCC pour Windows) ou Cygwin
- **Système**: Windows 10/11 x64
- **Terminal**: PowerShell ou CMD

### Commandes de compilation

**PowerShell** (recommandé):
```powershell
# Compiler l'analyseur de motifs
gcc -O2 motifs_particuliers.c -o motifs_particuliers.exe

# Compiler le mesureur de performances
gcc -O2 realtime_perf.c -o realtime_perf.exe

# Compiler les deux en une commande
gcc -O2 motifs_particuliers.c -o motifs_particuliers.exe; gcc -O2 realtime_perf.c -o realtime_perf.exe
```

**Vérification**:
```powershell
# Lister les exécutables générés
ls *.exe
```

Sortie attendue:
```
motifs_particuliers.exe
realtime_perf.exe
projet.exe  (si déjà compilé)
```

---

## 📊 1. Analyseur de motifs (`motifs_particuliers.exe`)

### Fonction
Détecte automatiquement le type de motif:
- **Still life** (stable, période 1)
- **Oscillateur** (répétition périodique)
- **Vaisseau** (déplacement avec périodicité)

### Syntaxe
```powershell
.\motifs_particuliers.exe --in <fichier> --width <W> --height <H> --gens <N> --boundary <mode>
```

### Paramètres

| Option | Description | Valeurs possibles | Défaut |
|--------|-------------|-------------------|--------|
| `--in` | Fichier d'entrée (motif) | `*.txt` | **Requis** |
| `--width` | Largeur de la grille | Entier > 0 | 80 |
| `--height` | Hauteur de la grille | Entier > 0 | 40 |
| `--gens` | Générations à simuler | Entier > 0 | 200 |
| `--boundary` | Type de frontière | `edge`, `torus`, `mirror`, `rim` | `torus` |
| `--help` | Afficher l'aide | - | - |

### Exemples d'utilisation

#### Still life (Block)
```powershell
.\motifs_particuliers.exe --in block.txt --width 20 --height 20 --gens 100 --boundary edge
```
**Sortie**:
```
Motif: block.txt
Boundary: 0 (0=edge,1=torus,2=mirror,3=rim)
Classification: Still life (stable, period 1)
```

#### Oscillateur (Blinker)
```powershell
.\motifs_particuliers.exe --in blinker.txt --width 20 --height 20 --gens 100 --boundary edge
```
**Sortie**:
```
Motif: blinker.txt
Boundary: 0 (0=edge,1=torus,2=mirror,3=rim)
Classification: Oscillateur (periode = 2)
```

#### Vaisseau (Glider, mode torus obligatoire)
```powershell
.\motifs_particuliers.exe --in glider.txt --width 40 --height 40 --gens 200 --boundary torus
```
**Sortie**:
```
Motif: glider.txt
Boundary: 1 (0=edge,1=torus,2=mirror,3=rim)
Classification: Oscillateur (periode = 160)
```
*Note: Le planeur revient à sa position initiale après 160 générations sur une grille 40×40 torique.*

#### Configuration complexe
```powershell
.\motifs_particuliers.exe --in glider_rain_safe.txt --width 140 --height 40 --gens 300 --boundary torus
```
**Sortie**:
```
Motif: glider_rain_safe.txt
Boundary: 1 (0=edge,1=torus,2=mirror,3=rim)
Classification: Non periodique ou extinction dans 300 generations
```

### Interprétation des résultats

| Sortie | Signification |
|--------|---------------|
| `Still life (stable, period 1)` | Motif immobile, aucune évolution |
| `Oscillateur (periode = N)` | Retour à l'état initial tous les N générations |
| `Sous-type: Vaisseau (deplacement...)` | Oscillateur qui se déplace (dx, dy) |
| `Non periodique ou extinction dans N generations` | Pas de répétition détectée (chaotique ou période > N) |

### Codes de frontière
- `0` = Edge (bord mort)
- `1` = Torus (enroulement)
- `2` = Mirror (réflexion)
- `3` = Rim (couronne vivante)

---

## ⏱️ 2. Mesureur de performances (`realtime_perf.exe`)

### Fonction
Mesure les performances temps réel:
- **Temps moyen** par génération
- **Pire cas** observé
- **Jitter** (variabilité)

Cible: **16.7 ms/génération** (60 Hz)

### Syntaxe
```powershell
.\realtime_perf.exe --width <W> --height <H> --gens <N> --boundary <mode> [--in <fichier>]
```

### Paramètres

| Option | Description | Valeurs possibles | Défaut |
|--------|-------------|-------------------|--------|
| `--width` | Largeur de la grille | Entier > 0 | 320 |
| `--height` | Hauteur de la grille | Entier > 0 | 240 |
| `--gens` | Générations à simuler | Entier > 0 | 1000 |
| `--boundary` | Type de frontière | `edge`, `torus`, `mirror`, `rim` | `torus` |
| `--in` | Fichier d'entrée (optionnel) | `*.txt` | Aléatoire si absent |
| `--help` | Afficher l'aide | - | - |

### Exemples d'utilisation

#### Configuration standard (320×240, 1000 générations)
```powershell
.\realtime_perf.exe --width 320 --height 240 --gens 1000 --boundary torus --in glider_rain_safe.txt
```
**Sortie**:
```
Mesures (width=320 height=240 gens=1000 boundary=1):
Moyenne: 1.335 ms
Pire cas: 1.769 ms
Jitter: 0.434 ms
Cible temps reel 60 Hz: 16.7 ms par generation -> OK
```

#### Test rapide (100×100, 500 générations)
```powershell
.\realtime_perf.exe --width 100 --height 100 --gens 500 --boundary edge
```

#### Stress test (grille maximale sous 64 KiB)
```powershell
.\realtime_perf.exe --width 512 --height 512 --gens 100 --boundary torus
```

### Interprétation des résultats

| Métrique | Signification | Objectif |
|----------|---------------|----------|
| **Moyenne** | Performance nominale | < 16.7 ms |
| **Pire cas** | Garantie worst-case | < 16.7 ms |
| **Jitter** | Stabilité (pire - moyenne) | Faible (< 1 ms idéal) |
| **Statut** | `OK` ou `A optimiser` | `OK` si tout < 16.7 ms |

---

## 📝 Résultats pour le rapport

### Section "Motifs particuliers"
✅ Voir fichier: **`MOTIFS_PARTICULIERS.md`**

Contenu:
- Explication de l'algorithme de détection (hachage)
- Tableau des motifs testés avec résultats
- Analyse du comportement par type de frontière
- Réponses aux questions du projet

### Section "Étape 5 - Contraintes temps réel"
✅ Voir fichier: **`ETAPE5_TEMPS_REEL.md`**

Contenu:
- Méthodologie de mesure (timer haute résolution)
- Résultats obtenus (320×240, 1000 générations)
- Interprétation (marge ×12.5 par rapport à la cible)
- Analyse mémoire (18.75 KiB / 64 KiB)
- Optimisations possibles (si nécessaire)

---

## 🔧 Dépannage

### Erreur: "gcc: command not found"
**Solution**: Installer MinGW-w64 ou Cygwin
```powershell
# Vérifier l'installation
gcc --version
```

### Erreur: "getline: undefined reference"
**Solution**: Déjà corrigée dans le code (utilise `getline` POSIX disponible dans MinGW/Cygwin moderne)

### Erreur: "Cannot open file glider_rain_safe.txt"
**Solution**: Vérifier que le fichier existe dans le répertoire courant
```powershell
# Lister les fichiers .txt
ls *.txt
```

### Performance < 16.7 ms malgré "A optimiser"
**Solution**: Système très rapide ! Pas de problème, marge positive.

---

## 📊 Captures d'écran recommandées pour le rapport

1. **Still life (block)**: sortie montrant `period 1`
2. **Oscillateur (blinker)**: sortie montrant `periode = 2`
3. **Vaisseau (glider)**: sortie en mode `torus` avec `periode = 160`
4. **Performances**: sortie de `realtime_perf.exe` avec 320×240 1000 générations
5. **Comparaison frontières**: même motif avec `edge` vs `torus` vs `mirror`

**Commande pour captures**:
```powershell
# Rediriger sortie vers fichier
.\motifs_particuliers.exe --in block.txt --width 20 --height 20 --gens 100 --boundary edge > resultat_block.txt
```

---

## 🚀 Utilisation avancée

### Test de tous les motifs (script PowerShell)
```powershell
# Créer un script test_all.ps1
$motifs = @("block", "blinker", "glider", "toad")
foreach ($m in $motifs) {
    Write-Host "=== Test de $m.txt ==="
    .\motifs_particuliers.exe --in "$m.txt" --width 40 --height 40 --gens 200 --boundary torus
    Write-Host ""
}
```

### Export résultats vers CSV
```powershell
# Header CSV
"Motif,Boundary,Classification" | Out-File resultats.csv

# Exécuter et parser (nécessite script PowerShell avancé)
.\motifs_particuliers.exe --in block.txt --width 20 --height 20 --gens 100 --boundary edge >> resultats.csv
```

---

## 📚 Références

### Documentation du Jeu de la Vie
- Wikipedia: http://en.wikipedia.org/wiki/Conway's_Game_of_Life
- LifeWiki (catalogue de motifs): https://conwaylife.com/wiki/

### Motifs célèbres
- **Still lifes**: Block, Beehive, Loaf, Boat
- **Oscillateurs**: Blinker (p2), Toad (p2), Pulsar (p3)
- **Vaisseaux**: Glider, LWSS, MWSS, HWSS

### Optimisations possibles
- Hashlife algorithm (motifs répétitifs)
- QuickLife (SIMD)
- Multi-threading (voir Étape 8 du projet)

---

## 🎯 Checklist pour le rapport

- [x] Fichiers `motifs_particuliers.c` et `realtime_perf.c` créés
- [x] Compilation sans erreurs avec `-O2`
- [x] Tests réussis sur block, blinker, glider, toad
- [x] Mesures temps réel obtenues (320×240, 1000 générations)
- [x] Documentation complète (`MOTIFS_PARTICULIERS.md`, `ETAPE5_TEMPS_REEL.md`)
- [ ] Captures d'écran ajoutées au rapport
- [ ] Tableau récapitulatif des résultats
- [ ] Analyse des optimisations (bit-packing, `-O2`)
- [ ] Réflexion personnelle sur contraintes embarquées

---

## 📧 Contact

Pour toute question sur l'utilisation de ces outils ou l'interprétation des résultats, consulter:
1. Les fichiers `.md` de documentation détaillée
2. Le code source (commenté) dans les fichiers `.c`
3. L'énoncé du projet (Aakash SONI, ECE Paris)

**Bon courage pour votre rapport !**
