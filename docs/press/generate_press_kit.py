#!/usr/bin/env python3
"""Generate localized FairWindSK press-kit images and print brochures."""

from pathlib import Path
from textwrap import dedent

from PIL import Image, ImageDraw, ImageEnhance, ImageFont
from reportlab.lib import colors
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.platypus import (
    Image as PdfImage,
    PageBreak,
    Paragraph,
    SimpleDocTemplate,
    Spacer,
    Table,
    TableStyle,
)


ROOT = Path(__file__).resolve().parents[2]
PRESS = ROOT / "docs" / "press"
UI_IMAGES = {
    "desktop": ROOT / "images" / "fairwindsk-desktop.png",
    "chart": ROOT / "images" / "fairwindsk-freeboardsk.png",
    "instruments": ROOT / "images" / "fairwindsk-kip.png",
    "data": ROOT / "images" / "fairwindsk-mydata-waypoints.png",
}
LOGO = ROOT / "resources" / "images" / "other" / "splash_logo.png"
URL = "github.com/OpenFairWind/FairWindSK"
NAVY = "#071b2b"
CYAN = "#35c3d6"
WHITE = "#ffffff"


LANGUAGES = {
    "english": {
        "code": "en",
        "label": "English",
        "tagline": "Your open marine display. One helm. Every platform.",
        "strap": "Open-source · Touch-first · Signal K connected",
        "cta": "Explore, build and contribute",
        "about": "About FairWindSK",
        "about_body": (
            "FairWindSK is an open-source, touch-first marine Multi-Functional Display and "
            "Signal K application host. Its single-window shell brings instruments, vessel "
            "operations, data tools and embedded web applications together across desktop, "
            "Raspberry Pi, Android and iOS/iPadOS targets."
        ),
        "features_title": "Built for the helm",
        "ui_caption": "Real FairWindSK interface: launcher, charts, instruments and vessel data.",
        "features": [
            ("One coherent display", "Live instruments, alarms, apps and vessel data stay inside a stable landscape workspace."),
            ("Touch-first operation", "Large targets, high contrast and comfort presets support use from dawn through night."),
            ("Open ecosystem", "Signal K connectivity and embedded web applications keep the platform adaptable."),
            ("Cross-platform", "Targets include Linux, Raspberry Pi OS, macOS, Windows, Android 13+ and iOS/iPadOS."),
            ("Operational tools", "Person Over Board, anchor, MyData and compatible autopilot access share the same shell."),
            ("Research ready", "A vessel-edge interface for data crowdsourcing, environmental observation and human-in-the-loop studies."),
        ],
        "audience": "For boat owners, yacht clubs, makers, educators and marine researchers.",
        "disclaimer": "FairWindSK supports onboard workflows; it does not replace certified navigation equipment, seamanship or emergency procedures.",
        "social": {
            "square": "OPEN MARINE DISPLAY",
            "landscape": "ONE HELM. EVERY PLATFORM.",
            "story": "NAVIGATE · CONNECT · EXPLORE",
        },
        "releases": {
            "boat-magazines": {
                "title": "FairWindSK brings an open, touch-first marine display to the modern helm",
                "subtitle": "A cross-platform Signal K application host unifies instruments, safety workflows and onboard apps in one landscape interface.",
                "body": [
                    "OpenFairWind introduces FairWindSK, an open-source marine Multi-Functional Display designed to make connected-boat software easier to use at the helm. Built with C++17 and Qt 6, FairWindSK combines live Signal K instruments, vessel operations, data management and embedded web applications in one consistent, touch-friendly window.",
                    "The interface keeps essential entry points visible while applications run inside the display instead of opening scattered external windows. Operators can return to a paged app launcher, review alarms, work with waypoints, routes, regions, notes, charts, tracks and files through MyData, and access Person Over Board, anchor and compatible autopilot workflows from the shared shell.",
                    "Six comfort presets - Default, Dawn, Day, Sunset, Dusk and Night - adjust the presentation for changing light at sea. Large touch targets, high-contrast text and stable layouts are intended for practical use at normal helm distance. FairWindSK targets Linux, Raspberry Pi OS, macOS, Windows, Android 13 and newer, and iOS/iPadOS, while preserving a single-window marine MFD model.",
                    "Because FairWindSK is open source and built around Signal K, owners, installers and developers can inspect it, adapt it and contribute improvements. It is particularly relevant to sailors assembling an open onboard stack, clubs teaching digital seamanship, and researchers connecting vessel-edge applications to environmental data projects.",
                ],
                "quote": "FairWindSK is designed as the calm, consistent layer between the operator and a growing ecosystem of onboard data and applications.",
            },
            "yacht-clubs": {
                "title": "FairWindSK offers yacht clubs an open platform for connected-boating education",
                "subtitle": "The touch-first marine display can support demonstrations, training, maker projects and club-led experimentation with Signal K.",
                "body": [
                    "OpenFairWind is making FairWindSK available to yacht clubs as an open-source platform for exploring connected navigation, onboard data and marine application design. The software presents Signal K instruments and web applications inside a consistent marine-style shell that is approachable for demonstrations while remaining useful for technical workshops.",
                    "Clubs can use FairWindSK to introduce members to open vessel data, build Raspberry Pi-based test benches, demonstrate waypoint and route resources, and discuss operational flows such as alarms, Person Over Board and anchor monitoring. Its launcher organizes applications into pages and sub-pages, and its comfort presets help maintain readable presentation in daylight and at night.",
                    "The project supports common desktop systems, Raspberry Pi OS, Android 13+ and iOS/iPadOS targets. This breadth makes it suitable for shore-based classroom displays, club laptops and experimental onboard installations. FairWindSK remains a companion interface, not a substitute for approved navigation equipment or established emergency procedures.",
                    "As an open project, it also gives club members a practical route into software contribution, localization, interface testing and documentation. Yacht clubs are invited to try the project, share operational feedback and develop workshops around the open Signal K ecosystem.",
                ],
                "quote": "A club can turn FairWindSK into a shared learning surface: part marine display, part open laboratory and part community project.",
            },
            "academia": {
                "title": "FairWindSK connects vessel-edge interaction with marine data research",
                "subtitle": "The open-source Qt 6 and Signal K platform provides a human-facing layer for Internet of Floating Things and compute-continuum experiments.",
                "body": [
                    "FairWindSK is an open-source vessel-edge interface developed in the context of the DYNAMO and FairWind research lines. It provides a touch-first marine Multi-Functional Display and Signal K application host through which onboard data, local services and operator-facing applications can be presented in a coherent single-window environment.",
                    "The current repository implements the vessel-local shell: Signal K REST and websocket connectivity, application discovery and hosting, reconnect recovery, operational overlays, MyData resource workflows, multilingual coordination and cross-platform runtime paths. It does not itself implement federated-learning orchestration, fleet-wide experiment management or model-training pipelines; instead, it can serve as the operational front end for those surrounding systems.",
                    "This separation makes FairWindSK useful for research on edge-to-cloud marine architectures, citizen sensing, environmental data crowdsourcing, human-in-the-loop analytics and resilient interaction under intermittent connectivity. Researchers can integrate web applications through Signal K while retaining common navigation and safety entry points, comfort-aware presentation and consistent operator workflows.",
                    "FairWindSK targets Linux, Raspberry Pi OS, macOS, Windows, Android 13+ and iOS/iPadOS. The codebase, research contextualization and references are openly available for evaluation, extension and reproducible teaching activities.",
                ],
                "quote": "FairWindSK operationalizes the vessel edge: it is where normalized marine data, local applications and human judgment meet.",
            },
        },
        "social_posts": dedent(
            """\
            # Ready-to-use social posts

            ## LinkedIn / Facebook

            Meet FairWindSK: an open-source, touch-first marine Multi-Functional Display and Signal K application host. Live instruments, vessel operations, MyData and embedded onboard apps come together in one coherent landscape workspace - with comfort presets for changing light and targets from Raspberry Pi to desktop and mobile. Explore the project, try it at your club or lab, and contribute: [link]

            #OpenSource #SignalK #MarineTechnology #Sailing #RaspberryPi #OceanTech

            ## Instagram

            One helm. Every platform. ⛵

            FairWindSK brings instruments, safety workflows, vessel data and Signal K apps into one touch-first marine display. Open source, cross-platform and designed for readable operation from dawn through night.

            Explore: [link]

            #FairWindSK #SignalK #SailingTech #OpenSource #MarineElectronics #ConnectedBoat #OceanTech

            ## X / Mastodon

            FairWindSK is an open-source, touch-first marine display and Signal K app host: instruments, vessel workflows, MyData and embedded apps in one single-window helm interface. Linux, Raspberry Pi OS, macOS, Windows, Android 13+ and iOS/iPadOS. [link] #SignalK #OpenSource

            ## Yacht-club post

            Looking for a practical connected-boating workshop? FairWindSK gives clubs an open platform for Signal K demonstrations, Raspberry Pi test benches, digital-seamanship discussion and member contributions. See the project and workshop possibilities: [link]

            ## Academic post

            FairWindSK provides an open vessel-edge HMI for marine compute-continuum research. The Qt 6/Signal K shell supports application hosting, live data, reconnect recovery, operational overlays and cross-platform experiments while leaving model training and federated orchestration to surrounding research systems. [link]

            ## Suggested image pairing

            - LinkedIn, Facebook and Mastodon: `social/fairwindsk-landscape-1200x630.png`
            - Instagram feed: `social/fairwindsk-square-1080x1080.png`
            - Instagram/Facebook story: `social/fairwindsk-story-1080x1920.png`
            """
        ),
    },
    "french": {
        "code": "fr",
        "label": "Français",
        "tagline": "Votre écran marin ouvert. Une barre. Toutes les plateformes.",
        "strap": "Open source · Tactile · Connecté à Signal K",
        "cta": "Découvrir, construire et contribuer",
        "about": "À propos de FairWindSK",
        "about_body": "FairWindSK est un écran multifonction marin open source et tactile, ainsi qu'un hôte d'applications Signal K. Son interface à fenêtre unique réunit instruments, opérations du bord, gestion des données et applications web intégrées sur ordinateur, Raspberry Pi, Android et iOS/iPadOS.",
        "features_title": "Conçu pour la barre",
        "ui_caption": "Interface FairWindSK réelle: lanceur, cartes, instruments et données du bord.",
        "features": [
            ("Un affichage cohérent", "Instruments, alertes, applications et données du bord restent dans un espace paysage stable."),
            ("Pensé pour le tactile", "Grandes cibles, fort contraste et modes de confort du lever du jour à la nuit."),
            ("Écosystème ouvert", "Signal K et les applications web intégrées gardent la plateforme adaptable."),
            ("Multiplateforme", "Linux, Raspberry Pi OS, macOS, Windows, Android 13+ et iOS/iPadOS."),
            ("Outils opérationnels", "Homme à la mer, mouillage, MyData et accès au pilote compatible dans la même interface."),
            ("Prêt pour la recherche", "Une interface de bord pour la collecte participative et les études homme-dans-la-boucle."),
        ],
        "audience": "Pour plaisanciers, yacht-clubs, makers, enseignants et chercheurs marins.",
        "disclaimer": "FairWindSK assiste les usages du bord; il ne remplace ni les équipements homologués, ni le sens marin, ni les procédures d'urgence.",
        "social": {"square": "ÉCRAN MARIN OUVERT", "landscape": "UNE BARRE. TOUTES PLATEFORMES.", "story": "NAVIGUER · CONNECTER · EXPLORER"},
    },
    "spanish": {
        "code": "es",
        "label": "Español",
        "tagline": "Tu pantalla marina abierta. Un timón. Todas las plataformas.",
        "strap": "Código abierto · Táctil · Conectado a Signal K",
        "cta": "Descubre, crea y contribuye",
        "about": "Acerca de FairWindSK",
        "about_body": "FairWindSK es una pantalla multifunción marina de código abierto y táctil, además de un anfitrión de aplicaciones Signal K. Su interfaz de una sola ventana reúne instrumentos, operaciones a bordo, herramientas de datos y aplicaciones web integradas en ordenadores, Raspberry Pi, Android e iOS/iPadOS.",
        "features_title": "Diseñado para el timón",
        "ui_caption": "Interfaz real de FairWindSK: lanzador, cartas, instrumentos y datos de la embarcación.",
        "features": [
            ("Una pantalla coherente", "Instrumentos, alarmas, aplicaciones y datos permanecen en un espacio horizontal estable."),
            ("Operación táctil", "Controles amplios, alto contraste y modos de confort desde el amanecer hasta la noche."),
            ("Ecosistema abierto", "Signal K y las aplicaciones web integradas mantienen la plataforma adaptable."),
            ("Multiplataforma", "Linux, Raspberry Pi OS, macOS, Windows, Android 13+ e iOS/iPadOS."),
            ("Herramientas operativas", "Persona al agua, fondeo, MyData y piloto compatible en la misma interfaz."),
            ("Preparado para investigar", "Una interfaz de borde para ciencia ciudadana y estudios con humanos en el ciclo."),
        ],
        "audience": "Para navegantes, clubes náuticos, makers, docentes e investigadores marinos.",
        "disclaimer": "FairWindSK ayuda en las tareas a bordo; no sustituye equipos homologados, buena práctica marinera ni procedimientos de emergencia.",
        "social": {"square": "PANTALLA MARINA ABIERTA", "landscape": "UN TIMÓN. TODAS LAS PLATAFORMAS.", "story": "NAVEGAR · CONECTAR · EXPLORAR"},
    },
    "italian": {
        "code": "it",
        "label": "Italiano",
        "tagline": "Il tuo display marino aperto. Un timone. Ogni piattaforma.",
        "strap": "Open source · Touch-first · Connesso a Signal K",
        "cta": "Scopri, costruisci e contribuisci",
        "about": "Informazioni su FairWindSK",
        "about_body": "FairWindSK è un display multifunzione marino open source e touch-first, oltre che un host per applicazioni Signal K. L'interfaccia a finestra singola riunisce strumenti, operazioni di bordo, gestione dei dati e applicazioni web integrate su desktop, Raspberry Pi, Android e iOS/iPadOS.",
        "features_title": "Progettato per il timone",
        "ui_caption": "Interfaccia FairWindSK reale: launcher, carte, strumenti e dati di bordo.",
        "features": [
            ("Un display coerente", "Strumenti, allarmi, applicazioni e dati restano in uno spazio orizzontale stabile."),
            ("Pensato per il touch", "Comandi ampi, alto contrasto e preset comfort dall'alba alla notte."),
            ("Ecosistema aperto", "Signal K e le applicazioni web integrate mantengono la piattaforma adattabile."),
            ("Multipiattaforma", "Linux, Raspberry Pi OS, macOS, Windows, Android 13+ e iOS/iPadOS."),
            ("Strumenti operativi", "Persona a mare, ancora, MyData e autopilota compatibile nella stessa interfaccia."),
            ("Pronto per la ricerca", "Un'interfaccia edge per crowdsourcing ambientale e studi human-in-the-loop."),
        ],
        "audience": "Per diportisti, yacht club, maker, docenti e ricercatori marini.",
        "disclaimer": "FairWindSK supporta le attività di bordo; non sostituisce strumenti omologati, perizia marinaresca o procedure di emergenza.",
        "social": {"square": "DISPLAY MARINO APERTO", "landscape": "UN TIMONE. OGNI PIATTAFORMA.", "story": "NAVIGA · CONNETTI · ESPLORA"},
    },
}


TRANSLATED_RELEASES = {
    "french": [
        ("boat-magazines", "FairWindSK apporte un écran marin ouvert et tactile à la barre moderne", "Une plateforme Signal K multiplateforme réunit instruments, sécurité et applications embarquées.", [
            "OpenFairWind présente FairWindSK, un écran multifonction marin open source conçu pour simplifier l'usage des logiciels du bateau à la barre. Développé en C++17 et Qt 6, il réunit instruments Signal K, opérations du bord, gestion des données et applications web intégrées dans une interface tactile cohérente.",
            "Les fonctions essentielles restent accessibles pendant que les applications s'exécutent dans l'écran, sans multiplier les fenêtres externes. L'opérateur retrouve un lanceur paginé, les alertes, MyData pour les waypoints, routes, régions, notes, cartes, traces et fichiers, ainsi que les flux Homme à la mer, mouillage et pilote automatique compatible.",
            "Six modes de confort - Default, Dawn, Day, Sunset, Dusk et Night - adaptent la présentation à la lumière. FairWindSK cible Linux, Raspberry Pi OS, macOS, Windows, Android 13+ et iOS/iPadOS, tout en conservant un modèle marin à fenêtre unique.",
            "Ouvert et fondé sur Signal K, le projet peut être inspecté, adapté et amélioré par les plaisanciers, installateurs et développeurs.",
        ]),
        ("yacht-clubs", "FairWindSK, une plateforme ouverte pour la formation nautique connectée", "Yacht-clubs et associations peuvent créer démonstrations, ateliers Raspberry Pi et projets Signal K.", [
            "OpenFairWind met FairWindSK à disposition des yacht-clubs comme plateforme open source pour explorer les données du bord, la navigation connectée et la conception d'applications marines.",
            "Les clubs peuvent présenter Signal K, construire des bancs d'essai Raspberry Pi, expliquer waypoints et routes, et discuter des alertes, de l'Homme à la mer et du mouillage dans une interface commune.",
            "Le projet fonctionne sur les principaux systèmes de bureau, Raspberry Pi OS, Android 13+ et iOS/iPadOS. Il convient donc aux salles de formation comme aux installations expérimentales à bord.",
            "FairWindSK reste une interface d'accompagnement et ne remplace ni les équipements de navigation homologués ni les procédures d'urgence. Les clubs sont invités à tester, traduire, documenter et partager leurs retours.",
        ]),
        ("academia", "FairWindSK relie l'interaction à bord à la recherche sur les données marines", "La plateforme ouverte Qt 6 et Signal K fournit une couche humaine pour l'Internet of Floating Things.", [
            "FairWindSK est une interface de bord open source issue des lignes de recherche DYNAMO et FairWind. Elle fournit un écran multifonction tactile et un hôte Signal K pour présenter données, services locaux et applications dans un environnement cohérent.",
            "Le dépôt implémente la couche locale: REST et websocket Signal K, découverte et hébergement d'applications, reprise après reconnexion, overlays opérationnels, MyData, localisation et exécution multiplateforme.",
            "Il n'implémente pas directement l'orchestration de l'apprentissage fédéré, la gestion d'expériences de flotte ou les pipelines d'entraînement; il peut en revanche servir de frontal opérationnel à ces systèmes.",
            "Cette séparation favorise les travaux sur l'edge-to-cloud marin, la science citoyenne, le crowdsourcing environnemental et l'analytique avec humain dans la boucle.",
        ]),
    ],
    "spanish": [
        ("boat-magazines", "FairWindSK lleva una pantalla marina abierta y táctil al timón moderno", "Un anfitrión Signal K multiplataforma reúne instrumentos, seguridad y aplicaciones a bordo.", [
            "OpenFairWind presenta FairWindSK, una pantalla multifunción marina de código abierto diseñada para simplificar el software náutico en el timón. Creada con C++17 y Qt 6, reúne instrumentos Signal K, operaciones de la embarcación, gestión de datos y aplicaciones web integradas.",
            "Las funciones esenciales permanecen accesibles mientras las aplicaciones se ejecutan dentro de la pantalla, sin ventanas externas dispersas. El operador dispone de lanzador paginado, alarmas, MyData para waypoints, rutas, regiones, notas, cartas, tracks y archivos, además de Persona al agua, fondeo y acceso a pilotos compatibles.",
            "Seis modos de confort - Default, Dawn, Day, Sunset, Dusk y Night - adaptan la presentación a la luz. FairWindSK se dirige a Linux, Raspberry Pi OS, macOS, Windows, Android 13+ e iOS/iPadOS.",
            "Al ser abierto y basarse en Signal K, propietarios, instaladores y desarrolladores pueden inspeccionarlo, adaptarlo y contribuir.",
        ]),
        ("yacht-clubs", "FairWindSK ofrece a los clubes náuticos una plataforma abierta de formación", "La pantalla táctil facilita demostraciones, talleres maker y experimentos con Signal K.", [
            "OpenFairWind pone FairWindSK a disposición de los clubes náuticos como plataforma abierta para explorar navegación conectada, datos de la embarcación y diseño de aplicaciones marinas.",
            "Los clubes pueden enseñar Signal K, crear bancos Raspberry Pi, demostrar recursos de rutas y waypoints y debatir alarmas, Persona al agua y fondeo desde una interfaz común.",
            "El proyecto funciona en sistemas de escritorio, Raspberry Pi OS, Android 13+ e iOS/iPadOS, por lo que encaja en aulas, portátiles del club e instalaciones experimentales.",
            "FairWindSK es una interfaz complementaria, no sustituye equipos homologados ni procedimientos de emergencia. Los socios pueden probar, traducir, documentar y contribuir.",
        ]),
        ("academia", "FairWindSK conecta la interacción en el borde con la investigación marina", "La plataforma abierta Qt 6 y Signal K ofrece una capa humana para el Internet of Floating Things.", [
            "FairWindSK es una interfaz de borde abierta desarrollada en el contexto de DYNAMO y FairWind. Proporciona una pantalla multifunción táctil y un anfitrión Signal K para presentar datos, servicios y aplicaciones en un entorno coherente.",
            "El repositorio implementa la capa local: REST y websocket Signal K, descubrimiento y alojamiento de aplicaciones, recuperación de conexión, overlays operativos, MyData, localización y ejecución multiplataforma.",
            "No implementa por sí mismo aprendizaje federado, experimentos de flota ni pipelines de entrenamiento; puede actuar como frontal operativo de esos sistemas externos.",
            "La separación resulta útil para investigar arquitecturas marinas edge-to-cloud, ciencia ciudadana, crowdsourcing ambiental y análisis con intervención humana.",
        ]),
    ],
    "italian": [
        ("boat-magazines", "FairWindSK porta un display marino aperto e touch-first al timone moderno", "Un host Signal K multipiattaforma unisce strumenti, sicurezza e applicazioni di bordo.", [
            "OpenFairWind presenta FairWindSK, un display multifunzione marino open source pensato per rendere il software di bordo più semplice da usare al timone. Sviluppato in C++17 e Qt 6, riunisce strumenti Signal K, operazioni, gestione dati e applicazioni web integrate.",
            "Le funzioni essenziali restano disponibili mentre le applicazioni operano all'interno del display, senza finestre esterne sparse. L'operatore dispone di launcher a pagine, allarmi, MyData per waypoint, rotte, regioni, note, carte, tracce e file, oltre a Persona a mare, ancora e accesso agli autopiloti compatibili.",
            "Sei preset comfort - Default, Dawn, Day, Sunset, Dusk e Night - adattano la leggibilità alla luce. FairWindSK supporta Linux, Raspberry Pi OS, macOS, Windows, Android 13+ e iOS/iPadOS.",
            "Essendo aperto e basato su Signal K, può essere studiato, adattato e migliorato da diportisti, installatori e sviluppatori.",
        ]),
        ("yacht-clubs", "FairWindSK offre agli yacht club una piattaforma aperta per la formazione", "Il display touch-first abilita dimostrazioni, laboratori Raspberry Pi e progetti Signal K.", [
            "OpenFairWind rende FairWindSK disponibile agli yacht club come piattaforma open source per esplorare navigazione connessa, dati di bordo e progettazione di applicazioni marine.",
            "I club possono presentare Signal K, costruire banchi prova Raspberry Pi, mostrare waypoint e rotte e discutere allarmi, Persona a mare e ancora tramite un'interfaccia comune.",
            "Il progetto supporta i principali desktop, Raspberry Pi OS, Android 13+ e iOS/iPadOS: è adatto sia alle aule sia alle installazioni sperimentali.",
            "FairWindSK è uno strumento di supporto e non sostituisce apparati omologati o procedure di emergenza. I soci possono provarlo, tradurlo, documentarlo e contribuire.",
        ]),
        ("academia", "FairWindSK collega l'interazione vessel-edge alla ricerca sui dati marini", "La piattaforma aperta Qt 6 e Signal K offre il livello umano per l'Internet of Floating Things.", [
            "FairWindSK è un'interfaccia vessel-edge open source sviluppata nel contesto delle linee di ricerca DYNAMO e FairWind. Offre un display multifunzione touch-first e un host Signal K per presentare dati, servizi locali e applicazioni in un ambiente coerente.",
            "Il repository implementa il livello locale: REST e websocket Signal K, scoperta e hosting delle applicazioni, ripristino della connessione, overlay operativi, MyData, localizzazione e runtime multipiattaforma.",
            "Non implementa direttamente orchestrazione federated learning, esperimenti di flotta o pipeline di training; può però essere il front end operativo di questi sistemi esterni.",
            "La separazione è utile per ricerca edge-to-cloud marina, citizen science, crowdsourcing ambientale e analisi human-in-the-loop.",
        ]),
    ],
}


SOCIAL_TRANSLATIONS = {
    "french": "Découvrez FairWindSK: un écran multifonction marin open source, tactile et connecté à Signal K. Instruments, opérations du bord, MyData et applications intégrées se retrouvent dans un espace cohérent, du Raspberry Pi au mobile. Explorez et contribuez: [link]",
    "spanish": "Descubre FairWindSK: una pantalla multifunción marina abierta, táctil y conectada a Signal K. Instrumentos, operaciones, MyData y aplicaciones integradas en un espacio coherente, desde Raspberry Pi hasta móvil. Explora y contribuye: [link]",
    "italian": "Scopri FairWindSK: un display multifunzione marino open source, touch-first e connesso a Signal K. Strumenti, operazioni, MyData e applicazioni integrate in un unico spazio, da Raspberry Pi al mobile. Esplora e contribuisci: [link]",
}


def font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont:
    """Load a Unicode-capable system font at the requested size."""
    name = "DejaVuSans-Bold.ttf" if bold else "DejaVuSans.ttf"
    return ImageFont.truetype(name, size)


def fit_text(draw: ImageDraw.ImageDraw, text: str, width: int, start: int) -> ImageFont.FreeTypeFont:
    """Reduce a headline until it fits within the safe width."""
    size = start
    while size > 24:
        candidate = font(size, True)
        if draw.textbbox((0, 0), text, font=candidate)[2] <= width:
            return candidate
        size -= 2
    return font(size, True)


def cover_crop(image: Image.Image, size: tuple[int, int]) -> Image.Image:
    """Crop an image to cover a target rectangle without distortion."""
    target_ratio = size[0] / size[1]
    source_ratio = image.width / image.height
    if source_ratio > target_ratio:
        crop_width = int(image.height * target_ratio)
        left = (image.width - crop_width) // 2
        image = image.crop((left, 0, left + crop_width, image.height))
    else:
        crop_height = int(image.width / target_ratio)
        top = (image.height - crop_height) // 2
        image = image.crop((0, top, image.width, top + crop_height))
    return image.resize(size, Image.Resampling.LANCZOS)


def social_image(language: dict, kind: str, size: tuple[int, int], output: Path) -> None:
    """Create one localized, platform-ready social asset."""
    source_key = {"landscape": "desktop", "square": "instruments", "story": "chart"}[kind]
    base = cover_crop(Image.open(UI_IMAGES[source_key]).convert("RGB"), size)
    base = ImageEnhance.Contrast(base).enhance(1.04)
    overlay = Image.new("RGBA", size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    band_height = int(size[1] * (0.31 if kind != "story" else 0.25))
    top = size[1] - band_height
    draw.rectangle((0, top, size[0], size[1]), fill=(7, 27, 43, 226))
    draw.rectangle((0, top, int(size[0] * 0.13), top + 10), fill=(53, 195, 214, 255))
    margin = int(size[0] * 0.06)
    headline = language["social"][kind]
    headline_font = fit_text(draw, headline, size[0] - margin * 2, int(size[0] * 0.065))
    draw.text((margin, top + int(band_height * 0.18)), headline, font=headline_font, fill=WHITE)
    small = font(max(22, int(size[0] * 0.025)), True)
    draw.text((margin, top + int(band_height * 0.63)), "FairWindSK", font=small, fill=CYAN)
    url_font = font(max(18, int(size[0] * 0.019)))
    url_box = draw.textbbox((0, 0), URL, font=url_font)
    draw.text((size[0] - margin - (url_box[2] - url_box[0]), top + int(band_height * 0.65)), URL, font=url_font, fill=WHITE)
    Image.alpha_composite(base.convert("RGBA"), overlay).convert("RGB").save(output, quality=94)


def press_release(language_name: str, release: dict, output: Path) -> None:
    """Write a publication-ready Markdown press release."""
    language = LANGUAGES[language_name]
    body = "\n\n".join(release["body"])
    text = f"""# {release["title"]}

## {release["subtitle"]}

**FOR IMMEDIATE RELEASE**

**Naples, Italy - 24 July 2026** - {body}

> "{release.get("quote", language["tagline"])}"

## {language["about"]}

{language["about_body"]}

## Media resources

- Project: <https://{URL}>
- Print brochure: [`brochure-fairwindsk-{language["code"]}.pdf`](brochure-fairwindsk-{language["code"]}.pdf)
- Social copy and image guidance: [`social-media-posts.md`](social-media-posts.md)
- High-resolution campaign image: [`social/fairwindsk-landscape-1200x630.png`](social/fairwindsk-landscape-1200x630.png)

## Editorial note

{language["disclaimer"]}
"""
    output.write_text(text, encoding="utf-8")


def localized_releases(language_name: str) -> dict:
    """Return English releases or localized editorial versions."""
    if language_name == "english":
        return LANGUAGES["english"]["releases"]
    releases = {}
    for slug, title, subtitle, body in TRANSLATED_RELEASES[language_name]:
        releases[slug] = {"title": title, "subtitle": subtitle, "body": body}
    return releases


def social_copy(language_name: str) -> str:
    """Return the full English pack or a concise localized platform pack."""
    if language_name == "english":
        return LANGUAGES["english"]["social_posts"].replace("[link]", f"https://{URL}")
    language = LANGUAGES[language_name]
    base = SOCIAL_TRANSLATIONS[language_name]
    return f"""# Ready-to-use social posts - {language["label"]}

## LinkedIn / Facebook

{base}

#OpenSource #SignalK #MarineTechnology #Sailing #RaspberryPi #OceanTech

## Instagram

{language["tagline"]} ⛵

{base}

#FairWindSK #SignalK #SailingTech #OpenSource #MarineElectronics #OceanTech

## X / Mastodon

{base} #SignalK #OpenSource

## Suggested image pairing

- LinkedIn, Facebook and Mastodon: `social/fairwindsk-landscape-1200x630.png`
- Instagram feed: `social/fairwindsk-square-1080x1080.png`
- Instagram/Facebook story: `social/fairwindsk-story-1080x1920.png`
""".replace("[link]", f"https://{URL}")


def brochure(language_name: str, output: Path) -> None:
    """Build a polished two-page A4 brochure."""
    language = LANGUAGES[language_name]
    styles = getSampleStyleSheet()
    title = ParagraphStyle("TitleFW", parent=styles["Title"], fontName="Helvetica-Bold", fontSize=25, leading=29, textColor=colors.HexColor(NAVY), alignment=TA_LEFT, spaceAfter=5 * mm)
    h2 = ParagraphStyle("H2FW", parent=styles["Heading2"], fontName="Helvetica-Bold", fontSize=15, leading=18, textColor=colors.HexColor(NAVY), spaceAfter=2.5 * mm)
    body = ParagraphStyle("BodyFW", parent=styles["BodyText"], fontName="Helvetica", fontSize=9.3, leading=13, textColor=colors.HexColor("#183545"))
    feature_title = ParagraphStyle("FeatureTitle", parent=body, fontName="Helvetica-Bold", fontSize=10, leading=12, textColor=colors.HexColor(NAVY))
    feature_body = ParagraphStyle("FeatureBody", parent=body, fontSize=8.5, leading=11.5)
    small = ParagraphStyle("SmallFW", parent=body, fontSize=7.4, leading=9.4, textColor=colors.HexColor("#47616f"))
    doc = SimpleDocTemplate(str(output), pagesize=A4, rightMargin=15 * mm, leftMargin=15 * mm, topMargin=14 * mm, bottomMargin=14 * mm, title=f"FairWindSK brochure - {language['label']}", author="OpenFairWind Project")
    story = []
    story.append(PdfImage(str(LOGO), width=34 * mm, height=25.2 * mm))
    story.append(Paragraph(language["tagline"], title))
    story.append(Paragraph(language["strap"], ParagraphStyle("Strap", parent=body, fontName="Helvetica-Bold", fontSize=11, textColor=colors.HexColor(CYAN), spaceAfter=5 * mm)))
    hero = PdfImage(str(UI_IMAGES["desktop"]), width=180 * mm, height=105.3 * mm)
    story.append(hero)
    story.append(Spacer(1, 3 * mm))
    gallery = Table(
        [[
            PdfImage(str(UI_IMAGES["chart"]), width=58 * mm, height=33.9 * mm),
            PdfImage(str(UI_IMAGES["instruments"]), width=58 * mm, height=33.9 * mm),
            PdfImage(str(UI_IMAGES["data"]), width=58 * mm, height=33.8 * mm),
        ]],
        colWidths=[60 * mm, 60 * mm, 60 * mm],
    )
    gallery.setStyle(TableStyle([
        ("ALIGN", (0, 0), (-1, -1), "CENTER"),
        ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
        ("LEFTPADDING", (0, 0), (-1, -1), 0),
        ("RIGHTPADDING", (0, 0), (-1, -1), 0),
        ("TOPPADDING", (0, 0), (-1, -1), 0),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 0),
    ]))
    story.append(gallery)
    story.append(Spacer(1, 1.5 * mm))
    story.append(Paragraph(language["ui_caption"], small))
    story.append(Spacer(1, 3 * mm))
    story.append(Paragraph(language["about"], h2))
    story.append(Paragraph(language["about_body"], body))
    story.append(Spacer(1, 4 * mm))
    story.append(Paragraph(language["audience"], ParagraphStyle("Audience", parent=body, fontName="Helvetica-Bold", fontSize=11, leading=15, textColor=colors.HexColor(NAVY))))
    story.append(PageBreak())
    story.append(Paragraph(language["features_title"], title))
    cells = []
    for feature_name, description in language["features"]:
        cells.append([Paragraph(feature_name, feature_title), Spacer(1, 1.2 * mm), Paragraph(description, feature_body)])
    table = Table([[cells[0], cells[1]], [cells[2], cells[3]], [cells[4], cells[5]]], colWidths=[86 * mm, 86 * mm], rowHeights=[40 * mm, 40 * mm, 40 * mm])
    table.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, -1), colors.HexColor("#eef6f8")),
        ("BOX", (0, 0), (-1, -1), 0.6, colors.HexColor("#aacbd2")),
        ("INNERGRID", (0, 0), (-1, -1), 0.6, colors.HexColor("#aacbd2")),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 5 * mm),
        ("RIGHTPADDING", (0, 0), (-1, -1), 5 * mm),
        ("TOPPADDING", (0, 0), (-1, -1), 5 * mm),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 5 * mm),
    ]))
    story.append(table)
    story.append(Spacer(1, 7 * mm))
    story.append(Paragraph(language["cta"], h2))
    cta_table = Table([[Paragraph(f"<b>https://{URL}</b>", body), PdfImage(str(LOGO), width=30 * mm, height=22.3 * mm)]], colWidths=[140 * mm, 32 * mm])
    cta_table.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "MIDDLE"), ("LINEABOVE", (0, 0), (-1, 0), 2, colors.HexColor(CYAN)), ("TOPPADDING", (0, 0), (-1, -1), 5 * mm)]))
    story.append(cta_table)
    story.append(Spacer(1, 5 * mm))
    story.append(Paragraph(language["disclaimer"], small))

    def footer(canvas, document):
        canvas.saveState()
        canvas.setFillColor(colors.HexColor(NAVY))
        canvas.rect(0, 0, A4[0], 8 * mm, fill=1, stroke=0)
        canvas.setFillColor(colors.white)
        canvas.setFont("Helvetica", 7)
        canvas.drawString(15 * mm, 3 * mm, "OpenFairWind Project")
        canvas.drawRightString(A4[0] - 15 * mm, 3 * mm, f"{document.page}")
        canvas.restoreState()

    doc.build(story, onFirstPage=footer, onLaterPages=footer)


def main() -> None:
    """Generate every localized deliverable in a stable directory structure."""
    for language_name, language in LANGUAGES.items():
        directory = PRESS / language_name
        social = directory / "social"
        social.mkdir(parents=True, exist_ok=True)
        releases = localized_releases(language_name)
        for slug, release in releases.items():
            press_release(language_name, release, directory / f"press-release-{slug}.md")
        (directory / "social-media-posts.md").write_text(social_copy(language_name), encoding="utf-8")
        social_image(language, "square", (1080, 1080), social / "fairwindsk-square-1080x1080.png")
        social_image(language, "landscape", (1200, 630), social / "fairwindsk-landscape-1200x630.png")
        social_image(language, "story", (1080, 1920), social / "fairwindsk-story-1080x1920.png")
        brochure(language_name, directory / f"brochure-fairwindsk-{language['code']}.pdf")


if __name__ == "__main__":
    main()
