# Beat Juggling · ANKS Labs

Un VST3 (instrument) pour Ableton qui joue du **passe-passe séquencé** : on pose deux sons sur deux platines, on prépare des séquences de blocs, on lance la lecture dans Ableton, et le plugin enchaîne les passes d'une platine à l'autre, en boucle et calé sur le tempo.

## ⬇ TÉLÉCHARGER ET INSTALLER — DOWNLOAD

[![TÉLÉCHARGER LA DERNIÈRE VERSION](https://img.shields.io/github/v/release/olivershadd-star/BEAT-JUGGLING?include_prereleases&sort=date&display_name=tag&style=for-the-badge&logo=github&logoColor=F7FAFF&label=T%C3%89L%C3%89CHARGER&labelColor=10151E&color=EF343E)](../../releases/latest) [![TOUTES LES VERSIONS](https://img.shields.io/badge/TOUTES_LES-VERSIONS-F5C343?style=for-the-badge&labelColor=10151E)](../../releases)

1. Clique sur **TÉLÉCHARGER**, puis, dans la rubrique *Assets*, télécharge `BEAT-JUGGLING-Windows-x64-X.Y.Z.zip` (X.Y.Z = numéro de version : prends toujours le plus grand).
2. Clic droit sur le ZIP → **Extraire tout**.
3. **Plugin pour Ableton** : copie le dossier `VST3\Beat Juggling.vst3` dans `C:\Program Files\Common Files\VST3`.
4. Dans Live : **Préférences → Plug-ins → Dossier système VST3 = On**, puis **Relancer l'analyse**.
5. C'est un **instrument** : pose-le sur une piste MIDI. **Sans Ableton** : lance `Application\Beat Juggling.exe`.

*Windows peut afficher « Windows a protégé votre ordinateur » : clique sur **Informations complémentaires → Exécuter quand même** (programme gratuit non signé, c'est normal).*

**EN —** Click **DOWNLOAD**, get `BEAT-JUGGLING-Windows-x64-X.Y.Z.zip` from *Assets*, extract it and copy `VST3\Beat Juggling.vst3` to `C:\Program Files\Common Files\VST3`, then rescan plug-ins. Or run `Application\Beat Juggling.exe` without any DAW.

---

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
Chaque envoi sur `main` lance la compilation Windows dans GitHub Actions. Quand le numéro de version change dans `CMakeLists.txt` (`project(BEAT_JUGGLING VERSION …)`), le ZIP numéroté est publié tout seul dans **Releases** (voir « Télécharger » plus haut).
