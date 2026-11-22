# Beat Juggling

Un VST3 (instrument) pour Ableton qui automatise le **passe-passe** : deux platines virtuelles A et B jouent la même phrase (ou deux sons différents). Elles se relancent en alternance depuis leur point de départ, suivant un programme rythmique calé sur le tempo d'Ableton.

## Utilisation rapide
1. Pose **Beat Juggling** sur une piste MIDI dans Ableton.
2. Glisse un fichier WAV sur une platine, ou utilise **Charger un son...**. Si la platine B est vide, elle reçoit le même son.
3. Dans la forme d'onde, sélectionne la phrase : une mesure, deux mesures, une demi-mesure...
4. Règle le **Départ A** et le **Départ B** en 1/16. Par exemple, A part sur la grosse caisse (0) et B sur la caisse claire (4).
5. Écris le programme, puis lance la lecture dans Ableton.

## Forme d'onde
- Molette : zoom autour de la souris. Maj + molette : défilement. Clic droit + glisser : déplacer la vue.
- Glisser : sélectionner. Tirer les bords rouges : ajuster la sélection.
- Double-clic : sélectionne la mesure sous la souris (Maj + double-clic : le temps).
- Ctrl + clic : place le départ de la platine. Alt + clic : place le 1er temps de la grille.
- **Caler le BPM** : indique ce que fait ta sélection (1 temps, 1 mesure...), et le plugin calcule le BPM du son.
- La barre du bas affiche tout le fichier. Clique ou glisse dessus pour te déplacer.

## Programme
| Code | Effet |
|---|---|
| `1` `1/2` `3/4` `1.5` | durée d'un segment en temps |
| `x3` | répète le segment, ou le groupe, précédent |
| `3+2` | groupe : 3 temps puis 2 temps |
| `A1` `B1/2` | force la platine (sinon : alternance A, B, A...) |
| `_1` | silence |
| `@4` | départ forcé, en 1/16 depuis le début de la sélection |
| `r` | reverse |
| `s` | stop platine (ralentit jusqu'à l'arrêt) |
| `w` | démarrage platine |
| `k` | backspin |
| `z` | scratch |
| `*4` | roll (rejoue 4 fois le début du segment) |
| `c` | continue (la platine reprend où elle s'était arrêtée) |

Exemple : `1 x3 | 1/2 x5 | 3/2 x3 1s`

**Sync tempo** activé : la boucle est jouée à la vitesse qui la cale sur le tempo d'Ableton, et la hauteur suit, comme un vinyle.

## Compilation
Chaque envoi sur `main` lance la compilation Windows dans GitHub Actions. Le VST3 se télécharge dans l'onglet **Actions**, artefact `BeatJuggling-VST3-Windows`. Copie le dossier `Beat Juggling.vst3` dans `C:\Program Files\Common Files\VST3`.
