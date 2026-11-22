# Beat Juggling · ANKS Labs

Un VST3 (instrument) pour Ableton qui joue du **passe-passe séquencé** : on pose deux sons sur deux platines, on prépare des séquences de blocs, on lance la lecture dans Ableton, et le plugin enchaîne les passes d'une platine à l'autre, en boucle et calé sur le tempo.

## Le principe
- **Deux platines, Gauche et Droite.** Glisse un son sur une platine, ou clique sur « Charger un son… ». Si l'autre platine est vide, elle reçoit le même son.
- **4 patterns.** Chacun a sa longueur : 1, 2, 4, 8, 16 ou 32 mesures. Un clic sur un pattern l'ouvre pour l'éditer et le fait enchaîner à la fin du pattern en cours, pile dans le temps.
- **Des blocs**, joués l'un après l'autre :
  - **Lecture** : une platine, une zone du son (départ et durée), un sens (avant ou arrière).
  - **Mouvement** : stop, scratch, backspin, démarrage, roll. La durée et le réglage se font au potard. Par défaut, le mouvement part de là où le bloc précédent s'est arrêté (« Suite du bloc »).
- **Pattern pas plein** : la fin est un silence. **Pattern qui déborde** : le dernier bloc est raccourci.

## Le vinyle (forme d'onde)
- Les zones des blocs de la platine sont dessinées. Le bloc sélectionné est en rouge.
- Glisser une zone la déplace, tirer ses bords change le départ ou la durée. Tout est calé sur la grille en doubles-croches.
- Double-clic : place le bloc sur la mesure sous la souris (Maj : sur le temps). Ctrl + clic : place le départ à cet endroit.
- Molette : zoom. Maj + molette : défilement. Clic droit + glisser : déplacer la vue. La barre du bas montre tout le son.
- **Caler le BPM d'un morceau** : Alt + glisser dessine une zone verte. Indique ce qu'elle dure (« Vert = 1 mesure »…), puis clique sur « Caler le BPM ». Le BPM est calculé et le 1er temps est placé au début de la zone. Alt + clic seul place le 1er temps.

## Positions et durées
- Départ : `mesure.temps.double-croche`, par exemple `9.1.1`.
- Durée : `mesures.temps.doubles-croches`, par exemple `1.2.0` = une mesure et demie, `0.1.0` = un temps.
- Double-clic sur la valeur sous un potard pour la taper.

## Compilation
Chaque envoi sur `main` lance la compilation Windows dans GitHub Actions. Le VST3 se télécharge dans l'onglet **Actions**, artefact `BeatJuggling-VST3-Windows`. Copie le dossier `Beat Juggling.vst3` dans `C:\Program Files\Common Files\VST3`.
