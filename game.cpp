#include <SFML/Graphics.hpp>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <chrono>
#include <stack>
#include <fstream>
#include <string>
#include <cmath>

const int CELL_SIZE = 30; // Taille de chaque cellule
const int width = 60;     // Largeur du labyrinthe (modifiable)
const int height = 25;    // Hauteur du labyrinthe (modifiable)
const int WINDOW_WIDTH = width * CELL_SIZE;
const int WINDOW_HEIGHT = height * CELL_SIZE;
const int MAX_TREASURES = height / 2; // Limite de diamants
int walk = (width * height) / 3;
std::string kill = "null";
std::string RemoveDiams = "null";
bool DevModStatus = true;

struct Player
{
    int x, y;
};

class MazeGame
{
public:
    MazeGame() : player{1, 1}, collectedTreasures(0), walkBoost{generateRandomPosition()}
    {
        if (!playerTexture.loadFromFile("player.png"))
        {
            std::cerr << "Erreur de chargement de la texture du joueur !" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            exit(1);
        }
        if (!treasureTexture.loadFromFile("treasure.png"))
        {
            std::cerr << "Erreur de chargement de la texture du trésor !" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            exit(1);
        }
        if (!wallTexture.loadFromFile("wall.png"))
        {
            std::cerr << "Erreur de chargement de la texture du mur !" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            exit(1);
        }
        if (!walkTexture.loadFromFile("boost.png")) 
        {
            std::cerr << "Erreur de chargement de la texture du boost !" << std::endl;
        } else if (DevModStatus)
        {
            // Rend le boost visible
            std::cerr << "Boost actif en mode dev !" << std::endl;
            std::cerr << "Position du boost : " << walkBoost.x + 1 << ", " << walkBoost.y + 1 << std::endl;
            walkSprite.setTexture(walkTexture); // Charger la texture du boost
            walkSprite.setPosition(walkBoost.x, walkBoost.y); // Positionner le sprite du boost là où il se trouve
        }

        playerSprite.setTexture(playerTexture);
        treasureSprite.setTexture(treasureTexture);
        wallSprite.setTexture(wallTexture);

        walkSprite.setPosition(walkBoost.x, walkBoost.y);
        walkSprite.setScale((float)CELL_SIZE / walkTexture.getSize().x, (float)CELL_SIZE / walkTexture.getSize().x);

        float scaleFactor = (float)CELL_SIZE / playerTexture.getSize().x;
        playerSprite.setScale(scaleFactor, scaleFactor);

        // **Correction de la taille du mur**
        float wallScaleX = (float)CELL_SIZE / wallTexture.getSize().x;
        float wallScaleY = (float)CELL_SIZE / wallTexture.getSize().y;
        wallSprite.setScale(wallScaleX, wallScaleY);

        // **Appliquer un scale plus petit pour le diamant**
        float treasureScale = (float)CELL_SIZE / treasureTexture.getSize().x;
        treasureSprite.setScale(treasureScale, treasureScale);

        generateMaze();
        placeTreasures();
    }

    Player generateRandomPosition()
    {
        Player pos;
        pos.x = (rand() % width - 2) + 1;  // Génère une position X aléatoire dans la largeur de la carte
        pos.y = (rand() % height - 2) + 1; // Génère une position Y aléatoire dans la hauteur de la carte
        return pos;
    }

    void draw(sf::RenderWindow &window)
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                sf::RectangleShape cellShape(sf::Vector2f(CELL_SIZE, CELL_SIZE));
                cellShape.setPosition(x * CELL_SIZE, y * CELL_SIZE);

                if (maze[y][x] == 1)
                {
                    wallSprite.setPosition(x * CELL_SIZE, y * CELL_SIZE);
                    window.draw(wallSprite);
                }
                else
                {
                    cellShape.setFillColor(sf::Color::Black);
                    window.draw(cellShape);
                }
            }
        }

        playerSprite.setPosition(player.x * CELL_SIZE, player.y * CELL_SIZE);
        window.draw(playerSprite);

        for (auto &t : treasures)
        {
            treasureSprite.setPosition(t.x * CELL_SIZE, t.y * CELL_SIZE);
            window.draw(treasureSprite);
        }
    }

    void movePlayer(int dx, int dy)
    {
        int newX = player.x + dx;
        int newY = player.y + dy;
        if (newX >= 0 && newX < width && newY >= 0 && newY < height && maze[newY][newX] == 0)
        {
            player.x = newX;
            player.y = newY;
            checkTreasure();
            checkBoost();
        }
    }

    bool isGameOver() const
    {
        return treasures.empty(); // Jeu terminé lorsque tous les trésors ont été collectés
    }

    void checkBoost()
    {
        if (player.x == walkBoost.x && player.y == walkBoost.y)
        {
            std::cerr << "Boost actif ! +100 deplacements !" << std::endl;
            walk += 100;                          // Ajoute 100 déplacements
            walkBoost = generateRandomPosition(); // Déplace le boost ailleurs
        }
    }

    // Message de victoire :
    void displayVictory(sf::RenderWindow &window)
    {
        if (DevModStatus)
            std::cerr << "[Normal] Victory" << std::endl;
        window.clear(); // Clear the window before displaying the victory message

        // Création du texte de victoire
        sf::Text endText;
        sf::Font font;

        if (!font.loadFromFile("police.ttf"))
        {
            std::cerr << "Erreur de chargement de la police !" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            return; // Arrêter la fonction si la police ne peut pas être chargée
        }

        endText.setFont(font);
        endText.setString("Vous avez gagne ! Il vous reste " + std::to_string(walk) + " deplacements.");
        endText.setCharacterSize(25);           // Taille du texte
        endText.setFillColor(sf::Color::Green); // Couleur du texte pour être visible

        // Centrer le texte horizontalement et verticalement
        sf::FloatRect textBounds = endText.getLocalBounds();
        endText.setPosition(WINDOW_WIDTH / 2 - textBounds.width / 2,
                            WINDOW_HEIGHT / 2 - textBounds.height / 2);

        // Afficher le texte à l'écran
        window.draw(endText);
        window.display(); // Afficher la fenêtre mise à jour

        // Attendre 15 secondes avant de fermer
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    void displayCheatVictory(sf::RenderWindow &window)
    {
        if (DevModStatus)
            std::cerr << "[Cheat] Victory" << std::endl;
        window.clear(); // Clear the window before displaying the victory message

        // Création du texte de victoire
        sf::Text endText;
        sf::Font font;

        if (!font.loadFromFile("police.ttf"))
        {
            std::cerr << "Erreur de chargement de la police !" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
            return; // Arrêter la fonction si la police ne peut pas être chargée
        }

        endText.setFont(font);
        endText.setString("USAGE DE CHEAT !");
        endText.setCharacterSize(75);         // Taille du texte
        endText.setFillColor(sf::Color::Red); // Couleur du texte pour être visible

        // Centrer le texte horizontalement et verticalement
        sf::FloatRect textBounds = endText.getLocalBounds();
        endText.setPosition(WINDOW_WIDTH / 2 - textBounds.width / 2,
                            WINDOW_HEIGHT / 2 - textBounds.height / 2);

        // Afficher le texte à l'écran
        window.draw(endText);
        window.display(); // Afficher la fenêtre mise à jour

        // Attendre 5 secondes avant de fermer
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    void displayDead(sf::RenderWindow &window)
    {
        if (DevModStatus)
            std::cerr << "[Normal] Dead" << std::endl;
        window.clear(); // Clear the window before displaying the message

        // Création du texte
        sf::Text endText;
        sf::Font font;

        if (!font.loadFromFile("police.ttf"))
        {
            std::cerr << "Erreur de chargement de la police !" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            return; // Arrêter la fonction si la police ne peut pas être chargée
        }

        endText.setFont(font);
        endText.setString("PERDU! dommage...");
        endText.setCharacterSize(55);         // Taille du texte
        endText.setFillColor(sf::Color::Red); // Couleur du texte pour être visible

        // Centrer le texte horizontalement et verticalement
        sf::FloatRect textBounds = endText.getLocalBounds();
        endText.setPosition(WINDOW_WIDTH / 2 - textBounds.width / 2,
                            WINDOW_HEIGHT / 2 - textBounds.height / 2);

        // Afficher le texte à l'écran
        window.draw(endText);
        window.display(); // Afficher la fenêtre mise à jour

        // Attendre 10 secondes avant de fermer
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    // Cheat code :
    bool Cheatkill(char letter, sf::RenderWindow &window)
    {
        if (kill == "null")
        {
            kill = letter;
            if (DevModStatus)
                std::cerr << "[Cheat] Init Kill" << std::endl;
            return false;
        }
        else
        {
            kill += letter;
            if (kill == "KILL")
            {
                if (DevModStatus)
                    std::cerr << "[Cheat] End Kill" << std::endl;
                return true;
            }
            else if (kill.size() > 4)
            {
                kill = "null";
                return false;
            }
            else
            {
                return false;
            }
        }
    }

    void CheatRemoveDiams(char letter, sf::RenderWindow &window)
    {
        if (RemoveDiams == "null")
        {
            RemoveDiams = letter;
            if (DevModStatus)
                std::cerr << "[Cheat] Init Remove" << std::endl;
        }
        else
        {

            RemoveDiams += letter;
            if (RemoveDiams.size() > 6)
            {
                RemoveDiams = "null";
            }
            else if (RemoveDiams == "REMOVE")
            {
                if (DevModStatus)
                    std::cerr << "[Cheat] End Remove" << std::endl;
                if (treasures.empty())
                {
                    return; // Aucun trésor à supprimer
                }

                auto farthestIt = treasures.begin();
                float maxDistance = 0.0f;

                for (auto it = treasures.begin(); it != treasures.end(); ++it)
                {
                    float distance = std::hypot(it->x - player.x, it->y - player.y); // Calcul de la distance euclidienne
                    if (distance > maxDistance)
                    {
                        maxDistance = distance;
                        farthestIt = it;
                    }
                }
                RemoveDiams = "null";
                treasures.erase(farthestIt); // Supprime le diamant le plus éloigné
            }
        }
    }

private:
    int maze[width][height]; // Le labyrinthe peut maintenant être plus grand
    Player player;
    Player walkBoost;
    std::vector<Player> treasures;
    int collectedTreasures;

    sf::Texture playerTexture, treasureTexture, wallTexture, walkTexture, empty;
    sf::Sprite playerSprite, treasureSprite, wallSprite, walkSprite;

    void generateMaze()
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                maze[y][x] = (rand() % 3 == 0) ? 1 : 0;
            }
        }
        maze[1][1] = 0;
    }

    void placeTreasures()
    {
        treasures.clear(); // Clear previous treasures
        bool placedSuccessfully = false;

        // Continue trying to place treasures until all conditions are met
        while (!placedSuccessfully)
        {
            // Générer des trésors aléatoires
            int maxAttempts = 100; // Maximum number of attempts to place treasures
            int attempts = 0;

            while (treasures.size() < MAX_TREASURES && attempts < maxAttempts)
            {
                int x, y;
                bool placed = false;
                while (!placed)
                {
                    x = rand() % (width - 2) + 1; // On place les trésors à partir de 1, 1 pour éviter les bords
                    y = rand() % (height - 2) + 1;

                    // Si la cellule est vide et accessible, on place le trésor
                    if (maze[y][x] == 0 && isAccessible(x, y))
                    {
                        treasures.push_back({x, y});
                        placed = true;
                    }

                    attempts++;
                    if (attempts > maxAttempts)
                    {
                        break;
                    }
                }
            }

            // Si on a pu placer tous les trésors, on considère la position comme valide
            if (treasures.size() <= MAX_TREASURES)
            {
                placedSuccessfully = true;
            }
            else
            {
                // Sinon, on régénère le labyrinthe et recommence
                generateMaze();
                treasures.clear(); // Clear previous treasures
            }
        }
    }

    void checkTreasure()
    {
        for (auto it = treasures.begin(); it != treasures.end(); ++it)
        {
            if (it->x == player.x && it->y == player.y)
            {
                treasures.erase(it);
                collectedTreasures++;
                break;
            }
        }
    }

    bool isAccessible(int targetX, int targetY)
    {
        // Perform a Depth-First Search (DFS) to check if the target is reachable from the start (1, 1)
        std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
        std::stack<std::pair<int, int>> stack;
        stack.push({player.x, player.y});
        visited[player.y][player.x] = true;

        while (!stack.empty())
        {
            auto [x, y] = stack.top();
            stack.pop();

            if (x == targetX && y == targetY)
                return true;

            // Explore neighbors (up, down, left, right)
            if (y > 0 && !visited[y - 1][x] && maze[y - 1][x] == 0)
            {
                visited[y - 1][x] = true;
                stack.push({x, y - 1});
            }
            if (y < height - 1 && !visited[y + 1][x] && maze[y + 1][x] == 0)
            {
                visited[y + 1][x] = true;
                stack.push({x, y + 1});
            }
            if (x > 0 && !visited[y][x - 1] && maze[y][x - 1] == 0)
            {
                visited[y][x - 1] = true;
                stack.push({x - 1, y});
            }
            if (x < width - 1 && !visited[y][x + 1] && maze[y][x + 1] == 0)
            {
                visited[y][x + 1] = true;
                stack.push({x + 1, y});
            }
        }
        return false;
    }
};

int main()
{
    srand(time(0));
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Donjon");
    MazeGame game;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            // **Correction : Ne détecte qu'un appui unique**
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Z)
                {
                    game.movePlayer(0, -1);
                    walk--;
                }
                if (event.key.code == sf::Keyboard::S)
                {
                    game.movePlayer(0, 1);
                    walk--;
                }
                if (event.key.code == sf::Keyboard::Q)
                {
                    game.movePlayer(-1, 0);
                    walk--;
                }
                if (event.key.code == sf::Keyboard::D)
                {
                    game.movePlayer(1, 0);
                    walk--;
                }
                if (event.key.code == sf::Keyboard::K || event.key.code == sf::Keyboard::I || event.key.code == sf::Keyboard::L)
                {
                    std::string text = "L";
                    if (event.key.code == 10)
                        text = "K";
                    if (event.key.code == 8)
                        text = "I";
                    if (game.Cheatkill(text[0], window))
                    {
                        game.displayCheatVictory(window);
                        break;
                    }
                }
                if (event.key.code == sf::Keyboard::R || event.key.code == sf::Keyboard::E || event.key.code == sf::Keyboard::M || event.key.code == sf::Keyboard::O || event.key.code == sf::Keyboard::V)
                {
                    std::string text = "R";
                    if (event.key.code == 4)
                        text = "E";
                    if (event.key.code == 12)
                        text = "M";
                    if (event.key.code == 14)
                        text = "O";
                    if (event.key.code == 21)
                        text = "V";
                    game.CheatRemoveDiams(text[0], window);
                }
            }
        }

        if (game.isGameOver() || kill == "KILL") // Check if the game is over
        {
            game.displayVictory(window);
            break; // End the game loop
        }

        if (walk == 0)
        {
            game.displayDead(window);
            break;
        }

        window.clear(); // Clear the window before drawing the game
        game.draw(window);
        window.display();
    }

    return 0;
}
