#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <limits.h>
#include<time.h>
#include <pthread.h>
#define PORT 8020
#define TAILLE_PAQUET 10
typedef struct {
    int valeur_numerique;
    int valeur_tetes_de_boeuf;
    int id_joueur; // Ajout de l'identifiant du joueur
} Carte;
typedef struct {
    Carte cartes[5];
    int nombre_cartes;
    int teteDeBoeufTotal;
} Rangee;
typedef struct {
    Rangee rangees[4]; // Changer à 4 si nécessaire
    int tour_actuel;
   int partie_terminee;
} Partie;
typedef struct{
    int score;
}Score;
void communiquerAvecServeur(int socket);
Carte * choisirEtSupprimerCarte(Carte paquet[]);
void affichepaquet(Carte paquet[], Score s);
void envoyerCarte(int socket, const Carte *carte);
void initialiserScore(Score *s) {
    s->score = 0;
}
void intialiserPartie(Partie *partie)
{
    partie->partie_terminee=0;
}
void afficherMessageErreur(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}
void deserializepaquet(char *buffer, size_t buffersize, Carte *c) {
    if (c == NULL || buffer == NULL || buffersize < sizeof(Carte)) {
        afficherMessageErreur("Erreur lors de la désérialisation du paquet");
    }
    memcpy(c, buffer, sizeof(Carte));
}

void libererPaquet(Carte paquet[]) {
    for (int i = 0; i < TAILLE_PAQUET; ++i) {
        memset(&paquet[i], 0, sizeof(Carte));
    }
}
void affichecarte(Carte c) {
    if (c.valeur_numerique != 0) {
        printf("Joueur %d - ", c.id_joueur);

        // Choisir une couleur en fonction de l'id du joueur
        switch (c.id_joueur % 6) {
            case 0:
                printf("\033[1;31m"); // Rouge
                break;
            case 1:
                printf("\033[1;32m"); // Vert
                break;
            case 2:
                printf("\033[1;33m"); // Jaune
                break;
            case 3:
                printf("\033[1;34m"); // Bleu
                break;
            default:
                break;
        }

        printf("%d || %d\033[0m\n", c.valeur_numerique, c.valeur_tetes_de_boeuf);
    }
}
void recevoirPaquet(int socket, Carte paquet[]) {
    size_t buffer_size = sizeof(Carte);

    for (int i = 0; i < TAILLE_PAQUET; ++i) {
        char buffer[buffer_size];
        int bytesRead = recv(socket, buffer, buffer_size, 0);

        if (bytesRead == -1) {
            perror("Erreur lors de la réception des données");
            // Ajoutez un traitement approprié pour gérer l'erreur, par exemple, sortir de la boucle
            break;
        } else if (bytesRead == 0) {
            printf("Le serveur a fermé la connexion\n");
            // Ajoutez un traitement approprié pour gérer la fermeture du serveur, par exemple, sortir de la boucle
            break;
        } else {
            Carte carte;
            deserializepaquet(buffer, buffer_size, &carte);
            paquet[i] = carte;
            affichecarte(paquet[i]); // Si vous avez une fonction pour afficher la carte
        }
    }
}
void deserialiserScore(char *buffer, size_t buffersize, Score *score) {
    if (score == NULL || buffer == NULL || buffersize < sizeof(Score)) {
        afficherMessageErreur("Erreur lors de la désérialisation du score");
    }
    memcpy(score, buffer, sizeof(Score));
}
void recevoirscore(int socket,Score *sc)
{
    int s;
    recv(socket, &s, sizeof(int), 0);
    sc->score+=s;
      printf("Mon score est %d \U0001F42E\n ", sc->score);
}
void recevoirCartesDesAutresJoueurs(int socket, int nombreJoueurs) {
    char buffer[sizeof(Carte)];

    for (int i = 0; i < nombreJoueurs - 1; ++i) {
        int bytesRead = recv(socket, buffer, sizeof(buffer), 0);
        if (bytesRead == -1) {
            perror("Erreur lors de la réception des données");
        } else if (bytesRead == 0) {
            printf("Le serveur a fermé la connexion\n");
        } else {
            Carte c;
            deserializepaquet(buffer, sizeof(buffer), &c);
            printf("la carte choisi par ");
            affichecarte(c);
        }
    }
}

void deserializetable(char *buffer, size_t buffersize, Rangee *rangee) {
    if (rangee == NULL || buffer == NULL || buffersize < sizeof(Rangee)) {
        afficherMessageErreur("Erreur lors de la désérialisation du table");
    }

    // Désérialiser les données de la rangée
    memcpy(rangee, buffer, sizeof(Rangee));
}
void recevoirTable(int socket, Partie *partie) {
    // Taille totale du buffer nécessaire pour désérialiser une rangée
    size_t buffer_size = sizeof(Rangee);

    // Allouer le buffer
    char *buffer = malloc(buffer_size);

    // Désérialiser et recevoir chaque rangée
    for (int i = 0; i < 4; ++i) {
        recv(socket, buffer, buffer_size, 0);
        deserializetable(buffer, buffer_size, &(partie->rangees[i]));
    }

    // Libérer le buffer après la réception de toutes les rangées
    free(buffer);
}

void affichertable(Partie *partie) {
    for (int i = 0; i < 4; i++) {
        // Afficher l'en-tête de la rangée en noir gras
        printf("\033[1;30;1mRangée %d (Têtes de bœuf: %d \U0001F42E)\033[0m\n", i, partie->rangees[i].teteDeBoeufTotal);

        for (int j = 0; j < partie->rangees[i].nombre_cartes; j++) {
            // Choisir une couleur en fonction de l'id du joueur
            int couleurJoueur = partie->rangees[i].cartes[j].id_joueur % 6;

            // Afficher la carte avec la couleur du joueur
            printf("\033[1;%dm %d || %d\033[0m\n", 31 + couleurJoueur,
                   partie->rangees[i].cartes[j].valeur_numerique, partie->rangees[i].cartes[j].valeur_tetes_de_boeuf);
        }

        printf("\n");
    }
}

void affichepaquet(Carte paquet[], Score s) {
    for (int i = 0; i < TAILLE_PAQUET; i++) {
        affichecarte(paquet[i]);
    }
}


void serializercarte(char *buffer, size_t buffersize, const Carte *c) {
    if (c == NULL || buffer == NULL || buffersize < sizeof(Carte)) {
        return;
    }
    memcpy(buffer, c, sizeof(Carte));
}

void envoyerCarte(int socket, const Carte *carte) {
    char buffer[sizeof(Carte)];

    // Sérialisation de la carte
    serializercarte(buffer, sizeof(buffer), carte);

    // Envoyer la carte au serveur
    send(socket, buffer, sizeof(buffer), 0);
}

void communiquerAvecServeur(int socket) {
    // Recevoir la taille du message d'abord
    size_t tailleMessage;
    int bytesRead = recv(socket, &tailleMessage, sizeof(size_t), 0);

    if (bytesRead <= 0) {
        // Gérer l'erreur ou la fermeture de la connexion
        perror("Erreur lors de la réception de la taille du message");
        return;
    }

    // Allouer suffisamment d'espace pour le message
    char *message = malloc(tailleMessage);

    // Recevoir le message
    bytesRead = recv(socket, message, tailleMessage, 0);

    if (bytesRead <= 0) {
        // Gérer l'erreur ou la fermeture de la connexion
        perror("Erreur lors de la réception du message");
    } else {
        // Assurez-vous de terminer la chaîne avec un caractère nul pour l'affichage correct
        message[bytesRead] = '\0';
        printf("Gestionnaire dit : %s\n", message);
        //sleep(1);
    }

    // Libérer la mémoire allouée pour le message
    free(message);
}
Carte *choisirEtSupprimerCarte(Carte paquet[]) {
    int valeurNumerique;
    int indice = -1; // Initialisez à un indice non valide

    do {
        printf("Choisissez la valeur numérique d'une carte à jouer : ");
        if (scanf("%d", &valeurNumerique) != 1) {
            // Si l'entrée n'est pas un entier, videz le tampon d'entrée et réessayez.
            printf("Veuillez entrer un nombre entier valide.\n");
            while (getchar() != '\n'); // Vide le tampon d'entrée.
            continue;
        }

        // Recherchez la carte avec la valeur numérique choisie.
        for (int i = 0; i < TAILLE_PAQUET; ++i) {
            if (paquet[i].valeur_numerique == valeurNumerique) {
                indice = i;
                break; // Carte trouvée, sortez de la boucle.
            }
        }

        if (indice == -1) {
            printf("Aucune carte avec la valeur numérique choisie n'a été trouvée. Veuillez réessayer.\n");
        }

    } while (indice == -1);

    Carte *c = malloc(sizeof(Carte));
    c->valeur_numerique = paquet[indice].valeur_numerique;
    c->valeur_tetes_de_boeuf = paquet[indice].valeur_tetes_de_boeuf;
    c->id_joueur = paquet[indice].id_joueur;

    printf("Vous avez choisi la carte : ");
    affichecarte(*c);

    // Décalez les cartes restantes dans le tableau pour combler le vide.
    for (int i = indice; i < TAILLE_PAQUET - 1; ++i) {
        paquet[i] = paquet[i + 1];
    }

    memset(&paquet[TAILLE_PAQUET - 1], 0, sizeof(Carte));

    return c;
}
int recevoirPartieTermineeDuServeur(int socket) {
  int partieTerminee;
    recv(socket, &partieTerminee, sizeof(int), 0);
    return partieTerminee;
}
int recevoirNbrJoueurs(int socket) {
  int nbrJ;
    recv(socket, &nbrJ, sizeof(int), 0);
    return nbrJ;
}
int recevoirNbrManche(int socket) {
  int manche;
    recv(socket, &manche, sizeof(int), 0);
    return manche;
}
int recevoirScorepartie(int socket) {
  int SCORE;
    recv(socket, &SCORE, sizeof(int), 0);
    return SCORE;
}
void recevoirScoreGagnant(int socket, int* scoreGagnant) {
    if (recv(socket, scoreGagnant, sizeof(int), 0) == -1) {
        perror("Erreur lors de la réception du score du gagnant");
        // Gérer l'erreur
    }
}
void recevoirMessageDuServeur(int socket, char *message) {
    // Recevez la taille du message
    int tailleMessage;
    recv(socket, &tailleMessage, sizeof(int), 0);

    // Recevez le message lui-même
    recv(socket, message, tailleMessage, 0);

    // Ajoutez un caractère de fin de chaîne à la fin du message
    message[tailleMessage] = '\0';
}

void *jeu_thread(void *arg) {
    int socketClient = *((int *)arg);
   // communiquerAvecServeur(socketClient);
     int NOMBRE_DE_JOUEURS=recevoirNbrJoueurs(socketClient);
     int manche=recevoirNbrManche(socketClient);
     int SCORE =recevoirScorepartie(socketClient);
    Score s;
    initialiserScore(&s);
    int partieTerminer=0;
    while (s.score < SCORE && !partieTerminer && manche!=0) {
        Partie *partie = malloc(sizeof(Partie));
        Carte *paquet = malloc(TAILLE_PAQUET * sizeof(Carte));

        recevoirPaquet(socketClient, paquet);
        printf("Le score du Joueur %d : %d \U0001F42E\n", paquet[0].id_joueur, s.score);

        recevoirTable(socketClient, partie);
        affichertable(partie);

        int n = 0;
        for (int tour = 0; tour < 10; ++tour) {
            communiquerAvecServeur(socketClient);
            Carte *c;
            c = choisirEtSupprimerCarte(paquet);
            affichepaquet(paquet, s);
            envoyerCarte(socketClient, c);
            free(c);
            n++;

            recevoirCartesDesAutresJoueurs(socketClient, NOMBRE_DE_JOUEURS);
            recevoirscore(socketClient, &s);
            recevoirTable(socketClient, partie);
            affichertable(partie);
        }

        free(paquet);
        free(partie);

        // Vérifie si la partie est terminée après chaque tour

        // Communique avec le serveur après chaque tour
        communiquerAvecServeur(socketClient);
        partieTerminer=recevoirPartieTermineeDuServeur(socketClient);
        manche--;
       if (partieTerminer == 1 || manche==0) {
    int scoreGagnant;
    recevoirScoreGagnant(socketClient, &scoreGagnant);
    // Le joueur avec le score le plus bas reçoit le message
    if (s.score == scoreGagnant) {
        char messageGagnant[100]; // Ajustez la taille selon vos besoins
          printf("\033[1;32m🎉Félicitations ! Vous avez gagné avec un score de %d !🎉\033[0m\n",scoreGagnant);
    }
    else {
        // Affichez un message indiquant que le joueur a perdu
        printf("Vous avez perdu. Meilleure chance la prochaine fois.\n");
    }
}
else{
    communiquerAvecServeur(socketClient);
}

    }
     printf("\033[1;35mFin De La Partie !! 🏁\033[0m\n");
    // Fermez la connexion, libérez les ressources, etc., si nécessaire
    close(socketClient);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Utilisation : %s <adresse_ip_serveur>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int socketClient;
    struct sockaddr_in serveurAdresse;

    socketClient = socket(AF_INET, SOCK_STREAM, 0);
    if (socketClient == -1) {
        perror("Erreur lors de la création du socket du client");
        exit(EXIT_FAILURE);
    }

    serveurAdresse.sin_family = AF_INET;
    serveurAdresse.sin_port = htons(PORT);
 if (inet_pton(AF_INET, argv[1], &serveurAdresse.sin_addr) <= 0) {
        perror("Adresse IP invalide");
        exit(EXIT_FAILURE);
    }

    if (connect(socketClient, (struct sockaddr *)&serveurAdresse, sizeof(serveurAdresse)) < 0) {
        perror("Erreur lors de la connexion au serveur");
        exit(EXIT_FAILURE);
    }

    // Création du thread pour la logique du jeu
    pthread_t thread;
    if (pthread_create(&thread, NULL, jeu_thread, &socketClient) != 0) {
        perror("Erreur lors de la création du thread de jeu");
        exit(EXIT_FAILURE);
    }

    // Attente de la fin du thread
    if (pthread_join(thread, NULL) != 0) {
        perror("Erreur lors de l'attente du thread de jeu");
        exit(EXIT_FAILURE);
    }

    // Fermez la connexion après la fin du thread de jeu
    close(socketClient);

    return 0;
}
