# Roadmap LinScreen

## Besoin produit

LinScreen doit fournir une capture d’écran fiable sur les bureaux Linux
modernes, en particulier sous Wayland, sans sacrifier X11 ni les usages en
ligne de commande. Les livrables doivent démarrer dans la langue de
l’utilisateur, être reconnus par le portail desktop et se comporter de la même
façon qu’ils proviennent d’un paquet Debian, RPM, Flatpak ou AppImage.

Les risques principaux observés sont moins liés au nombre de fonctionnalités
qu’à la reproductibilité : dépendances Wayland incomplètes, métadonnées qui se
désynchronisent, traductions absentes d’un paquet et tests historiques encore
manuels.

## Principes et indicateurs

- Un démarrage Wayland ne produit aucune erreur de plugin Qt ou d’identité
  portail.
- Les catalogues français LinScreen et Qt sont présents dans chaque livrable.
- `org.linscreen.LinScreen` reste identique dans le code, le fichier desktop,
  AppStream, D-Bus, Flatpak et Snap.
- Une régression de CLI ou de packaging bloque la CI avant publication.
- Les limites imposées par les portails sont expliquées à l’utilisateur avec
  une action corrective.

## P0 — Stabiliser le démarrage (terminé)

- Initialiser et charger une seule fois les traducteurs.
- Créer le répertoire des catalogues `.qm` dans l’arbre de build.
- Déclarer `qt6-wayland` dans les paquets Debian/CPack.
- Inclure toutes les variantes `libqwayland*.so` dans l’AppImage.
- Ne publier l’identité desktop à Qt que si son fichier desktop est visible.

Critère d’acceptation : compilation complète et démarrage français sous
`QT_QPA_PLATFORM=wayland` sans les trois avertissements initiaux.

## P1 — Rendre les livraisons vérifiables (en cours)

- Activer CTest et couvrir les commandes CLI sans interface graphique.
- Vérifier automatiquement le catalogue français et les métadonnées générées.
- Valider une installation mise en scène avant de produire les artefacts CI.
- Aligner les versions déclarées dans les formats de paquets.

Critère d’acceptation : `ctest` et le validateur d’installation passent
localement et dans GitHub Actions.

## P2 — Tester réellement Wayland (prochaine étape)

- Ajouter un smoke test dans un compositeur Wayland headless.
- Exécuter des scénarios de portail sur GNOME, KDE Plasma et wlroots.
- Tester les réponses succès, refus, annulation et délai dépassé du portail.
- Vérifier les captures multi-écrans avec facteurs d’échelle mixtes.

Critère d’acceptation : aucune attente infinie, message exploitable pour chaque
échec et géométrie correcte pour les configurations couvertes.

## P3 — Qualité utilisateur (planifié)

- Proposer un diagnostic intégré du backend portal/compositeur.
- Réduire les messages techniques visibles hors mode debug.
- Permettre le rechargement de la langue sans redémarrage.
- Documenter une matrice de compatibilité par bureau et format de paquet.

Critère d’acceptation : un rapport de diagnostic partageable permet d’identifier
le backend, la session, les versions et la cause probable sans données privées.

## P4 — Maintenance (continu)

- Ajouter des tests unitaires autour des chemins, géométries et réponses D-Bus.
- Résorber les contournements temporaires du cycle de vie Qt/Wayland.
- Vérifier les paquets Debian, RPM, Flatpak et AppImage à chaque release.
- Maintenir les traductions prioritaires au-dessus d’un seuil documenté.
