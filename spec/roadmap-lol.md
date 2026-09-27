# Feuille de route : exécutable autonome Lands of Lore

Objectif : partir de ScummVM et n'en garder que le moteur Kyra, réduit à
Lands of Lore: The Throne of Chaos, pour obtenir à terme un exécutable autonome
qui n'est plus ScummVM.

Principe : on réduit le code **par étapes, en gardant un build fonctionnel à
chaque étape** (compiler, lancer le jeu, commiter).

## Règles impératives

- **Nom du projet : LolEN** (Lands of Lore ENgine). C'est un fork de ScummVM
  avec sa propre identité.
  - **Le moteur Kyra garde ses noms d'origine** : dossier `engines/kyra`, noms
    de fichiers, namespace `Kyra`, classes `KyraEngine_v1`, `KyraRpgEngine`,
    `LoLEngine`, etc. On doit toujours pouvoir retrouver l'origine Kyra.
  - **Seules les classes qui remplacent l'infrastructure ScummVM** (point
    d'entrée, launcher, backend/`OSystem`, configuration, sauvegardes,
    évènements…) portent le nom **LolEN**, au moment où elles sont écrites
    (étapes 3 et 4).
- **Le jeu de base doit toujours pouvoir être lancé tel quel**, uniquement par
  émulation logicielle, sans matériel spécifique : graphismes VGA d'origine, son
  AdLib émulé (OPL, intégré, sans fichier externe), MIDI GM/MT-32 et haut-parleur
  PC en option. L'émulateur MT-32 (munt) est conservé ; il demande les ROM MT-32
  fournies par l'utilisateur, mais n'est jamais obligatoire. Les futurs mods
  (musique, graphismes, contenu) sont toujours optionnels et ne doivent jamais
  empêcher le lancement du jeu d'origine.
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
    --disable-hq-scalers --disable-edge-scalers --disable-translation
  make -j12
  ```

- Relancer `./configure` dès que `engines/kyra/configure.engine` change.

---

## Étape 0 : préparation

- [x] Installer la chaîne de build (MSYS2 UCRT64, SDL2, make, pkgconf, diffutils)
- [x] Compiler un ScummVM qui ne contient que Kyra et LoL
- [x] `git init`, `.gitignore`, commit initial
- [x] Garder une copie de l'exécutable de référence (`saveScumm/`)
- [x] Isoler la config du build de travail de celle de ScummVM officiel (fait en 3a)
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
Méthode : la liste des fichiers utiles est calculée à partir du build (sources
compilées et en-têtes listés dans les fichiers de dépendances `.deps/*.d`). Tout
fichier source ni compilé ni inclus est supprimé. Le dépôt passe de 48 942 à
environ 1 430 fichiers suivis.
- [x] Supprimer les 145 autres dossiers `engines/*`
- [x] Supprimer les plateformes autres que SDL/Windows (`backends/platform/*`
      et les sous-plateformes de `backends/platform/sdl`, sauf `win32`)
- [x] Supprimer les sources de backends non compilées (`cloud`, `dlc`, `imgui`,
      `text-to-speech`, `presence`, `networking`, `vkeybd`…). Quelques en-têtes
      de `networking` et `vkeybd` restent, car le launcher les inclut encore.
- [x] Supprimer `po/`, `doc/`, `test/`, `.github/`, les `dists/` des autres
      plateformes et les données des autres moteurs. `dists/engine-data` ne
      garde que `kyra.dat` et les données « core » du launcher.
- [x] Supprimer les `devtools` sauf `create_kyradat`
- [x] Retirer de `common/`, `graphics/`, `audio/`, `image/`, `video/`, `math/`,
      `gui/` et `base/` les sources non compilées
- [x] Retirer du dépôt la copie de worktree `.kilo/worktrees/giddy-curtain`
      commitée par erreur (les fichiers restent sur le disque)
- [x] Émulateur MT-32 (munt, `audio/softsynth/mt32`) : conservé (option, ROM
      fournies par l'utilisateur)
- [ ] Nettoyer `dists/scummvm.rc` (sections de moteurs supprimés, thèmes du
      launcher) : à faire avec l'étape 3 ou 4

Note : l'essentiel de `common/`, `graphics/`, `image/`, `video/` et `gui/` reste
utilisé par le launcher et l'infrastructure ScummVM. Leur réduction se fera à
l'étape 3, quand le `main()` et le backend LolEN remplaceront `base/`, `gui/`
et `backends/`.

## Étape 3 : remplacer l'intégration ScummVM

### 3a. Exe qui lance directement le jeu, sans launcher (fait)
Le backend SDL de ScummVM est conservé ; seuls le démarrage et le launcher sont
remplacés. Le nouveau code est dans `lolen/` (namespace `LolEN`).
- [x] `lolen/main.cpp` : point d'entrée Windows (remplace
      `backends/platform/sdl/win32/win32-main.cpp`)
- [x] `LolEN::Application` (`lolen/application.cpp`) : remplace `base/main.cpp`
      et `base/commandLine.cpp` (supprimés). Il gère la ligne de commande, les
      valeurs par défaut, la config, la détection de la version, le lancement du
      moteur et les messages d'erreur (boîte de message Windows).
- [x] Configuration dans `lolen.ini` et sauvegardes dans `saves\`, à côté de l'exe
      (plus de dépendance à `%APPDATA%\ScummVM` pour la config et les sauvegardes)
- [x] Chemin du jeu : argument de la ligne de commande, sinon `path` dans la
      section `[lol]` de `lolen.ini`, sinon le dossier de l'exe
- [x] Langue : `language` dans `lolen.ini`, sinon celle du système si la version
      la propose, sinon la première trouvée ; la langue retenue est enregistrée
- [x] Launcher supprimé (`gui/launcher.*`, options globales, thèmes du launcher,
      dialogues associés), puis tous les objets compilés mais non liés à l'exe
      (analyse de la carte de l'éditeur de liens)
- [x] `dists/scummvm.rc` nettoyé et aux couleurs de LolEN (thèmes, soundfont,
      shaders d'autres moteurs et fichier « Game Explorer » retirés)
- [x] Traductions de la GUI ScummVM désactivées (`--disable-translation`)
- [x] Plus de bouton « Return to Launcher » dans le menu Ctrl+F5

Ligne de commande : `LandsOfLore.exe [OPTIONS] [DOSSIER_DU_JEU]`, avec `--path=`,
`--language=`, `-f/--fullscreen`, `-w/--windowed`, `--debugflags=`,
`-d/--debuglevel=`, `-h/--help`.

Limites connues (réglées en 3b) :
- le menu ScummVM en jeu (Ctrl+F5) est toujours là, en anglais, avec le thème
  intégré de ScummVM ; c'est aussi le seul écran qui permet de choisir le pilote
  musical (sinon `music_driver` dans `lolen.ini`) ;
- la section générale de `lolen.ini` s'appelle encore `[scummvm]` (nom codé en
  dur dans `ConfigManager`) ;
- le backend Windows de ScummVM crée encore `%APPDATA%\ScummVM` (journaux) ;

### 3b. Backend et infrastructure LolEN
- [ ] Remplacer la détection MD5 (`AdvancedDetector`) par une détection
      simplifiée ou des `GameFlags` fixes
- [ ] Écrire un backend LolEN minimal (classes `LolEN…`) sur SDL2 ou SDL3 :
      affichage, palette, événements, timer (`getMillis`, `delayMillis`), mixer
      audio, sauvegardes
- [ ] Remplacer `ConfMan` par une configuration LolEN simple (`lolen.ini`)
- [ ] Remplacer le keymapper par une gestion directe du clavier
- [ ] Remplacer ou retirer la GUI ScummVM restante : menu Ctrl+F5, `GUI::Debugger`
      (console de debug), macro de traduction `_()`
- [ ] Prévoir un écran d'options LolEN (pilote musical, plein écran…), puisque
      le menu Ctrl+F5 de ScummVM disparaîtra
- [ ] Élaguer ensuite `common/`, `graphics/`, `image/`, `video/`, `gui/`

## Étape 4 : système de build et identité
- [ ] Passer à CMake (retirer `/CMakeLists.txt` du `.gitignore`)
- [x] Nommer l'exécutable `LandsOfLore.exe` (`EXECUTABLE` dans le `Makefile`) ;
      le projet s'appelle LolEN, le moteur Kyra n'est pas renommé
- [x] Icône LolEN (`icons/lolen.ico`, 16 à 256 px, générée depuis
      `saveScumm/icone.ico`), utilisée pour l'exe et la fenêtre ; icônes
      ScummVM `.ico`/`.icns`/`.svg` supprimées (`scummvm.xpm` reste, pour les
      autres systèmes)
- [x] Splash : celui de ScummVM d'abord (conservé, par respect pour le projet
      d'origine), puis celui de LolEN (`lolen/data/splash.bmp`, intégré comme
      ressource `lolen_splash.bmp` dans `dists/scummvm.rc`, affiché par
      `LolEN::showSplashScreen()` pendant 2 s, passable par une touche ou un clic ;
      le splash ScummVM passe de 0,6 s à 1 s, dans `engines/engine.cpp`)
- [x] Informations de version de l'exe (produit LolEN, copyright ScummVM, GPL)
- [x] Traductions de la GUI ScummVM désactivées par défaut dans `configure`
      (plus besoin de `--disable-translation`)
- [ ] Intégrer `kyra.dat` dans l'exe ou le livrer à côté
- [ ] Rédiger le README (licence GPLv3, crédits ScummVM, données de jeu requises)

## Étape 5 : mods (plus tard, toujours optionnels)
Le jeu d'origine reste lançable sans aucun mod (voir « Règles impératives »).
- [ ] Gestion de mods : un dossier de mod qui peut remplacer des fichiers du
      jeu, par-dessus les PAK d'origine (le moteur passe déjà par un système
      d'archives prioritaires, `Resource`)
- [ ] Musique de remplacement en OGG/MP3/FLAC. Le décodeur Vorbis est déjà lié ;
      MP3 (libmad) et FLAC demandent leurs bibliothèques.
- [ ] Graphismes de remplacement
- [ ] Choix du mod au lancement (ligne de commande ou configuration LolEN)
