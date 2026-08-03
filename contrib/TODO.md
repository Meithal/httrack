TODO
===

Ongoing
---

- Verifier memory leaks
- Ecrire des tests unitaires
- Ecrire des tests UI
- Mettre des bulles d'aide sur les different controles
- Ecrire une documentation
- Garder le programme suffisamment intuitif pour n'avoir besoin ni de l'un ni de l'autre
- Ameliorer le temps de demarrage

Bugs
---

- [x] 1star.gif plante le cache_add
- [x] Hash coucal plante pour les caracteres superieurs a 128
- [x] Le listing de sites a gauche apparait une fois sur deux, sans bugs lancé par NSZombieObjects
- [x] Le listing fait appraitre grabag deux fois de suite au lieu de teletype.com
- [ ] Il reste des fuites de mémoire
- [ ] Apres avoir cliqué sur STOP, appuyer sur download ne fait rien

Taches
---

- [x] Faire que le CmakeLists genere une APP cocoa
- [x] ... avec des tests
- [x] ... qui fonctionnent y compris en mode UI
- [x] Avoir une target dependant de httrack ouvrant une fenetre simple
- [x] Avoir une version la plus retrocompatible possible (10.6)
- [x] Avoir une invite d'url qui lance la copie d'un site
- [x] Message d'erreur quand url non trouvée, url vide, etc.
- [x] Changer le sous titre de la fenetre une fois la copie lancée
- [x] Empecher que l'UI freeze quand on lance une copie
- [x] ... Griser le bouton pendant que le telechargement est en cours
- [x] ... Mettre un bouton de pause lors du telechargement
- [x] ... Avoir un feedback qu'une copie est en cours
- [ ] ... Quand la fenetre associée est fermée, arrêter le thread de copie (?)
- [ ] ...... Avoir un systeme de background worker qui continue la copie meme quand le programme est arrêté et qui s'y reconnecte quand le programme est redémarré ?
- [x] Afficher hts_stat_struct update
- [ ] ... Afficher les warnings, les infos et les erreurs
- [ ] Autocompletion avec sites deja existants <https://developer.apple.com/library/archive/samplecode/SearchField/Introduction/Intro.html#//apple_ref/doc/uid/DTS10004112-Intro-DontLinkElementID_2>
- [ ] Lorsque le cache est corrompu, on a des erreurs bizarres, et sigbus
- [ ] ... Quitter l'application proprement avec des handlers de sortie
- [ ] Avoir un feedback des pages copiées
- [x] ... Chercher le listing des pages dans httrack plutot que lister l'arborescence
- [ ] ... Afficher une barre d'avancement de telechargement
- [ ] Tester Crackman.TTF si Crackman.ttf n'est pas trouvé
- [ ] Avoir un bouton pour cacher sidebar <https://developer.apple.com/videos/play/wwdc2020/10104/?time=835> <https://stackoverflow.com/questions/54870957/nssplitviewcontroller-nssplitviewitem-support-in-xibs>
- [x] ... résoudre le bug de double declaration d'enum HTTP_STATUS_OK
- [x] Sauvegarder le listing dans le dossier user plutot que l'endroit de l'executable.
- [x] Avoir un listing de sites copiés
- [ ] ... Afficher l'avancement de chaque recopie
- [x] ... Faire en sorte que ce listing se mette a jour en temps reel
- [x] ... Le listing doit se faire sur un thread different
- [ ] Avoir une icone a coté de chaque nom de site (favicon)
- [ ] ... Mettre une option pour desactiver cette option
- [ ] Avoir un apercu de chaque item
- [ ] ... image pour image
- [ ] ...... ouvrir l'image dans apercu
- [ ] ... video pour video
- [ ] ...... pouvoir lancer la video
- [ ] ...... ouvrir la video dans un programme externe (quicktime, vlc...)
- [ ] ... Vebview pour page
- [ ] Avoir un listing de liens externes référencés par les diverses pages
- [ ] Commencer a télécharger le favicon du site pour y associer une icone
- [x] En proposer une vue arborescente comme dans un arbre FTP
- [ ] Montrer le nombre d'octets telechargés par page comme dans le process
monitor app, en vue tableau.
- [ ] S'assurer que le site web puisse s'imprimmer correctement. L'option "imprimer" ou "exporter en PDF" doit donner une vue qui fasse sens ?
- [ ] Faire une maquette avec une grosse barre d'url en haut
- [ ] Ajouter un feedback qui empeche de télécharger tant que l'url n'est pas correcte
- [ ] Poposer un backend via nc pour naviguer sur le site en local et aussi proposer un site de test pour tester lle la lib fonctionne bien. il y a egalement htsserver.
- [ ] Proposer une alternative plus moderne (storyboard? swift?_) <https://stackoverflow.com/questions/27807951/how-to-embed-a-custom-view-xib-in-a-storyboard-scene>
- [ ] Version téléphone. UIKit ? SwiftUI pour version universelle ? Optionnel
- [ ] App Clip ? Widget ?
- [ ] Pousser la retrompatibilité derrière big sur ? <https://github.com/devernay/xcodelegacy>
- [ ] Permettre de verifier si les liens externes ne sont pas en fait des liens internes mais devenus morts suite a un changement de domaine. Ajouter une verification de type garde-fu pour les URL externes tres generiques.
- [ ] Ajouer un drop down d'urls deja entrées dans le passé
- [ ] ... Pouvoir éditer cette liste
- [ ] ... Avoir une option incognito pour ne jamais enregistrer cette info
- [ ] Afficher la date de modification du fichier depuis loop sback->lnk->send_too
- [ ] ... Pouvoir trier les pages du site par ordre de modification
- [ ] Donner une version réparé du site à coté d'une version non touchée
- [ ] ... Si on cherche Crackman.ttf, rechercher aussi Crackman.TTF et CRACKMAN.TTF
- [ ] ... Si on detecte un encodage utf essayer un encodage non utf, et vice versa
- [ ] ... Convertir une url de type file:// vers une url en ligne, sans doute etourderie
- [ ] ... Signaler les modifications faites
- [ ] Afficher en temps reel les liens trouvés sur chaque page
- [ ] Permettre d'afficher la version locale du site d'un clic
- [ ] Implementer le NSPasteboard protocol mpour glisser deposer des sites vers le finder.
- [ ] Dans le listing de pages, pouvoir en modifier ou ajouter pour forcer le telechargement
- [ ] Pouvoir exporter l'arborescence d'un site sous forme de json ou de graphe
- [ ] Afficher une notification bureau quand le site a fini de se télécharger
- [ ] Pouvoir mettre en pause et arrêter un téléchargement
- [ ] Creer une fenetre separée pour lancer un nouveau telechargement ? Peut etre simpliste qui contient un simple textView et a l'aspect d'un crayon
- [ ] Ajouter des settings
- [ ] ... Le dossier où on veut faire le mirroir
- [ ] Avoir une vue simpliste avec juste une barre de progression
- [ ] Pouvoir drag et drop une url (page web safari?) sur la fenetre, ou coller du texte
- [ ] Si on veut telecharger tous les PDF d'une page, separer visiblement les liens d'une certine arborescence d'une autre, et mettre un bouton "telecharger tout d'une arborescence" dans le outliner.
- [ ] Utiliser NSdocument pour representer la copie d'un site? D'un reseau de sites interconnecté ? Permettrait undo/redo et icloud. Avoir un panel de sites visibles dans "fenetres" et pouvoir en ouvrir plusieurs.
- [ ] Pouvoir customiser en profondeur l'apparence du site avant de l'imprimer en pdf, afficher les images d'un dossier en mosaique par exemple.
- [ ] Ajouter un pont vers AppleScript
- [ ] ajouter un service de type texte qui permette de telecharger l'URL selectionnee dans safari ou n'importe ou
- [ ] Afficher une barre de progression dans l'icone comme pour la stack de telechargement de safari
- [ ] Idem pour l'icone de la toolbar qui ouvre un panel
- [ ] Utiliser PaperKit pour montrer comment les pages sont liées entre elles avec
des fleches, en mode Figma, afficher le contenu des pages avec webkit.
- [ ] Faire que le panel de stats soit lié au bouton de la toolbar (qui reste actif
tant que le panel est ouvert), ce bouton doit permettre de "toggle" le panel.
- [ ] Quand dans l'outline on clique sur un fichier html,
afficher la page (dans un panel, ou dans la vue principale?)
- [x] Quand dans l'outline on clique sur une image,
afficher l'image (dans un panel, ou dans la vue principale?)
- [ ] Afficher toutes les images dans un CollectionLayout <https://developer.apple.com/documentation/appkit/nscollectionview>
- [ ] Quand dans l'outline on clique sur un dossier zip,
afficher son contenu (dans un panel, ou dans la vue principale?)
- [x] Dans l'outline, a coté de chaque image en afficher une miniature
- [ ] Utiliser NsWorkBench pour ouvrir safari ou questionner les navigateurs disponibles <https://developer.apple.com/documentation/appkit/nsworkspace>
- [ ] NSSharing service qui compresse le site web et l'envoye par mail & autre.
- [ ] Utiliser le pasteboard pour automatiquement commencer une copie, aussi un service et pouvoir drag@drop

Plus tard
---

Dans l'ordre se debarasser de NSOperation et utiliser pthread, 
adopter KVC pour apple scripting, ce ui permet d'utliser les bindng de
Tiger et Leopard avec leur NSController, puis autoLayout et NSSplitViewController,
implementer CoreData, remettre les NSOperation. Faire une verson tablette
avec UIKit, et une version Catalyst. Utiliser IKImageBrowserView

- [ ] Avoir un REPL (que faire en cas de decouverte de nouveaux liens)
- [ ] Migrer vers grand central dispatch avec la version SwiftUI pour le listing de sites et le telechargement
- [ ] Creer une version Mac Catalyst pour que l'App tourne sur iPad et MacBook
- [ ] Version compatible avec Rhapsody et compilable avec Interface Builder for Windows ?
- [ ] Afficher l'avancée des téléchargements
dans une status bar <https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/StatusBar/StatusBar.html>
- [ ] Imprimer le site recopié, regrouper les pages en use seule vue ?
images séparées ? Version webview allégée (focus lecture).
- [ ] Implementer restoreState pour retablir l'application apres fermeture : <https://developer.apple.com/fr/videos/play/wwdc2026/289>

Doc utilisées
---

- <https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/ProgrammingWithObjectiveC/WorkingwithBlocks/WorkingwithBlocks.html> Bouts de fonction a executer plus tard.
- <https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/Blocks/Articles/00_Introduction.html> doc
- <https://developer.apple.com/library/archive/documentation/General/Conceptual/ConcurrencyProgrammingGuide/OperationObjects/OperationObjects.html> Lancer un block en parellele, l'interrompre, etc.

Docs a utiliser
---

- Mettre en pause telechargement : <https://stackoverflow.com/questions/8113268/how-to-cancel-nsblockoperation>
- Cocoa Bindings : <https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/CocoaBindings/Concepts/HowDoBindingsWork.html>
- Evenements : <https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/EventOverview/EventHandlingBasics/EventHandlingBasics.html> Faire que les boutons du menu fassent quelque chose
- Outline : <https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/OutlineView/OutlineView.html#//apple_ref/doc/uid/10000023i> Pour creer des arbres hierarchiques
- Settings : <https://developer.apple.com/documentation/foundation/accessing-settings-from-your-code?language=objc> acces depuis le code et observation
- Settings : <https://developer.apple.com/documentation/foundation/adding-a-settings-interface-to-your-app?language=objc> ajouter UI
- Mise a jour listing temps reel : <https://developer.apple.com/documentation/foundation/improving-performance-and-stability-when-accessing-the-file-system?language=objc> (File presenter & coordinator)
- Listing multithreadé : <https://developer.apple.com/documentation/foundation/nsfilecoordinator?language=objc>
- NSdocument : <https://developer.apple.com/documentation/appkit/nsdocument?language=objc>

