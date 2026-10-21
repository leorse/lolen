# Passation : option « layout étendu »

État au 2026-10-21. Rien n'est encore implémenté : cette session a servi à
analyser le code et à choisir une disposition. Aucun fichier source n'a été
modifié.

## Objectif

Ajouter une option (dans le menu de LoLEN) qui change la disposition de
l'interface en jeu :

- **désactivée** : layout standard d'origine, strictement inchangé ;
- **activée** : layout personnalisé, avec une zone de jeu beaucoup plus grande
  (au lieu de la petite fenêtre décalée à droite), l'argent déplacé, les
  flèches de déplacement retirées, l'inventaire et les portraits repensés.

Les règles de la feuille de route restent valables (voir `roadmap-lol.md`) :
le jeu d'origine doit toujours se lancer tel quel, et le moteur Kyra garde ses
noms.

## Constats sur le code

- La vue 3D est une fenêtre fixe de **176×120 px placée en x=112** sur un écran
  de 320×200 en mode indexé. Elle est composée de bitmaps de murs pré-rendus et
  de sprites (monstres, objets) à des échelles prévues pour cette taille. Il n'y
  a pas de rendu 3D recalculable : agrandir la vue revient à **étirer l'image**.
- Les valeurs `112, 0, 176, 120` sont écrites en dur à plus de 50 endroits de
  `engines/kyra/engine/scene_lol.cpp` : défilement fluide, rotations
  (`smoothScroll*`), animations de sorts, zones de clic spéciales
  (`setSpecialSceneButtons`), copies de régions. On les retrouve aussi dans
  `gui_lol.cpp`, `lol.cpp`, `text_lol.cpp` et `script_lol.cpp`.
- Les boutons (flèches, inventaire, portraits) viennent de tables de
  coordonnées propres à LoL. Chacune est à adapter.
- Fichiers à étudier en premier : `engines/kyra/engine/scene_lol.cpp`,
  `engines/kyra/engine/lol.cpp`, `engines/kyra/gui/gui_lol.cpp`,
  `engines/kyra/text/text_lol.cpp`.

## Contraintes décidées

- La vue 3D **garde ses proportions d'origine** (176×120 en 320×200, soit
  environ 352×288 à l'affichage avec correction d'aspect 4:3). Elle est
  seulement agrandie, jamais déformée.
- Les **flèches de déplacement sont retirées** dans tous les layouts :
  déplacement au clavier et au clic sur la vue.
- Principe technique envisagé : la vue continue d'être rendue en 176×120 sur
  une page cachée, puis agrandie à l'affichage. L'interface est dessinée par
  dessus, à la résolution de sortie.

## Maquettes

Page de maquettes (privée, ouvrir avec le compte claude.ai) :
https://claude.ai/artifact/8WhS9oUa6HUfx8fZtKw1av

Les maquettes sont en 4:3 (640×480). Elles utilisent des formes simples à la
place des vrais sprites : elles servent à juger la disposition, pas le rendu.

| Layout | Vue 3D | Interface |
|---|---|---|
| **A. Vue maximale à droite** | 520×424 (×1,5) | colonne de 120 px à droite : 4 portraits empilés, argent, boussole, petite carte. Dialogue en bandeau sous la vue, inventaire de 16 cases en bas |
| **B. Panneau latéral et barre basse** | 440×360 (×1,25) | panneau de 200 px à droite : portraits en grille 2×2, argent et boussole sur une ligne, grande carte 188×166. Dialogue pleine largeur, inventaire de 10 cases plus sac de 2 lignes, place pour les statistiques |
| **C. Plein écran avec superpositions** | 588×480 (×1,67) | bande de 52 px à droite (portraits, argent, boussole). Carte, dialogue et inventaire en overlays translucides. Masque le bas de la vue |

**Recommandation : B** pour une première version. Il garde un vrai panneau
d'interface sans cacher la vue. A est le plus simple à implémenter, C le plus
risqué.

Les dimensions ci-dessus sont celles des maquettes à l'écran. Il faudra les
convertir en coordonnées logiques une fois la résolution de sortie choisie.

## Décisions encore ouvertes

1. **Layout retenu** : A, B ou C (recommandé : B).
2. **Résolution de sortie** : 640×400 (vue ×2, interface nette, recommandé) ou
   rester en 320×200 (vue étirée, plus simple mais plus floue). Cela demande de
   vérifier si le `Screen` de Kyra et la couche d'affichage de LoLEN acceptent un
   framebuffer plus grand.
3. **Graphismes de l'interface** : sprites d'origine uniquement, ou nouveaux
   éléments dessinés (barres, cadres).
4. **Où placer l'option** dans le menu de LoLEN et comment la stocker dans la
   configuration.

## Plan d'implémentation

1. Option « layout étendu » dans le menu et la configuration, sans effet
   visible. Le jeu standard doit rester identique.
2. Rendu agrandi de la vue 3D seule, avec les proportions conservées.
3. Remplacer les `112, 0, 176, 120` écrits en dur par une zone de scène
   configurable (par exemple une structure rectangle lue au même endroit pour
   les deux layouts).
4. Refaire l'interface zone par zone, en validant chaque étape : portraits,
   argent, boussole, carte, dialogue, inventaire.
5. Recalculer les zones de clic (boutons, déplacement à la souris).
6. Vérifier les cas particuliers : sorts, effets d'écran, cinématiques, menu de
   sauvegarde, dialogues du jeu.

À chaque étape : compiler (MSYS2 UCRT64), lancer le jeu, commiter.

## Pour reprendre sur un autre poste

- Récupérer le dépôt (`git pull`) et lire ce fichier.
- Demander d'abord de trancher les décisions ouvertes ci-dessus, puis
  commencer par l'étape 1 du plan.
- Les maquettes sont sur claude.ai. Si le lien ne s'ouvre pas, les trois
  layouts sont décrits dans le tableau ci-dessus.
