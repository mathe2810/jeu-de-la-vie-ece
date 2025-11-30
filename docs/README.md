# Jeu de la vie — Projet C

Ce projet implémente le Jeu de la Vie de Conway en C, avec une représentation mémoire compacte bit-packée, des frontières configurables, et deux outils dédiés aux sections de rapport:

- `motifs_particuliers`: classification de motifs (fixe, oscillateur, vaisseau)
- `realtime_perf`: mesure des contraintes temps réel (moyenne, pire, jitter)

Structure:
- `src/`: sources C (`main.c`, `motifs_particuliers.c`, `realtime_perf.c`)
- `bin/`: exécutables générés
- `patterns/`: fichiers de motifs `.txt`
- `docs/`: documentation (ce fichier, plus sections détaillées)
- `scripts/`: scripts utilitaires (ex: `rain_generator.py`)

Compilation (PowerShell Windows):
```
make
```

Utilisation (exemples):
```
bin\projet.exe --width 80 --height 60 --gens 1000 --boundary torus --target-hz 60 --in patterns\glider.txt
bin\motifs_particuliers.exe --width 40 --height 20 --in patterns\blinker.txt --steps 64 --boundary edge
bin\realtime_perf.exe --width 320 --height 240 --gens 1000 --boundary torus --target-hz 60
```

Sections de rapport:
- Voir `docs/MOTIFS_PARTICULIERS.md` pour la méthodologie et résultats de classification.
- Voir `docs/ETAPE5_TEMPS_REEL.md` pour la méthodologie et résultats de performance temps réel.

Notes de conception:
- Grille bit-packée: 1 bit par cellule (`uint64_t`), deux buffers (grille+surcharge) avec limite totale de 64 KiB.
- Frontières: `edge`, `torus`, `mirror`, `rim`.
- Portabilité Windows: `clock_gettime` shim et `Sleep` pour `msleep_ms`.
- Parsing CLI manuel côté outils; `main` supporte `getopt_long` si disponible.
../README.md