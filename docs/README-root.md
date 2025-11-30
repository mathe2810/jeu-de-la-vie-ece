# Jeu de la Vie (ECE) — Structure et Guide

## Vue d'ensemble

Projet en C (bit-packed) respectant contraintes embarquées. Ce dépôt contient le programme principal et deux outils autonomes pour le rapport.

---

## Structure du projet

 - `main.c` — Programme principal (ne pas modifier)
 - `motifs_particuliers.c` — Outil d’analyse de motifs (still life / oscillateur / vaisseau)
 - `realtime_perf.c` — Outil de mesure temps réel (moyenne / pire cas / jitter)
 - `block.txt`, `blinker.txt`, `glider.txt`, `toad.txt` — Motifs de test simples
 - `glider_rain_safe.txt`, `glider_rain.txt` — Motifs complexes
 - `MOTIFS_PARTICULIERS.md` — Rapport: motifs particuliers
 - `ETAPE5_TEMPS_REEL.md` — Rapport: contraintes temps réel
 - `Makefile` — Build simple (MinGW/GCC)
 - `.gitignore` — Ignore binaires et artefacts

---

## Prérequis

 - Windows + PowerShell
 - GCC (MinGW-w64) installé (`gcc --version`)

## Build rapide
```powershell
# Construire les outils
make

# (optionnel) Construire le programme principal
make projet.exe
```

Sans Makefile:
```powershell
gcc -O2 motifs_particuliers.c -o motifs_particuliers.exe
gcc -O2 realtime_perf.c -o realtime_perf.exe
```

---

## Utilisation
 - Analyse de motifs:

```powershell
.\motifs_particuliers.exe --in block.txt --width 20 --height 20 --gens 100 --boundary edge
.\motifs_particuliers.exe --in blinker.txt --width 20 --height 20 --gens 100 --boundary edge
.\motifs_particuliers.exe --in glider.txt --width 40 --height 40 --gens 200 --boundary torus
```

 - Mesures temps réel (cible 60 Hz = 16.7 ms):
```powershell
.\realtime_perf.exe --width 320 --height 240 --gens 1000 --boundary torus --in glider_rain_safe.txt
```

## Résultats clés (à intégrer au rapport)
 - Block: Still life, période 1
 - Blinker: Oscillateur, période 2
 - Toad: Oscillateur, période 2
 - Glider (40×40 torus): Retour en 160 générations
 - Performances (320×240, 1000 gén.): moyenne ≈ 1.335 ms, pire ≈ 1.769 ms, jitter ≈ 0.434 ms → OK pour 60 Hz

## Notes de conception
 - Grille bit-packed (`uint64_t`), 1 bit/cellule (efficace mémoire)
 - Frontières: `edge`, `torus`, `mirror`, `rim`
 - Détection périodicité par hachage 64-bit du monde
 - Mesures sans I/O pour éviter biais

## Bonnes pratiques
 - Ne pas versionner les exécutables (`.gitignore` inclus)
 - Garder `main.c` intact; outils séparés pour le rapport
 - Documentations séparées: `MOTIFS_PARTICULIERS.md`, `ETAPE5_TEMPS_REEL.md`

---

## Références
 - Wikipedia: Conway’s Game of Life
 - LifeWiki: catalogue de motifs

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
