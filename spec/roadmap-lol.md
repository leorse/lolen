# Feuille de route : exécutable autonome Lands of Lore

Objectif : partir de ScummVM et n'en garder que le moteur Kyra, réduit à
Lands of Lore: The Throne of Chaos, pour obtenir à terme un exécutable autonome
qui n'est plus ScummVM.

Principe : on réduit le code **par étapes, en gardant un build fonctionnel à
chaque étape** (compiler, lancer le jeu, commiter).

## Règles impératives

- **Conserver les commentaires d'en-tête avec la licence** (bloc GPLv3
  « ScummVM - Graphic Adventure Engine … ») en tête de chaque fichier source
  conservé ou modifié. Un nouveau fichier dérivé de code ScummVM doit reprendre
  le même en-tête.
- Le projet reste sous **GPLv3** : si l'exécutable est distribué, les sources
  doivent l'être aussi. Ne pas utiliser le nom ni la marque « ScummVM » pour le
  produit final.
- **Ne pas modifier les valeurs de l'énumération des ressources** dans
  `engines/kyra/resource/resource.h` : ce sont des index dans `kyra.dat`.
- **Conserver `GI_LOL = 4`** (`engines/kyra/detection.h`), qui est écrit dans les
  en-têtes de sauvegarde, ainsi que l'index de jeu `5` dans `iGameTable`
  (`engines/kyra/resource/staticres.cpp`), qui sert à lire `kyra.dat`.
- `kyra.dat` (`dists/engine-data/kyra.dat`) reste indispensable. Garder
  `devtools/create_kyradat` pour pouvoir le régénérer.

## Environnement de build

- MSYS2, terminal **UCRT64**. Paquets : `make`, `diffutils`,
  `mingw-w64-ucrt-x86_64-pkgconf`, `mingw-w64-ucrt-x86_64-SDL2`,
  `mingw-w64-ucrt-x86_64-zlib`.
- Configuration actuelle :

  ```bash
  ./configure --disable-all-engines --enable-engine=kyra,lol \
    --disable-detection-full --enable-release --disable-debug \
    --disable-cloud --disable-libcurl --disable-sdlnet --disable-enet \
    --disable-discord --disable-tts --disable-taskbar --disable-sparkle \
    --disable-freetype2 --disable-fribidi \
    --disable-jpeg --disable-gif --disable-theoradec --disable-vpx --disable-faad \
    --disable-bink --disable-tinygl --disable-lua \
    --disable-fluidsynth --disable-fluidlite --disable-sonivox \
    --disable-hq-scalers --disable-edge-scalers
  make -j12
  ```

- Relancer `./configure` dès que `engines/kyra/configure.engine` change.

---

## Étape 0 : préparation

- [x] Installer la chaîne de build (MSYS2 UCRT64, SDL2, make, pkgconf, diffutils)
- [x] Compiler un ScummVM qui ne contient que Kyra et LoL
- [x] `git init`, `.gitignore`, commit initial
- [x] Garder une copie de l'exécutable de référence (`saveScumm/`)
- [ ] Isoler la config du build de travail de celle de ScummVM officiel
      (`scummvm.ini` à côté de l'exe pour le mode portable, ou `--config=…`).
      Aujourd'hui les deux partagent `%APPDATA%\ScummVM`.
- [x] Vérifier le lancement direct sans launcher :
      `scummvm.exe -p "E:\scummvm\lol" lol`

## Étape 1 : réduire `engines/kyra` à Lands of Lore

### 1.1 Retirer Eye of the Beholder
- [x] Supprimer les fichiers propres à EoB : moteur, écrans, GUI, automap,
      SegaCD, sons, pilotes `audiomaster2`, `mlalf98`, `capcom98`,
      `pcspeaker_v1`, `segacd`
- [x] Résoudre les `#ifdef ENABLE_EOB` et `ENABLE_LOL`
      (plus aucune occurrence dans le moteur)
- [x] Retirer le sous-moteur `eob` de `configure.engine` et de `module.mk`

### 1.2 Retirer Kyrandia 1, 2 et 3
- [x] Supprimer les fichiers LoK, HoF et MR : moteur, scènes, items, timers,
      animateurs, écrans, GUI, sauvegardes, scripts, séquences, sons, textes,
      VQA
- [x] Supprimer le pathfinder v1 (`scene_v1.cpp`), qui n'est pas utilisé par LoL
- [x] Nettoyer `staticres.cpp` (tables et chargeurs propres à Kyrandia)
- [x] Nettoyer `resource.cpp` (installeur Mac de Kyrandia 1, StuffIt,
      répertoires Kyra3)
- [x] Réduire le debugger à `Debugger` et `Debugger_LoL`
- [x] Réduire les pilotes son (`sound_intern.h`, sélection dans `kyra_v1.cpp`,
      déclencheurs Kyra1)
- [x] Réduire les polices asiatiques à la police chinoise de LoL
- [x] Déplacer `GUI_V2_BUTTON` dans `gui_lol.h` (renommée `GUI_LOL_BUTTON`)
- [x] Retirer la démo non interactive de LoL, qui dépendait du lecteur de
      séquences de Kyrandia 2 (elle affiche maintenant un message d'erreur)

### 1.3 Détection et metaengine
- [x] Réduire `detection_tables.h` aux entrées LoL (43 sur 156)
- [x] `metaengine.cpp` : création directe de `LoLEngine`, options LoL seulement
- [x] `detection.h` : énumération réduite à `GI_LOL = 4`
- [x] Mettre à jour `POTFILES`

### 1.4 Nettoyer les tests par jeu
- [x] Supprimer tous les tests `GI_KYRA1/2/3` et `GI_EOB1/2`
- [x] Simplifier les tests `GI_LOL`, désormais toujours vrais
- [x] Corriger `delete _text` sur un type incomplet (`kyra_v1.cpp`)
- [x] Build sans erreur ni warning, détection du jeu OK

### 1.5 Validation en jeu (manuelle)
Validation en jeu faite après les étapes 1.4 et 1.6 (version CD). La fin du jeu
et la version disquette restent à tester quand ce sera possible.
- [x] Intro et menu principal
- [x] Nouvelle partie, déplacements, portes et leviers
- [x] Combat, sorts, inventaire
- [ ] Sauvegarde et chargement, y compris une ancienne sauvegarde ScummVM
- [ ] Musique AdLib, musique MIDI (GM/MT-32), bruitages, voix (CD)
- [ ] Menu d'options (volumes, texte et voix)
- [ ] Fin du jeu (`LOLFINAL.TIM`) : pas encore de sauvegarde proche de la fin
- [ ] Version disquette (normale et avec installeur compressé)
- [ ] Traduction chinoise (police `FID_CHINESE_FNT`), si disponible

### 1.6 Nettoyage restant dans le moteur
Versions conservées : **DOS disquette** et **DOS CD** uniquement, y compris les
traductions de fans (russe, italien, espagnol, chinois).
- [x] Retirer les versions PC-98, FM-Towns et les démos de la table de détection
      (35 entrées conservées)
- [x] Réduire `GameFlags` : suppression de `platform`, `isDemo`, `useHiRes`,
      `use16ColorMode`, `useHiColorMode`, `useDigSound`, `isOldFloppy`,
      `useAltShapeHeader`, `extraLang`
- [x] Supprimer le pilote son FM-Towns/PC-98 (`sound_pc98_v2.cpp`), le composant
      `fmtowns_pc98_audio` et les types de son associés
- [x] Supprimer le code Amiga, SegaCD et Mac (`AMIGAFont`, `WSAMovieAmiga`,
      `convertAmigaGfx`, double palette, décalage SegaCD, plateformes du clavier…)
- [x] Supprimer le mode 16 couleurs PC-98, les superpositions SJIS, les polices
      SJIS et le japonais
- [x] Supprimer le rendu haute couleur (16/32 bits) et EGA/CGA (`Screen`,
      rendu VCN de `scene_rpg.cpp`)
- [x] Supprimer le code coréen et Johab
- [x] Supprimer `ResLoaderInsMalcolm` (installeur Kyrandia 3) et le flux AUD
      (`audstream.cpp`)
- [x] Supprimer les fonctions devenues inutilisées (détection par analyse des
      relocations des fichiers objets)
- [ ] Nettoyer `KyraEngine_v1` de ce qui ne sert qu'à Kyrandia
      (`_trackMap`, `setHandItem`/`removeHandItem`, opcodes `o1_*` inutilisés…)
- [ ] Envisager de fusionner `KyraEngine_v1`, `KyraRpgEngine` et `LoLEngine`
      (et renommer `EoBFlyingObject`, dont la taille sert de référence à LoL)
- [ ] Supprimer les membres morts déjà présents dans ScummVM (`_processingButtons`,
      `_audioFilename`, `_pageBreakDefault`…)
- [ ] Générer un `kyra.dat` qui ne contient que LoL (ou intégrer les tables
      dans le code)

## Étape 2 : retirer le reste de ScummVM
- [ ] Supprimer les 145 autres dossiers `engines/*`
- [ ] Supprimer les backends autres que SDL et Windows (`backends/platform/*`,
      `cloud`, `networking`, `dlc`, `imgui`, `vkeybd`, `text-to-speech`,
      `updates`, `presence`…)
- [ ] Supprimer `po/`, les `dists/` inutiles (garder `engine-data/kyra.dat`)
      et les `devtools` (garder `create_kyradat`)
- [ ] Retirer de `common/`, `graphics/`, `audio/`, `image/`, `video/` et `math/`
      ce qui n'est plus lié (en se guidant sur les erreurs de link)
- [ ] Décider pour l'émulateur MT-32 (munt) : le garder ou non

## Étape 3 : remplacer l'intégration ScummVM
- [ ] Écrire un `main()` qui crée directement `LoLEngine`, à la place de
      `base/` et du launcher `gui/`
- [ ] Remplacer la détection MD5 (`AdvancedDetector`) par une détection
      simplifiée ou des `GameFlags` fixes
- [ ] Écrire un `OSystem` minimal sur SDL2 ou SDL3 : affichage, palette,
      événements, timer (`getMillis`, `delayMillis`), mixer audio, sauvegardes
- [ ] Remplacer `ConfMan` par une configuration simple (INI à côté de l'exe)
- [ ] Remplacer le keymapper par une gestion directe du clavier
- [ ] Remplacer `GUI::Debugger` et la macro de traduction `_()` par des stubs
- [ ] Définir les chemins propres à l'exe (`lol.ini`, `saves\` à côté de
      l'exe) pour ne plus dépendre de `%APPDATA%\ScummVM`

## Étape 4 : système de build et identité
- [ ] Passer à CMake (retirer `/CMakeLists.txt` du `.gitignore`)
- [ ] Renommer le projet et l'exécutable (sans utiliser « ScummVM »)
- [ ] Intégrer `kyra.dat` dans l'exe ou le livrer à côté
- [ ] Rédiger le README (licence GPLv3, crédits ScummVM, données de jeu requises)
