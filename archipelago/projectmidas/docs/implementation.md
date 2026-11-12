# Project Midas - GE 007 — implémentation Archipelago

Document technique de ce qui est branché aujourd’hui. Le nom de jeu envoyé au serveur est exactement `Project Midas - GE 007`. Un yaml ou une room encore nommés `GoldenEye 007` ne correspondent plus.

Le monde n’est pas dans la distribution officielle d’Archipelago. On copie `archipelago/projectmidas` dans `custom_worlds/projectmidas`, puis on génère la seed en local. Le dossier s’appelle `projectmidas` pour cohabiter avec un autre monde GoldenEye déjà installé. Le client Windows parle le protocole 0.6.

Les identifiants C et Python sont comparés par `python archipelago/check_ids.py`. Les listes de noms partagées sont marquées `AP_SYNC` dans `port/src/ap_client.c` et doivent rester identiques à `archipelago/projectmidas/data.py`.

## Fichiers

| Rôle | Fichier |
|---|---|
| Identifiants | `port/include/ap_ids.h`, `archipelago/projectmidas/data.py` |
| Génération, logique, slot data | `archipelago/projectmidas/__init__.py` |
| Options yaml | `archipelago/projectmidas/options.py` |
| Client, état, effets | `port/src/ap_client.c`, `port/include/ap_client.h` |
| Réseau | `port/src/ap_net.c` |
| JSON minimal | `port/src/ap_json.c` |
| Login | `port/src/ap_menu.c` |
| Toasts et compteurs | `port/src/ap_popup.c` |
| Gardes et musique | `port/src/ap_enemy.c` |
| Accroches dans le jeu | `src/boss.c`, `src/game/file.c`, `src/game/file2.c`, `src/game/front.c`, `src/game/bondview_r.c`, `src/game/bondview2.c`, `src/game/bondinv.c`, `src/game/propobj.c`, `src/game/prop.c`, `src/game/objective_status.c`, `src/game/chraction.c`, `src/game/chrai.c`, `src/game/chr.c`, `src/game/music_0D2720.c` |

## Protocole

`ap_net.c` ouvre un WebSocket dans un thread. Sous Windows c’est WinHTTP, donc `wss://` si TLS est coché. WinHTTP ne négocie pas `permessage-deflate` : le serveur peut afficher un avertissement, la session continue. Les autres builds parlent `ws://` en sockets POSIX.

La file d’entrée garde 16 messages. Un texte est jeté avant une liste d’objets si la file est pleine. Si l’index d’items du serveur dépasse celui du client, le client envoie `Sync` toutes les deux secondes jusqu’à recevoir une liste complète d’index 0.

Le `Connect` envoie :

- `game`: `Project Midas - GE 007`
- `items_handling`: 7 (inventaire de départ, objets déjà reçus, objets en direct)
- `tags`: `["AP"]`, plus `DeathLink` via `ConnectUpdate` si l’option est allumée
- `slot_data`: true
- la version renvoyée par `RoomInfo`

`StatusUpdate` utilise 20 pour « en train de jouer » et 30 pour le goal. Envoyer 30 à la connexion marquait la room comme déjà gagnée.

Les locations partent en `LocationChecks`. Le goal part en statut 30 quand `goalMet()` devient vrai.

## Identifiants

Vingt missions, index 0 à 19. Dam est 0, Cradle 17, Aztec 18, Egyptian 19. Les dix-huit premières sont la campagne.

| Plage | Contenu |
|---|---|
| `640001 + mission` | Dossier partagé entre les difficultés |
| `640501 + mission * 3 + difficulté` | Dossier d’une seule difficulté |
| `640101 + arme` | 24 armes, dans l’ordre de `WEAPONS` |
| `640201`, `640202`, `640203` | Body Armor, Ammo Cache, Ammo Clip |
| `640301` à `640312` | Camera Alert, Jammed Clip, Slipped on the Ice, Baron Samedi, Greasy Hands, Restart Mission, Tiny Bond, The Living Daylights, I am invisible, DK Mode, Paintball, Single Bullet |
| `640401` à `640409` | Neuf outils de mission |
| `641000 + mission * 10 + difficulté` | Clear de mission. Difficulté 0 Agent, 1 Secret, 2 00 |
| `642000 + mission * 40 + difficulté * 10 + slot` | Objectif. La bande 3 est le check unique « n’importe quelle difficulté » |
| `643000 + index` | Cheat. 0–19 temps cible, 20 Magnum, 21 Laser, 22 Golden Gun |

Les 19 premières armes sont les armes tenues en main, jusqu’au lance-roquettes. Les cinq suivantes sont grenade, mine chronométrée, mine de proximité, mine télécommandée, couteau de lancer.

## Génération de la seed

`ProjectMidasWorld.generate_early` fige la forme de la seed avant de créer les items.

- Aztec ou Egyptian comme goal force l’inclusion des missions bonus.
- `difficulty_checks` choisit l’ensemble des difficultés qui existent comme locations : les trois, ou une seule.
- `goal_difficulty` doit être dans cet ensemble, ou `all`, qui reprend tout l’ensemble. Sinon `OptionError`.
- `mission_tools` off ne gate les outils que si `global_weapons` est on.
- Les règles d’accès sont calculées par `build_locations`.

Une région par mission est reliée au menu. Sans dossiers par difficulté, la règle est « possède le dossier de la mission ». Avec, la règle est « possède au moins un des dossiers de difficulté de cette mission ». Le verrou fin, difficulté par difficulté, est appliqué dans le client, pas seulement par la région.

`create_items` pré-collecte `starting_missions` dossiers de campagne, tirés au hasard, jamais Aztec ni Egyptian. `ensure_open_start` réajuste ce tirage pour qu’une mission de départ reste jouable avec les outils exigés. Le reste des dossiers entre dans la piscine.

Le nombre d’emplacements libres est `len(locations) - len(dossiers restants)`. S’il est négatif, la génération s’arrête : trop de dossiers pour trop peu de checks. Les outils exigés par la logique sont ensuite retirés du filler et mis en progression. Les armes restent utiles, sauf le Golden Gun quand Egyptian est dans la seed et que l’inventaire d’armes est partagé : ce clear l’exige. Elles comptent quand même dans les règles quand une location les demande. Si ces objets ne tiennent pas dans les emplacements libres, la génération s’arrête aussi.

`early_fill` simule un ordre d’obtention. Si aucune ordre ne rend chaque location accessible, et que le Golden Gun manque pour Egyptian, il est pré-collecté. Si ça ne suffit toujours pas, `OptionError`.

Le filler restant est un mélange égal de Body Armor, Ammo Cache et Ammo Clip, plus six Camera Alert, quatre Jammed Clip, quatre Slipped on the Ice, quatre Baron Samedi, quatre Greasy Hands, quatre Restart Mission, quatre Tiny Bond, deux I am invisible, deux DK Mode, deux Paintball et deux Single Bullet. The Living Daylights n’est pas tiré : en solo le drapeau est sur le corps de Bond, que le joueur ne voit pas. L’identifiant et la page AP Debug restent. Body Armor et Ammo Cache sont classés utiles. Ammo Clip reste du filler. Avec `global_weapons`, une arme tenue au hasard parmi les 19 est pré-collectée. Si ce tirage est un lance-grenades ou un lance-roquettes, une arme à feu du pool est marquée `early_items` : le remplissage la place sur un check de sphère 1, elle n’est pas donnée au départ. Un seed solo n’a pas d’autre sphère 1, donc ce tirage est alors une arme à feu. Chaque location exige une arme à feu (`need_gun`) : les explosifs seuls ne rendent aucun clear accessible. `collect` compte aussi cette arme de départ dans l’état de logique. Sans ça, Archipelago ne voit que les objets de progression, le Golden Gun est la seule arme à feu visible, et le remplissage n’a plus aucune case où le poser.

`set_rules` pose sur chaque location un `all_of` (tous requis) et des `any_of` (un groupe suffit). La condition de victoire est `can_reach` des clears du goal :

- Cradle, Aztec ou Egyptian : cette mission, sur chaque difficulté du goal.
- Campaign : Dam à Cradle, sur chaque difficulté du goal.

`fill_slot_data` envoie les entiers lus par le client. Les noms d’options yaml (`agent`, `all`, `grouped`…) sont aussi acceptés.

## Logique des outils

Active seulement si l’inventaire d’armes est partagé et que Mission Tools est off. Sinon le kit de départ du niveau reste donné par le client.

| Mission | Exigence |
|---|---|
| Bunker 1 | Key Analyser et Camera sur les clears. Datathief en plus sur 00 et sur l’objectif de téléchargement |
| Silo | Camera. Plastique sur 00 et sur « planter les bombes » |
| Frigate | Tracker Bug. Bomb Defuser sur Secret, 00 et les deux objectifs de désamorçage |
| Bunker 2 | Watch Magnet sur toute la mission |
| Archives | Watch Magnet pour la boîte noire, et pour les clears Secret et 00. Le clear Agent n’est pas bloqué |
| Train | Watch Laser sur toute la mission |
| Caverns | Timed Mine sur les trois destructions de pompes, et sur les clears Secret et 00 |
| Aztec | Explosive Floppy sur toute la mission, si le bonus est inclus |

Timed Mine est l’arme déjà dans le pool, pas un outil `6404xx`.

Avec l’inventaire partagé, Egyptian et « Defeat Baron Samedi » exigent en plus le Golden Gun. Le ramasser dans le temple ne le met pas dans cet inventaire, et Baron Samedi ne meurt qu’avec. Une seed à un seul check par mission, sans objectifs ni cheats, n’a pas de place pour le pistolet en plus du dossier : il est alors donné au départ.

Si les dossiers ne sont pas séparés par difficulté, les cheats de temps peuvent viser une difficulté qui n’est pas un check de mission. La logique des outils couvre alors les trois difficultés. Si les dossiers sont séparés, une difficulté hors check n’est pas jouable, et son cheat n’est pas dans la seed.

## Client

Tant que `apSeedReady()` est faux, les menus solo (fichiers, dossier, missions, briefing, cheats) sont renvoyés vers l’écran de connexion. Une mission déjà lancée n’est pas coupée.

À la connexion, la liste `checked_locations` du serveur remplace les checks locaux. Un vieux fichier ne peut pas renvoyer un goal déjà enregistré. L’index d’items repart de cette liste.

`apStageStatus` remplace la progression de la cartouche :

- pas connecté, ou pas le dossier : 0, verrouillé
- dossier possédé, pas encore clear sur cette difficulté : 1, débloqué
- clear envoyé : 3, terminé

En mode 007, la difficulté compte comme 00 Agent.

L’état de la partie est dans `Data/Saves/archipelago.txt`, à côté de l’exécutable : uuid, seed, index d’items, checks, dossiers, armes, outils, stocks d’armure et de munitions, cheats. La connexion elle-même est dans `midas.ini`, section `[archipelago]`.

Survie : la ligne du dossier est retirée pour l’instant. `ap survival` ouvre la grille limitée à Facility, Bunker, Archives, Caverns, Egyptian, Statue et Cradle. Un joueur, le setup `Ump_` de l’arène choisie, des gardes posés sur les pads, vague par vague. Ces arènes gardent les pads du solo, alors le graphe de chemin du solo est posé sur le setup multijoueur après vérification que chaque pad visé est encore au même endroit. Ils apparaissent déjà en alerte. Tant qu’ils sont loin, le jeu les renvoie sur le trajet vers Bond, celui qui ouvre les portes ; de près, ils tirent. L’écran garde le numéro de vague et le nombre encore debout. Le corps reste le temps de la chute, puis il est retiré. Pas de check, pas d’objectif, pas de Death Link. Sans arme à feu, un PP7 est prêté pour l’étage. Réessayer depuis l’écran d’échec relance la même arène. Un angle de visée à genoux hors d’un tour complet est ramené dans la table de 32 animations.

Les quatre dossiers de la cartouche sont un fichier par seed, `Data/Saves/ap/<seed>.bin`. Une nouvelle seed ouvre des dossiers vides. La seed déjà en cours au moment de cette séparation reprend les dossiers qui étaient dans `eeprom.bin`. Sans Archipelago, le jeu continue d’utiliser `Data/Saves/eeprom.bin`.

La console tilde accepte `ap` : status, connect, disconnect, host, port, tls, slot, password, popup, place.

## Ce que reçoit le joueur

| Item | Effet |
|---|---|
| Dossier | Débloque la mission, ou une seule difficulté si les dossiers sont séparés |
| Arme | Ajoutée à l’inventaire Archipelago et à la montre si une mission est en cours |
| Outil `6404xx` | Ajouté à la montre. Gardé d’une mission à l’autre et redonné au chargement |
| Body Armor | Stocké. Haut de l’écran, touche par défaut F5. Ne se consomme pas si l’armure est déjà portée. Joue le son de ramassage |
| Ammo Cache | Stocké. Touche par défaut F6. Ajoute 50 munitions de chaque type. Un type qui ne peut pas en porter autant s’arrête à son maximum |
| Ammo Clip | Un chargeur de l’arme en main. Rien si les poings sont équipés. Hors mission, attend le début de la suivante |
| Camera Alert | Alarme |
| Jammed Clip | Le chargeur en main est vidé pendant dix secondes (600 ticks). La réserve n’est pas touchée. Le rechargement automatique est refusé pendant ce temps, puis reprend. Une ligne reste affichée avec le temps qui reste |
| Slipped on the Ice | Le recul d’un impact, sur le côté, sans retirer de vie ni d’armure. Le flash de dégâts joue |
| Baron Samedi | Rire, puis un Baron invincible avec le ZMG. Son script est `GAILIST_STANDARD_GUARD` (la liste globale des gardes), pas la liste d’un garde du niveau, qui le laissait planté. Il court vers Bond et tire dès qu’il le voit. Le spawn autorise une position à l’écran (bit `0x10`). S’il manque des slots, des gardes anonymes sont retirés. Deux réceptions, deux Barons. Hors mission, attend le début de la suivante |
| Greasy Hands | L’arme en main est lancée devant Bond, avec la même trajectoire qu’une mine (`gunThrowItem`). Elle ne quitte la montre que si le lancer a réussi. Les outils restent. L’arme reste possédée : la ramasser la rend, et la mission suivante la redonne. Hors mission, attend le début de la suivante |
| Restart Mission | Recharge le stage par `bossSetLoadedStage`, le même chemin qu’un départ de mission. Un téléport vers le pad de départ pouvait valider la Frigate tout de suite, ou laisser la fin du Train derrière Bond. Une seconde réception pendant le chargement ne relance pas une deuxième fois |
| Tiny Bond | Soixante secondes. Passe par `cheatButtonHandleCheatsTurnedOn(CHEAT_TINY_BOND)`, qui pose le bit lu par `cheatIsActive`, divise la hauteur de caméra et l’échelle du modèle déjà à l’écran. Un rebuild pendant la minute reste petit, parce que `makeonebody` lit ce bit. À la fin, `cheatButtonHandleCheatsTurnedOff` rend la taille. Si le cheat était déjà actif, il le reste. Une seconde réception allonge le délai |
| The Living Daylights | Pas dans le pool. Trente secondes, `ITEM_TOKEN`, drapeau multijoueur sur le corps. En solo ce corps n’est pas dessiné, donc le joueur ne voit que son arme disparaître. Reste testable par AP Debug |
| I am invisible | Boris, invincible, sans arme, `visionrange` à 0 pour qu’il reste sur la boucle d’attente de `GAILIST_STANDARD_GUARD`. Le tirage d’armes se fait dans l’allocation, avant le drapeau invincible, et un slot libéré garde la marque du garde précédent : ce spawn est exclu du tirage. Même réserve de slots que Baron |
| DK Mode | Soixante secondes. `cheatButtonHandleCheatsTurnedOn(CHEAT_DK_MODE)`. S’il était déjà actif, il le reste |
| Paintball | Trois minutes. Même chemin, `CHEAT_PAINTBALL` |
| Single Bullet | Ajoute une balle à la réserve de l’arme en main. Le chargeur n’est pas touché. Rien si ce n’est pas une arme à feu, ou si la réserve est déjà au maximum |

Les pièges minutés (Jammed Clip, Tiny Bond, The Living Daylights, DK Mode, Paintball) gardent une ligne, fleur or, avec le temps qui reste. Elle s’affiche même si les toasts de réception sont coupés. Finir la mission met ce temps en pause : les menus ne le consomment pas, et la mission suivante le reprend. Seul un nouveau seed l’efface.

Les noms d’objets et de lieux des autres jeux viennent du `DataPackage`, demandé jeu par jeu après `Connected`. Sans ça, `PrintJSON` n’a que l’identifiant numérique, et un lieu hors de `641000` s’affichait « location ».

Death Link se bascule sur l’écran Archipelago du menu principal (gauche / droite ou confirmer). Le choix est écrit dans `midas.ini` (`death_link` = `seed`, `on` ou `off`). `seed` suit l’option de la seed. `on` et `off` envoient `ConnectUpdate` tout de suite.

Avec `global_weapons`, la réserve du Golden Gun n’est remplie que si l’arme est possédée. Le ramassage du Golden Gun du temple n’est plus refusé quand la réserve est au maximum, et le prop reste en inventaire pour l’objectif « Recover the golden gun » même si l’arme partagée est bloquée. Control prête les mines télécommandées et la détente pour la mission seulement : l’intro les retirait tant que l’objet Remote Mine n’était pas trouvé.

Les pièges et le clip reçus dans le menu sont mis en file et appliqués quand la mission a tourné assez longtemps (`sSettle` atteint 30).

Les toasts montrent ce que ce slot reçoit, et ce qu’il envoie à quelqu’un d’autre. Pas le journal de la room. Chaque ligne d’objet porte la fleur Archipelago : or pour une progression ou un piège, en couleurs pour un objet utile, gris pour du remplissage. Le drapeau vient du paquet serveur, sinon de l’identifiant local. Durée huit secondes, horloge murale. Coin par défaut en bas à gauche. Réglable dans le login et dans Options, Gameplay, Archipelago.

## Inventaire d’armes partagé

Off par défaut. On, les armes au sol et celles lâchées par les gardes ne donnent que des munitions, sauf si l’arme est déjà dans l’inventaire Archipelago. `apBlockLooseWeapon` est appelé depuis `bondinv.c` et `propobj.c`.

Silver PP7 et Gold PP7 ne sont pas des armes du pool. Un drop de garde ne les ajoute pas. Un cheat déjà débloqué les donne quand même.

Au début de mission, `apApplyLoadout` redonne les armes et les outils possédés. L’intro du niveau ne donne une arme tenue que si elle est possédée (`apAllowIntroItem`, `apEquipIntroItem`). Les outils de script restent donnés si Mission Tools est on. Les jetables de l’intro aussi. Les poings restent.

Chaque type d’arme à feu démarre à 100 balles de réserve, le montant que la cartouche donne à l’arme de départ, plafonné au maximum du type. Grenades, roquettes et mines démarrent à un chargeur. Équiper l’arme charge un chargeur depuis cette réserve.

La montre de la cartouche a 30 cases. `prop.c` en ajoute 48 quand une seed est connectée, pour les 24 armes, les 9 outils, la détente des mines, et des doublons. Sans ça, la GoldenEye Key n’entrait plus une fois les armes reçues.

## Checks

Un clear de mission est envoyé par `boss.c` via `apOnMissionComplete`, seulement si cette difficulté est dans le masque de la seed, sans cheat activé. Le mode 007 compte comme 00.

Un objectif complété passe par `objective_status.c`. La lettre affichée A/B/C change avec la difficulté. Le nom de location est le texte de l’objectif, pas cette lettre. En seed « une seule difficulté », le check est unique et stocké dans la bande 3.

Un cheat de temps est envoyé par `file.c` si le chrono est sous `solo_target_time_array` pour la difficulté jouée, et qu’un cheat n’était pas déjà actif. 007 ne compte pas.

Magnum, Laser et Golden Gun ne se comparent plus à l’état du dossier. Cet état est déjà « terminé » quand le clear Archipelago a été posé, donc la transition n’était jamais vue. Le clear lui-même envoie le check : n’importe quel clear de Cradle, Aztec en Secret ou 00, Egyptian en 00. À la reconnexion, un clear déjà dans la room renvoie le cheat correspondant s’il manquait.

## Dossiers par difficulté

Off : un dossier ouvre les trois difficultés. On : chaque difficulté qui envoie un check a son item, et `apStageStatus` ne débloque que les difficultés dont le dossier est là. Le curseur de sélection de mission s’ouvre sur le premier dossier possédé, pas sur Dam. Chaque photo de Select Mission porte `A`, `SA` et `00` : rouge verrouillé, orange encore à jouer, vert déjà clear. Une difficulté que la seed ne check pas reste grise.

## Goal et outro

Le goal est annoncé une fois, statut 30, toast « Goal complete ».

Next sur l’écran de mission réussie, en seed connectée, ouvre le prochain dossier débloqué et pas encore clear sur la difficulté qui vient d’être jouée. La cinématique de fin (`LEVELID_CUBA`, celle de Next après Cradle hors seed) ne part que si cette mission vient de terminer le goal. Finir Cradle alors que le goal n’est pas atteint ouvre la suite, ou la sélection de mission.

La page Archipelago du dossier, après le choix de sauvegarde, affiche le goal, sa progression, les checks de missions, et les checks d’objectifs et de cheats s’ils sont dans la seed. La photo, le mot OHMSS et les tampons sont cachés pour que le texte tienne sur la feuille. La ligne Multiplayer de GoldenEye n’est plus dans ce dossier.

## Login et options

`MENU_ARCHIPELAGO` demande host, port, slot, mot de passe, TLS, popup, coin, Connect et Play. La souris sélectionne la ligne. Un clic sur un champ le prend pour la frappe. Un clic sur TLS, Popup ou Place bascule. Connect et Play se lancent au clic. Les flèches restent. La musique est celle des menus de dossiers.

Les touches d’armure et de cache se rebindent dans Options, Gameplay, Archipelago. Échap annule, Suppr retire la touche. Les noms de touches sont ceux de SDL, écrits dans `midas.ini`. En mission, la touche est retirée des binds du jeu pour ne pas aussi recharger ou tirer.

Les compteurs restent en haut à gauche pendant une mission solo : quantité, et le nom de la touche.

## Gardes et musique

Le tirage est un hash FNV du nom de seed, du numéro de mission et d’un sel. En groupé, le sel est le même pour toute la mission. En chaotic, le sel est le slot du garde : deux gardes qui partagent un numéro de setup ne tirent plus la même chose, et un garde qui apparaît pendant la mission non plus. Recharger la mission refait le même tirage, parce que les slots se remplissent dans le même ordre.

Ne sont pas touchés : un personnage invincible du setup, un corps qui n’est pas un uniforme de combat, une tête nommée. Scientifiques, civils et alliés à tête unique restent ceux de la cartouche.

- Skins : off, un uniforme pour toute la mission, ou un uniforme par garde.
- Têtes : off, une tête masculine et une tête féminine pour la mission, ou une tête par garde.
- Armes : off, tout le monde pareil, ou chacun le sien. Le tirage remplace aussi l’arme posée dans le setup. Pas de gadgets, pas de couteaux, pas de mines. Le Golden Gun, le Silver PP7 et le Gold PP7 restent dehors sauf si `allow_lethal_weapons` est on. Un seul coup suffit.
- Précision : huit paliers, de presque jamais toucher à très létal. Les personnages nommés gardent la valeur de la cartouche. Un garde sans arme après le tirage en reçoit une.
- Musique : la piste principale de la mission est une autre piste de la cartouche. Les stingers d’action restent.

## Death Link

Off par défaut. On, le tag `DeathLink` est ajouté après le connect. La mort de Bond en mission envoie un `Bounce` avec l’heure, le nom du slot et une cause. La mort de Natalya aussi. Un lien reçu tue Bond une fois la mission assez avancée. Un lien arrivé dans le menu attend ce moment. Une mort causée par un lien ne renvoie pas un second lien.

## Menu de debug

`GE_AP_DEBUG` est une option CMake, off par défaut. On, le dossier a une page AP Debug et la console accepte `ap debug`. Les deux déclenchent les pièges, les stocks, le chargeur et un Death Link. Un build sans l’option n’a ni la page ni la commande. Ce n’est pas une option de seed.

## Limites connues

- Pas de monde officiel sur archipelago.gg tant que le dossier n’y est pas publié. Le nom `Project Midas - GE 007` est celui à enregistrer.
- Le build livré est l’exécutable Windows. Le client non Windows n’a pas le TLS WinHTTP.
- Les menus multijoueur de la cartouche ne sont pas une partie Archipelago. L’entrée a été retirée du dossier solo.
- Une seed déjà générée ne change pas quand le yaml ou le monde change. Il faut recopier le monde et régénérer.
- L’inventaire de la montre agrandi ne s’alloue qu’au chargement de la mission. Une clé reçue pendant qu’il était encore plein apparaît au prochain chargement, pas au milieu de la mission en cours.
