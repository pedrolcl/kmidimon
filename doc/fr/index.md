% Index de l'aide

# Introduction

[Drumstick MIDI Monitor] (https://kmidimon.sourceforge.io) enregistre les événements à venir 
depuis un port externe MIDI ou une application
via le séquenceur ALSA, ou stocké sous forme de fichiers MIDI standard. Il est
particulièrement utile si vous souhaitez déboguer un logiciel MIDI ou votre configuration MIDI.
Il dispose d'une interface utilisateur graphique agréable, de filtres d'événements personnalisables
et paramètres du séquenceur, prise en charge de tous les messages MIDI et de certains ALSA
messages et en sauvegardant la liste des événements enregistrés dans un fichier texte ou SMF.

# Commencer

## Fenêtre principale

Le programme démarre en état d'enregistrement, enregistrant tous les messages MIDI entrants
événements jusqu'à ce que vous appuyiez sur le bouton d'arrêt. Il y a aussi des boutons pour jouer,
pause, rembobinage et avance, avec le comportement habituel de tout autre média
joueur.

Au-dessus de la grille de la liste des événements, vous pouvez trouver un ensemble d'onglets, un pour chaque
piste définie dans un SMF. Vous pouvez ajouter de nouveaux onglets ou fermer des onglets sans
perdre les événements enregistrés, car ce ne sont que des vues ou des événements
filtres.

Vous pouvez contrôler les connexions MIDI du séquenceur ALSA aux programmes et
périphériques de l'intérieur de Drumstick MIDI Monitor. Pour ce faire, utilisez les options du menu
"Connexions" dans le menu principal. Il existe des options pour se connecter et
déconnectez tous les ports d'entrée disponibles vers Drumstick MIDI Monitor, ainsi qu'une boîte de dialogue
où vous pouvez choisir les ports à surveiller et celui de sortie.

Vous pouvez également utiliser un outil de connexion MIDI comme
[aconnect(1)](https://linux.die.net/man/1/aconnect)
ou [QJackCtl](https://qjackctl.sourceforge.io) pour connecter l'application
ou port MIDI vers Drumstick MIDI Monitor.

Lorsqu'un port MIDI OUT a été connecté au port d'entrée de Drumstick MIDI Monitor dans
l'état d'enregistrement, il affichera les événements MIDI entrants si tout est
correct.

Chaque événement MIDI reçu est affiché sur une seule ligne. Les colonnes ont le
sens suivant.

* **Ticks** : L'heure musicale de l'arrivée de l'événement
* **Temps** : Le temps réel en secondes de l'arrivée de l'événement
* **Source** : l'identifiant ALSA de l'appareil MIDI à l'origine du
    un événement. Vous serez en mesure de reconnaître à quel événement appartient
    appareil si vous en avez plusieurs connectés simultanément. Il y a un
    option de configuration pour afficher le nom du client ALSA au lieu d'un
    numéro
* **Type d'événement** : Le type d'événement : note on/off, control change, ALSA et
    bientôt
* **Chan** Le canal MIDI de l'événement (s'il s'agit d'un événement de canal). Ce
    est également utilisé pour afficher le canal Sysex.
* **Données 1** : Cela dépend du type d'événement reçu. Pour un contrôle
    Changer d'événement ou de note, c'est le numéro de contrôle, ou le numéro de note
* **Données 2** : Cela dépend du type d'événement reçu. Pour un contrôle
    Changer ce sera la valeur, et pour un événement Note ce sera le
    rapidité
* **Données 3** : représentation textuelle des événements exclusifs ou méta.

Vous pouvez masquer ou afficher n'importe quelle colonne à l'aide du menu contextuel. Pour ouvrir ce
menu, appuyez sur le bouton secondaire de la souris sur la liste des événements. Vous pouvez également
utilisez la boîte de dialogue Configuration pour choisir les colonnes visibles.

La liste des événements affiche toujours les événements enregistrés les plus récents au bas de la
la grille.

Drumstick MIDI Monitor peut sauvegarder les événements enregistrés sous forme de fichier texte (au format CSV) ou
un fichier MIDI standard (SMF).

## Options de configuration 

Pour ouvrir la boîte de dialogue de configuration, accédez à l'option de menu Paramètres → Configurer
ou cliquez sur l'icône correspondante dans la barre d'outils.

Ceci est une liste des options de configuration.

* **Onglet Séquenceur**. Les paramètres par défaut de la file d'attente affectent l'heure de l'événement
    précision.
* **Onglet Filtres**. Ici, vous pouvez consulter plusieurs familles d'événements à
    affiché dans la liste des événements.
* **Onglet Affichage**. Le premier groupe de cases à cocher permet d'afficher/masquer les
    colonnes correspondantes de la liste des événements.
* **Divers. languette**. Les options diverses incluent :
    + Traduisez les identifiants des clients ALSA en noms. Si coché, le client ALSA
      les noms sont utilisés à la place des numéros d'identification dans la colonne "Source" pour
      tous les types d'événements et les colonnes de données pour les événements ALSA.
    + Traduisez les messages exclusifs du système universel. Si coché,
      Messages universels exclusifs au système (en temps réel, en temps différé,
      MMC, MTC et quelques autres types) sont interprétés et traduits.
      Sinon, le vidage hexadécimal est affiché.
    + Utilisez une police fixe. Par défaut, Drumstick MIDI Monitor utilise la police système dans le
      liste des événements. Si cette option est cochée, une police fixe est utilisée
      au lieu.

# Crédits et licence

Programme Copyright © 2005-2023 Pedro Lopez-Cabanillas

Documentation Copyright © 2005 Christoph Eckert

Documentation Copyright © 2008-2023 Pedro Lopez-Cabanillas

#Installation

## Comment obtenir Drumstick MIDI Monitor 

[Ici](https://sourceforge.net/projects/kmidimon/files/)
vous pouvez trouver la dernière version. Il y a aussi un miroir Git à
[GitHub](https://github.com/pedrolcl/kmidimon)

## Conditions

Pour utiliser avec succès Drumstick MIDI Monitor, vous avez besoin de Qt 5, Drumstick 2
et les pilotes et la bibliothèque ALSA.

Le système de construction requiert [CMake](http://www.cmake.org) 3.14 ou plus récent.

La bibliothèque ALSA, les pilotes et les utilitaires peuvent être trouvés sur
[Page d'accueil d'ALSA](http://www.alsa-project.org)

Vous pouvez trouver une liste des changements sur https://sourceforge.net/p/kmidimon

## Compilation et installation

Pour compiler et installer Drumstick MIDI Monitor sur votre système, tapez le
suivant dans le répertoire de base de la distribution Drumstick MIDI Monitor :

    $ cmake .
    $ make
    $ make install

Étant donné que Drumstick MIDI Monitor utilise `cmake` et `make` vous ne devriez avoir aucun problème
le compiler. Si vous rencontrez des problèmes, veuillez les signaler au
l'auteur ou le système de suivi des bogues du projet.

