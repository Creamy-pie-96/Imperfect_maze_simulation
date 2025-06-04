#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <climits>
using namespace std;

const float cellSize = 60.f;
const float offset = 50.f;
const float animationDelay_ms = 1000;
int total=0;
void drawMaze(const vector<vector<int>> &maze, const int &ratRow, const int &ratCol, const vector<bool> &visited, sf::RenderWindow &window, const int &n,int totalCost)
{
    sf::Font font;
    if (!font.loadFromFile("arial.ttf"))
    {
        cerr << "failed to load text from 'arial.ttf'\n";
    }
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            sf::RectangleShape Cell(sf::Vector2f(cellSize, cellSize));
            Cell.setPosition(offset + j * cellSize, offset + i * cellSize);
            Cell.setOutlineThickness(1.f);
            Cell.setOutlineColor(sf::Color::White);

            if (maze[i][j] == 0)
                Cell.setFillColor(sf::Color(50, 50, 50));
            else // for visited cells
            {
                // If the cell has been visited during the current exploration path
                if (visited[i * n + j])
                {
                    // Use a slightly transparent yellow to show it's visited
                    Cell.setFillColor(sf::Color(200, 200, 0, 150));
                }
                else
                {
                    Cell.setFillColor(sf::Color(180,
                         100, 100)); // Lighter grey for unvisited passable cells
                }
            }
            if (i == 0 && j == 0)
                Cell.setFillColor(sf::Color(0, 180, 0));
            else if (i == n - 1 && j == n - 1)
                Cell.setFillColor(sf::Color(100, 0, 0));
            window.draw(Cell);
            if (maze[i][j] != 0)
            {
                sf::Text costText;
                costText.setFont(font);
                costText.setCharacterSize(20);
                costText.setString(to_string(maze[i][j]));
                sf::Rect req = costText.getLocalBounds();
                costText.setOrigin(req.left + req.width / 2.f, req.top + req.height / 2.f);
                costText.setPosition(Cell.getPosition().x + Cell.getSize().x / 2, Cell.getPosition().y + Cell.getSize().y / 2);
                costText.setFillColor(sf::Color::White);
                sf::Text Total;
                Total.setFont(font);
                Total.setFillColor(sf::Color::White);
                Total.setCharacterSize(20);
                Total.setString("Total cost: "+to_string(totalCost));
                Total.setPosition(10.f,10.f);
                window.draw(costText);
                window.draw(Total);
            }
        }
    }
    sf::CircleShape Rat;
    Rat.setOrigin(Rat.getRadius(), Rat.getRadius());
    Rat.setRadius(cellSize / 3.f);
    float RatX;
    float RatY;

    RatX = offset + ratCol * cellSize + cellSize / 2.f;
    RatY = offset + ratRow * cellSize + cellSize / 2.f;

    Rat.setPosition(RatX, RatY);
    Rat.setFillColor(sf::Color::Cyan);
    window.draw(Rat);
}

void solve(int remainingDepth, int n, int row, int col, vector<vector<char>> &ans, vector<char> &path, const vector<vector<int>> &maze, vector<bool> &visited, int pathLength, int totalUsedCost, int &bestCost, int &shortestLength, sf::RenderWindow &window, bool &isOpen)
{
    if (!isOpen)
        return;
    window.clear(sf::Color(30, 30, 30));

    drawMaze(maze, row, col, visited, window, n,totalUsedCost);
    window.display();
    sf::sleep(sf::milliseconds(animationDelay_ms));
    sf::Event event;
    while (window.pollEvent(event))
    {
        if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape))
        {
            isOpen = false; // Set flag to stop recursion
            window.close(); // Close the window
        }
    }
    // Destination reached
    if (row == n - 1 && col == n - 1)
    {
        if (totalUsedCost < bestCost)
        {
            bestCost = totalUsedCost;
            shortestLength = pathLength;
            ans.clear();
            ans.push_back(path);
            total=totalUsedCost;
        }
        else if (totalUsedCost == bestCost)
        {
            if (pathLength < shortestLength)
            {
                shortestLength = pathLength;
                ans.clear();
                ans.push_back(path);
            total=totalUsedCost;
            }
            else if (pathLength == shortestLength)
            {
                ans.push_back(path);
            total=totalUsedCost;
            }
        }
        return;
    }

    // Pruning
    int distance = abs(n - 1 - row) + abs(n - 1 - col);
    if (remainingDepth < distance || pathLength >= shortestLength)
        return;

    // Move directions: Right, Down, Left, Up
    int dx[4] = {0, 1, 0, -1};
    int dy[4] = {1, 0, -1, 0};
    char dir[4] = {'R', 'D', 'L', 'U'};

    for (int i = 0; i < 4; ++i)
    {
        int newRow = row + dx[i];
        int newCol = col + dy[i];

        if (newRow >= 0 && newRow < n && newCol >= 0 && newCol < n)
        {
            int index = newRow * n + newCol;
            int cost = maze[newRow][newCol];

            if (maze[newRow][newCol] != 0 && !visited[index])
            {
                if (remainingDepth - cost < 0)
                    continue;

                visited[index] = true;
                path.push_back(dir[i]);
                solve(remainingDepth - cost, n, newRow, newCol, ans, path,
                      maze, visited, pathLength + 1, totalUsedCost + cost,
                      bestCost, shortestLength, window, isOpen);
                path.pop_back();
                visited[index] = false;
                window.clear(sf::Color(30, 30, 30));

                drawMaze(maze, row, col, visited, window, n,totalUsedCost);
                window.display();
                sf::sleep(sf::milliseconds(animationDelay_ms / 2));
                sf::Event event;
                while (window.pollEvent(event))
                {
                    if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape))
                    {
                        isOpen = false; // Set flag to stop recursion
                        window.close(); // Close the window
                    }
                }
                if (!isOpen)
                    return;
            }
        }
    }
}

int main()
{
    
    vector<vector<int>> maze = {
        {1, 2, 0, 1, 1, 1},
        {0, 3, 2, 0, 2, 0},
        {1, 0, 3, 4, 0, 1},
        {2, 2, 0, 2, 3, 0},
        {0, 1, 1, 0, 4, 2},
        {1, 0, 2, 1, 1, 1}};

    int n = maze.size();
    int k = 100; // initial max energy
    vector<bool> visited(n * n, false);
    visited[0] = true;
    const float windowHeight = (n * cellSize + 2 * offset);
    const float winowWidth = (n * cellSize + 2 * offset);
    sf::RenderWindow window(sf::VideoMode(winowWidth, windowHeight), "Maze Solve Simulation", sf::Style::Close);
    window.setFramerateLimit(60);
    bool isOpen = window.isOpen();
    vector<char> path;
    vector<vector<char>> ans;

    int bestCost = INT_MAX;
    int shortestLength = INT_MAX;

    solve(k - maze[0][0], n, 0, 0, ans, path, maze, visited, 0, maze[0][0], bestCost, shortestLength, window, isOpen);

    cout << "Optimal Paths (Cost: " << bestCost << ", Steps: " << shortestLength << "):\n";
    for (const auto &p : ans)
    {
        for (char c : p)
            cout << c << " ";
        cout << endl;
    }
    int currentRow = 0;
        int currenCol = 0;
        bool isFinished = false;
    drawMaze(maze, currentRow, currenCol, visited, window, n,total);
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape))
                window.close();
        }
        if (!isFinished)
        {
            for (const auto &p : ans)
            {
                for (char c : p)
                {
                    visited[currentRow * n + currenCol] = true;
                    if (c == 'R')
                        currenCol++;
                    else if (c == 'L')
                        currenCol--;
                    else if (c == 'U')
                        currentRow--;
                    else if (c == 'D')
                        currentRow++;
                    window.clear(sf::Color(30, 30, 30));
                    drawMaze(maze, currentRow, currenCol, visited, window, n,total);
                    window.display();
                    sf::sleep(sf::microseconds(animationDelay_ms / 1.5));
                    if (currenCol == n - 1 && currentRow == n - 1)
                        isFinished = true;
                }
            }
        }
        // will display the optimal path later with different color!
        // window.close();
        cout << "In the loop";
    }

    cout << endl
         << "broke the loop" << endl;
    return 0;
}
