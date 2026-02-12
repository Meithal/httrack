TODO
===

- [x] Faire que le CmakeLists genere une APP cocoa
- [x] ... avec des tests
- [ ] ... qui fonctionnent y compris en mode UI
- [x] Avoir une target dependant de httrack ouvrant une fenetre simple
- [x] Avoir une version la plus retrocompatible possible (10.6)
- [x] Avoir une invite d'url qui lance la copie d'un site
- [x] Message d'erreur quand url non trouvée, url vide, etc.
- [ ] Changer le sous titre de la fenetre une fois la copie lancée
- [ ] Avoir un feedback des pages copiées
- [ ] Avoir un bouton pour cacher sidebar <https://developer.apple.com/videos/play/wwdc2020/10104/?time=835> <https://stackoverflow.com/questions/54870957/nssplitviewcontroller-nssplitviewitem-support-in-xibs>
- [x] ... résoudre le bug de double declaration d'enum HTTP_STATUS_OK
- [ ] Avoir un listing de sites copiés
- [ ] Avoir un listing de liens externes référencés par les diverses pages
- [ ] Avoir un aperçu de chaque page copiée
- [ ] Commencer a télécharger le favicon du site pour y associer une icone
- [ ] En proposer une vue arborescente comme dans un arbre FTP
- [ ] S'assurer que le site web puisse s'imprimmer correctement.
- [ ] Faire une maquette avec une grosse barre d'url en haut
- [ ] Ajouter un feedback qui empeche de télécharger tant que l'url n'est pas correcte
- [ ] Poposer un backend via nc pour naviguer sur le site en local et aussi
proposer un site de test pour tester lle la lib fonctionne bien. il y a egalement htsserver.
- [ ] Proposer une alternative plus moderne (storyboard? swift?_) <https://stackoverflow.com/questions/27807951/how-to-embed-a-custom-view-xib-in-a-storyboard-scene>
- [ ] Version téléphone. UIKit ? SwiftUI pour version universelle ?
Optionnel
- [ ] App Clip ? Widget ?
- [ ] Pousser la retrompatibilité derrière big sur ? <https://github.com/devernay/xcodelegacy>
- [ ] Permettre de verifier si les liens externes
ne sont pas en fait des liens internes mais devenus
morts suite a un changement de domaine. Ajouter
une verification de type garde-fu pour les URL
externes tres generiques.
- [ ] Ajouer un drop down d'urls deja entrées dans le passé
- [ ] ... Pouvoir éditer cette liste
- [ ] ... Avoir une option incognito pour ne jamais enregistrer cette info
- [ ] Afficher la date de modification du fichier depuis loop sback->lnk->send_too
- [ ] ... Pouvoir trier les pages du site par ordre de modification
- [ ] Si on cherche Crackman.ttf, rechercher aussi Crackman.TTF et CRACKMAN.TTF
- [ ] Donner une version réparé du site à coté d'une version non touchée

---

- [ ] Avoir un REPL
