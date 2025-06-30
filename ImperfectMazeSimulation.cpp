#include <iostream>
#include <SFML/Graphics.hpp>
#include <ctime>
#include <vector> // For vector (though raw pointers are still used for maze/visited)
#include <cmath>  // For abs (not strictly needed here)
#include <math.h>
#include <climits>
#include <unordered_map>
#include <tuple>
#include<algorithm>
using namespace std;

// Forward declaration for ratInmaze (will be removed later if not used)
// int ratInmaze(vector<vector<int>> maze); // REMOVED: Not defined in this file, causes linker error.

int dx[4] = {0, 1, 0, -1};
int dy[4] = {-1, 0, 1, 0};

float cellSize;
const float offset = 50.f;
float animationDelay_ms; // Milliseconds
float delayForBacktrack;
int End[2]{-1, -1};
int start[2] = {-1, -1};

vector<bool> TotalVisited;
vector<vector<bool>> DeadEnd; // deadend[cell][direction]
vector<vector<int>> Route;
vector<vector<bool>> Path_toRender;
vector<char> ansPath;

bool isPerfect;
int totalTraversed = 1;
int currentPathLength = 1;
int totalPath = 0;
int totalCost = 0;
int shortestLen = 0;
int minimumCost = 0;
bool gotSOl = false;
bool Isdebug = false;
const int max_cellCost = 10;
struct intelligence
{
    char dir;
    int index;
    double direction_importance;
};

struct KeyHash
{
    size_t operator()(const tuple<int, int, int> &k) const
    {
        return get<0>(k) ^ (get<1>(k) << 1) ^ (get<2>(k) << 2);
    }
};

unordered_map<tuple<int, int, int>, bool, KeyHash> deadEndMap;

double getDistance(int x1, int x2, int y1, int y2)
{
    return (double)(abs(x1 - x2) + abs(y1 - y2));
}
void drawMazeGeneration(sf::RenderWindow &window, int **maze, int currentX, int currentY, int size, bool isImperfecting)
{
    sf::Font font;
    if (!font.loadFromFile("arial.ttf"))
    {
        cerr << "failed to load text from 'arial.ttf'\n";
    }

    for (int i = 0; i < size; ++i)
    {
        for (int j = 0; j < size; ++j)
        {
            sf::RectangleShape Cell(sf::Vector2f(cellSize, cellSize));
            Cell.setPosition(offset + j * cellSize, offset + i * cellSize);
            Cell.setOutlineThickness(1.f);
            Cell.setOutlineColor(sf::Color::White);

            if (maze[i][j] == 0) // It's a wall
            {
                Cell.setFillColor(sf::Color(50, 50, 50)); // Dark grey for walls
            }
            else // It's a path (maze[i][j] == 1)
            {
                Cell.setFillColor(sf::Color(200, 200, 200)); // Light grey for paths
            }

            if (i == currentY && j == currentX)
            {
                Cell.setOutlineThickness(4.f);
                Cell.setOutlineColor(sf::Color::Cyan);

                if (isImperfecting)
                {
                    Cell.setFillColor(sf::Color(180, 0, 255));
                }
                else
                    Cell.setFillColor(sf::Color(255, 191, 0));
            }
            if (start[0] != -1 && i == start[1] && j == start[0])
            {
                Cell.setFillColor(sf::Color(0, 180, 0)); // Green for start
            }

            if (End[0] != -1 && i == End[1] && j == End[0])
            {
                Cell.setFillColor(sf::Color(100, 0, 0)); // Red for end point
            }

            window.draw(Cell);
            if (maze[i][j] != 0)
            {
                sf::Text costText;
                costText.setFont(font);
                costText.setCharacterSize(cellSize / 3.f);
                costText.setString(to_string(maze[i][j]));
                sf::FloatRect req = costText.getLocalBounds();
                costText.setOrigin(req.left + req.width / 2.f, req.top + req.height / 2.f);
                costText.setPosition(Cell.getPosition().x + Cell.getSize().x / 2, Cell.getPosition().y + Cell.getSize().y / 2);
                costText.setFillColor(sf::Color::Black);
                window.draw(costText);
            }
        }
    }
}

void shuffle(int array[], int size)
{
    for (int i = size - 1; i > 0; --i)
    {
        int j = rand() % (i + 1); // any random index from i to 0
        if (i != j)
        {
            array[i] ^= array[j];
            array[j] ^= array[i];
            array[i] ^= array[j];
        }
    }
}
bool isDisconnected(int **maze, int row, int col, bool isVertical)
{
    if (isVertical)
        return (maze[row][col] == 1 && maze[row + 2][col] == 1 && maze[row + 1][col] == 0);
    else
        return (maze[row][col] == 1 && maze[row][col + 2] == 1 && maze[row][col + 1] == 0);
}
int chanceProbability(const int &row, const int &col, const int &n, const int &cut_middleOrEdge)
{
    float pi = 3.14159265358979323846f;
    float baseChance = 0.02f, adaptive, finalChance, gradient = 4.8f;
    if (cut_middleOrEdge == 1) // cuts towards center
    {

        float normalized_distance = (float)(row + col) / (2.f * (n - 1));               // this gives 0 to 1 . 0 and 1 at towards corners and 0.5 towards middle
        float how_far_from_main_dia = (float)(abs(row - col)) / (float)(n - 1) * 0.98f; // this is to penalize. like say for n=20 row=18 col=1 will also peak the sin function. so we need to penalize the more the differece between row and col
        float shifting_strength = 0.5;                                                  // need to fine tune this value later
        float shift_amount = shifting_strength * how_far_from_main_dia;                 // the far from main diagonal the more we shift the value to a corner so sin doesnot give peak for those values
        float shifted_distance = normalized_distance;
        if (normalized_distance > .5)
            shifted_distance += shift_amount;
        else
            shifted_distance -= shift_amount;

        float penalty_strength = 0.5;                                // need to tune this for better output
        gradient = 1.0 + (how_far_from_main_dia * penalty_strength); // makes the gradient greater than 1 for better penalization
        adaptive = sinf(shifted_distance * pi) * 0.95;               // multiplying wit 0.95 cz if the value is 1 gradient will have literally  no effect!
        adaptive = pow(adaptive, gradient);
    }
    else // cuts towards the edge
    {
        adaptive = max(pow((float)(row) / (n - 1), gradient), pow((float)col / (n - 1), gradient));
    }
    finalChance = clamp(baseChance + (1.f - baseChance) * adaptive, 0.0f, 0.9f);
    return (finalChance * 100);
}
int calculateLimit(int n)
{
    float baseDensity = 0.01f, gradientBoost, finalDensity;
    gradientBoost = pow((float)(n / 100.f), 2.0); // the bigger n the more imperfections
    finalDensity = baseDensity + gradientBoost * 0.005;
    int limit = finalDensity * n * n;
    return limit;
}
void MakeItImperfect(sf::RenderWindow &window, int **maze, int n)
{

    int count = 0, counter = 0;
    int limit = calculateLimit(n);
    cout << "Limit : " << limit << endl;
    int avg_perRow = limit / n;
    while (count < limit)
    {
        int cut_horizontalORvertiacal = rand() % 2; // to determine if it will cut horizontally or vertical
        // Horizontal
        if (cut_horizontalORvertiacal == 1)
        {
            for (int row = 1; row < n - 1; ++row)
            {
                for (int col = 1; col < n - 2; ++col)
                {
                    int cut_middleOrEdge = rand() % 2;
                    window.clear(sf::Color(30, 30, 30));
                    drawMazeGeneration(window, maze, col, row, n, false);
                    window.display();
                    sf::sleep(sf::milliseconds(animationDelay_ms / n));
                    // sf::sleep(sf::milliseconds(animationDelay_ms));
                    if (isDisconnected(maze, row, col, false))
                    {
                        if ((rand() % 100) < chanceProbability(row, col, n, cut_middleOrEdge))
                        {
                            maze[row][col + 1] = 1;
                            ++count;
                            ++counter;
                            window.clear(sf::Color(30, 30, 30));
                            drawMazeGeneration(window, maze, col + 1, row, n, true);
                            window.display();
                            sf::sleep(sf::milliseconds(animationDelay_ms / (n / 10.f)));
                            cout << "Made " << counter << " Holes at Row : " << row << " Col " << col << " Total made : " << count << endl;
                            // sf::sleep(sf::milliseconds(animationDelay_ms));
                            if (counter > avg_perRow)
                            {
                                counter = 0;
                                row += 2;
                            }
                        }
                    }
                    if (count >= limit)
                        break;
                }
                counter = 0;
                if (count >= limit)
                    break;
            }
        }
        else if (cut_horizontalORvertiacal == 0)
        {
            // Vertical
            for (int row = 1; row < n - 2; ++row)
            {
                for (int col = 1; col < n - 1; ++col)
                {
                    int cut_middleOrEdge = rand() % 2;
                    window.clear(sf::Color(30, 30, 30));
                    drawMazeGeneration(window, maze, col, row, n, false);
                    window.display();
                    sf::sleep(sf::milliseconds(animationDelay_ms / n));
                    // sf::sleep(sf::milliseconds(animationDelay_ms));
                    if (isDisconnected(maze, row, col, true))
                    {
                        if ((rand() % 100) < chanceProbability(row, col, n, cut_middleOrEdge))
                        {
                            maze[row + 1][col] = 1;
                            ++count;
                            ++counter;
                            window.clear(sf::Color(30, 30, 30));
                            drawMazeGeneration(window, maze, col, row + 1, n, true);
                            window.display();
                            sf::sleep(sf::milliseconds(animationDelay_ms / (n / 10.f)));
                            // sf::sleep(sf::milliseconds(animationDelay_ms));
                            cout << "Made " << counter << " Holes at Row : " << row << " Col " << col << " Total made : " << count << endl;
                            if (counter > avg_perRow)
                            {
                                counter = 0;
                                row += 2;
                            }
                        }
                    }
                    if (count >= limit)
                        break;
                }
                counter = 0;
                if (count >= limit)
                    break;
            }
        }
    }
}

// Added bool& window_is_open parameter
void carveMaze(sf::RenderWindow &window, int **maze, bool **visited, int x, int y, int n, bool &window_is_open)
{
    // Flaw 6 Fix: Check if window is open at the very beginning of the function
    if (!window_is_open)
        return;

    visited[y][x] = true;
    maze[y][x] = 1; // Flaw 3 Fix: Use int 1 for path instead of char '.'

    // Flaw 4 Fix: Declare sf::Event once at the top of the function
    sf::Event event;

    window.clear(sf::Color(30, 30, 30));
    drawMazeGeneration(window, maze, x, y, n, false); // Renamed call
    window.display();

    // Flaw 5 Fix: Process events *before* sleeping
    while (window.pollEvent(event))
    {
        if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape))
        {
            window_is_open = false; // Set flag to stop recursion
            window.close();         // Close the window
        }
    }
    // Check again if window was closed during event processing
    if (!window_is_open)
        return;

    sf::sleep(sf::milliseconds(animationDelay_ms / (n / 8.f))); // Flaw 5 Fix: Use sf::milliseconds

    int dirs[] = {0, 1, 2, 3};
    shuffle(dirs, 4);

    for (int d : dirs)
    {
        int nx = x + dx[d] * 2;
        int ny = y + dy[d] * 2;

        if (nx >= 0 && ny >= 0 && nx <= n - 1 && ny <= n - 1 && !visited[ny][nx])
        {
            maze[y + dy[d]][x + dx[d]] = 1; // Flaw 3 Fix: Use int 1 for path

            // Pass window_is_open to the recursive call
            carveMaze(window, maze, visited, nx, ny, n, window_is_open);

            // Flaw 6 Fix: Check if window was closed during the recursive call
            if (!window_is_open)
                return;

            // Backtracking visualization
            window.clear(sf::Color(30, 30, 30));
            drawMazeGeneration(window, maze, x, y, n, false); // Renamed call, show carver returning to (x,y)
            window.display();
            // delayForBacktrack = clamp((10000 / (n * n)), 200, 400);
            sf::sleep(sf::milliseconds(delayForBacktrack / (n / 8.f))); // Flaw 5 Fix: Use sf::milliseconds

            // Process events again after backtracking redraw
            while (window.pollEvent(event)) // Re-use the 'event' object
            {
                if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape))
                {
                    window_is_open = false; // Set flag to stop recursion
                    window.close();         // Close the window
                }
            }
            if (!window_is_open)
                return;
        }
    }
}
void drawRat(sf::RenderWindow &window, int **maze, int currentRow, int currentCol, bool **visited, int n, bool IsDrawingAns)
{
    sf::Font font;
    if (!font.loadFromFile("arial.ttf"))
    {
        cerr << "failed to load text from 'arial.ttf'\n";
    }
    // cout << "Draw called\n";
    for (int row = 0; row < n; ++row)
    {
        for (int col = 0; col < n; ++col)
        {
            sf::RectangleShape Cell(sf::Vector2f(cellSize, cellSize));
            Cell.setPosition(offset + cellSize * col, offset + cellSize * row);
            Cell.setOutlineThickness(1.f);
            Cell.setOutlineColor(sf::Color::White);
            bool isBlocked[4] = {false};
            for (int i = 0; i < 4; ++i)
            {
                if (DeadEnd[row * n + col][i])
                {
                    isBlocked[i] = true;
                }
            }
            if (maze[row][col] == 0) // obstical
            {
                Cell.setFillColor(sf::Color(50, 50, 50));

                window.draw(Cell);
            }

            else // for visited cells
            {

                // If the cell has been visited during the current exploration path
                if (!IsDrawingAns && visited[row][col])
                {
                    Cell.setFillColor(sf::Color(173, 255, 235, 180));  // Pale aqua-teal fill
                    Cell.setOutlineColor(sf::Color(0, 255, 200, 200)); // Vibrant cyan outline
                    Cell.setOutlineThickness(3.5f);
                }

                else if (!IsDrawingAns && TotalVisited[row * n + col])
                {
                    Cell.setFillColor(sf::Color(180, 100, 100)); // Lighter grey for unvisited passable cells
                }
                else if (IsDrawingAns && visited[row][col])
                {
                    if (Path_toRender[0][row * n + col]) // this is for the most optimal path
                    {
                        Cell.setFillColor(sf::Color(255, 249, 120, 200)); // Soft warm yellow light
                        Cell.setOutlineColor(sf::Color(255, 255, 180, 220));
                        Cell.setOutlineThickness(3.5f);
                    }
                    if (Path_toRender[1][row * n + col]) // this is for shortest path
                    {
                        Cell.setFillColor(sf::Color(180, 220, 255, 200)); // Light sky blue
                        Cell.setOutlineColor(sf::Color(140, 255, 255, 230));
                        Cell.setOutlineThickness(3.5f);
                    }
                    if (Path_toRender[2][row * n + col]) // this is for the most costEffective path
                    {
                        Cell.setFillColor(sf::Color(255, 190, 120, 200));    // Peachy orange glow
                        Cell.setOutlineColor(sf::Color(255, 150, 100, 220)); // Warm coppery aura
                        Cell.setOutlineThickness(3.5f);
                    }
                }
                else
                {
                    Cell.setFillColor(sf::Color(200, 200, 200));
                }
                if (start[1] != -1 && row == start[1] && col == start[0])
                    Cell.setFillColor(sf::Color(0, 180, 0));

                if (End[1] != -1 && row == End[1] && col == End[0])
                    Cell.setFillColor(sf::Color(100, 0, 0));

                window.draw(Cell);

                // for drawing the border
                if (!IsDrawingAns)
                {
                    float borderThickness = 7.f;
                    if (isBlocked[0]) // upper
                    {
                        sf::RectangleShape upperBorder(sf::Vector2f(cellSize - 1.f, borderThickness));
                        upperBorder.setFillColor(sf::Color(100, 0, 100));
                        upperBorder.setPosition(Cell.getPosition().x, Cell.getPosition().y - borderThickness - 1.f);
                        upperBorder.setOutlineThickness(1.f);
                        upperBorder.setOutlineColor(sf::Color::White);

                        window.draw(upperBorder);
                    }
                    if (isBlocked[1]) // Right
                    {
                        sf::RectangleShape rightBorder(sf::Vector2f(borderThickness, cellSize - 1.f));
                        rightBorder.setFillColor(sf::Color(100, 0, 100));
                        rightBorder.setPosition(Cell.getPosition().x + cellSize + borderThickness + 1.f, Cell.getPosition().y);
                        rightBorder.setOutlineThickness(1.f);
                        rightBorder.setOutlineColor(sf::Color::White);

                        window.draw(rightBorder);
                    }
                    if (isBlocked[2]) // down
                    {
                        sf::RectangleShape bottomBorder(sf::Vector2f(cellSize - 1.f, borderThickness));
                        bottomBorder.setFillColor(sf::Color(100, 0, 100));
                        bottomBorder.setPosition(Cell.getPosition().x, Cell.getPosition().y + cellSize + borderThickness + 1.f);
                        bottomBorder.setOutlineThickness(1.f);
                        bottomBorder.setOutlineColor(sf::Color::White);

                        window.draw(bottomBorder);
                    }
                    if (isBlocked[3]) // left
                    {
                        sf::RectangleShape leftBorder(sf::Vector2f(borderThickness, cellSize - 1.f));
                        leftBorder.setFillColor(sf::Color(100, 0, 100));
                        leftBorder.setPosition(Cell.getPosition().x - borderThickness - 1.f, Cell.getPosition().y);
                        leftBorder.setOutlineThickness(1.f);
                        leftBorder.setOutlineColor(sf::Color::White);

                        window.draw(leftBorder);
                    }
                }
                sf::Text costText;
                costText.setFont(font);
                costText.setCharacterSize(cellSize / 3.f);
                costText.setString(to_string(maze[row][col]));
                sf::FloatRect req = costText.getLocalBounds();
                costText.setOrigin(req.left + req.width / 2.f, req.top + req.height / 2.f);
                costText.setPosition(Cell.getPosition().x + Cell.getSize().x / 2, Cell.getPosition().y + Cell.getSize().y / 2);
                costText.setFillColor(sf::Color::Black);

                window.draw(costText);
            }
        }
    }

    int baseSize = max(16, 25 - (15 / n));
    float top = 10.f;
    float space = 40.f;
    vector<sf::Text> info(5);
    vector<string> content =
        {
            "Total Path cell: " + to_string(totalPath),
            "Total searched cell: " + to_string(totalTraversed),
            (!IsDrawingAns ? "Total cost: " : "Minimum cost: ") + to_string(!IsDrawingAns ? totalCost : minimumCost),
            (!IsDrawingAns ? "Current path length: " : "Shortest length: ") + to_string(!IsDrawingAns ? currentPathLength : shortestLen),
            "Shortest length :" + ((gotSOl) ? to_string(shortestLen) : "Unknown")};
    float totalWidth = 0.f;

    for (int i = 0; i < 5; ++i)
    {
        info[i].setFont(font);
        info[i].setCharacterSize(baseSize);
        info[i].setFillColor(sf::Color::White);
        info[i].setString(content[i]);
        if (i < 4)
        {
            sf::FloatRect bound = info[i].getLocalBounds();
            totalWidth += bound.width;
        }
    }
    totalWidth += 3 * space;
    while (totalWidth > window.getSize().x - 20.f && baseSize > 12)
    {
        baseSize -= 1;
        totalWidth = 0;
        for (int i = 0; i < 5; ++i)
        {
            if (i >= 4)
            {

                info[i].setCharacterSize(baseSize / 1.5);
                continue;
            }
            info[i].setCharacterSize(baseSize);
            sf::FloatRect bound = info[i].getLocalBounds();
            totalWidth += bound.width;
        }
        totalWidth += space * 3;
    }
    float startX = (window.getSize().x - totalWidth) / 2.f;
    float startX2 = 10.f;
    vector<sf::CircleShape> ColorIndicator(3);
    vector<sf::Text> ColorIndicatorText(4);
    vector<string> s = {
        IsDrawingAns ? "Most optimal Path  " : "Current Path  ", IsDrawingAns ? "Shortest Path  " : "Explored cells  ", IsDrawingAns ? "Most cost efficient Path  " : "Unexplored Cells  ", "Dead End  "};
    for (int i = 0; i < 4; ++i)

    {
        info[i].setPosition(startX, top);
        if (i < 3)
        {
            ColorIndicator[i].setRadius(10.f);
            ColorIndicator[i].setOrigin(10.f, 10.f);
            if (i == 0)
                IsDrawingAns ? ColorIndicator[i].setFillColor(sf::Color(255, 249, 120, 200)) : ColorIndicator[i].setFillColor(sf::Color(173, 255, 235, 180));
            else if (i == 1)
                IsDrawingAns ? ColorIndicator[i].setFillColor(sf::Color(180, 220, 255, 200)) : ColorIndicator[i].setFillColor(sf::Color(180, 100, 100));
            else if (i == 2)
                IsDrawingAns ? ColorIndicator[i].setFillColor(sf::Color(255, 190, 120, 200)) : ColorIndicator[i].setFillColor(sf::Color(200, 200, 200));
            ColorIndicatorText[i].setFont(font);
            ColorIndicatorText[i].setCharacterSize(20.f);
            ColorIndicatorText[i].setFillColor(sf::Color::White);
            ColorIndicatorText[i].setString(s[i]);
            ColorIndicatorText[i].setPosition(startX2, window.getSize().y - 40.f);
            ColorIndicator[i].setPosition(startX2 + ColorIndicatorText[i].getLocalBounds().width + 10.f, window.getSize().y - 30.f);

            window.draw(ColorIndicatorText[i]);
            window.draw(ColorIndicator[i]);
            startX2 += ColorIndicator[i].getLocalBounds().width + ColorIndicatorText[i].getLocalBounds().width + space;
        }
        startX += info[i].getLocalBounds().width + space;
        window.draw(info[i]);
    }
    ColorIndicatorText[3].setFont(font);
    ColorIndicatorText[3].setCharacterSize(20.f);
    ColorIndicatorText[3].setFillColor(sf::Color::White);
    ColorIndicatorText[3].setString(s[3]);
    ColorIndicatorText[3].setPosition(startX2, window.getSize().y - 40.f);

    sf::RectangleShape Dead_end_Indicator(sf::Vector2f(7.f, cellSize));
    Dead_end_Indicator.setFillColor(sf::Color(100, 0, 100));
    startX2 += ColorIndicatorText[3].getLocalBounds().width + 10.f;
    Dead_end_Indicator.setPosition(startX2, window.getSize().y - 40.f);
    startX2 += Dead_end_Indicator.getLocalBounds().width + 10.f;
    info[4].setPosition(startX2, window.getSize().y - 40.f);

    window.draw(ColorIndicatorText[3]);
    window.draw(Dead_end_Indicator);
    window.draw(info[4]);

    float ratX, ratY;
    ratX = offset + currentCol * cellSize + cellSize / 2.f;
    ratY = offset + currentRow * cellSize + cellSize / 2.f;
    sf::CircleShape Rat;
    Rat.setRadius(cellSize / 3.f);
    Rat.setOrigin(Rat.getRadius(), Rat.getRadius());
    Rat.setPointCount(70); // how much smooth
    Rat.setPosition(ratX, ratY);
    Rat.setFillColor(sf::Color(102, 255, 255, 220));
    Rat.setOutlineThickness(2.5f);
    Rat.setOutlineColor(sf::Color(0, 180, 255, 180)); // Electric blue glow
    window.draw(Rat);

    // cout << "Drew rat at " << ratX << " , " << ratY << endl;
}
bool isValid(int **maze, int row, int col, int n, bool **isVisited)
{
    // Check bounds first!
    if (row < 0 || col < 0 || row >= n || col >= n)
        return false;
    if (maze[row][col] == 0)
        return false;
    if (isVisited[row][col])
        return false;
    return true;
}
bool Explored(int **maze, int row, int col, int n)
{
    return (row >= 0 && row < n && col >= 0 && col < n && TotalVisited[row * n + col] && (col > 0 ? (TotalVisited[row * n + col - 1] || maze[row][col - 1] == 0) : true) && (col < n - 1 ? (TotalVisited[row * n + col + 1] || maze[row][col + 1] == 0) : true) && (row < n - 1 ? (TotalVisited[(row + 1) * n + col] || maze[row + 1][col] == 0) : true) && (row > 0 ? (TotalVisited[(row - 1) * n + col] || maze[row - 1][col] == 0) : true)); // checks if all neibour along with the cell is visited! then it is loop
}
void Pre_determineDeadEnds(int **maze, int n) // this thing is giving bugs!
{
    for (int row = 1; row < n - 1; ++row) // Avoid border walls
    {
        for (int col = 1; col < n - 1; ++col)
        {
            if (maze[row][col] == 0)
                continue; // skip wall cells

            cout << "Checking cell (" << row << ", " << col << ")...\n";
            // UP direction
            if (maze[row - 1][col] == 0)
            {
                DeadEnd[row * n + col][0] = true;
                cout << "\tUP is blocked\n";
            }
            // DOWN direction
            if (maze[row + 1][col] == 0)
            {
                DeadEnd[row * n + col][3] = true;
                cout << "\tDOWN is blocked\n";
            }
            // LEFT direction
            if (maze[row][col - 1] == 0)
            {
                DeadEnd[row * n + col][2] = true;
                cout << "\tLEFT is blocked\n";
            }
            // RIGHT direction
            if (maze[row][col + 1] == 0)
            {
                DeadEnd[row * n + col][1] = true;
                cout << "\tRIGHT is blocked\n";
            }
        }
    }
}

bool solve(sf::RenderWindow &window, int **maze, vector<pair<int, vector<char>>> &ans, vector<char> &path, bool **isVisited, int currentCol, int currentRow, int n, int &bestCost, int &shortestLength, int &costEffective, int depth = 0)
{
    if (gotSOl && isPerfect)
        return true;
    if (gotSOl && (totalCost > bestCost) && ((currentPathLength + (abs(currentCol-End[0]))) > shortestLength && (currentPathLength + (abs(currentCol-End[1]))) > shortestLength))
        return true; // not false becz then it will be marked as dead end. but through other road the cell could lead to better solution
    if (!window.isOpen())
        return false;

    window.clear(sf::Color(30, 30, 30));
    drawRat(window, maze, currentRow, currentCol, isVisited, n, false);
    window.display();
    sf::sleep(sf::milliseconds((animationDelay_ms / (n / 8.88))));

    isVisited[currentRow][currentCol] = true;
    sf::Event event;
    while (window.pollEvent(event))
    {
        if (event.type == sf::Event::Closed ||
            (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape))
        {
            window.close();
            return false;
        }
    }

    // if(Explored(maze,currentRow,currentCol,n,isVisited)) return false; // is in loop

    if (currentCol == End[0] && currentRow == End[1]) // reached goal
    {

        gotSOl = true;
        if (!isPerfect)
        {
            //  for most optimal
            if (totalCost < bestCost)
            {
                bestCost = totalCost;
                ans.erase(remove_if(ans.begin(), ans.end(), [](const pair<int, vector<char>> &p)
                                    { return p.first == 0; }),
                          ans.end());
                ans.push_back({0, path});
                minimumCost = totalCost;
                cout << "Found a better path. Minimum cost is " << minimumCost << endl;
            }
            // for shortest path
            if (currentPathLength < shortestLength)
            {
                shortestLength = currentPathLength;
                ans.erase(remove_if(ans.begin(), ans.end(), [](const pair<int, vector<char>> &p)
                                    { return p.first == 1; }),
                          ans.end());
                ans.push_back({1, path});
                shortestLen = shortestLength;
                cout << "Found shorter path" << endl;
            }
            // for most cost effective
            if (totalCost - currentPathLength < costEffective)
            {
                costEffective = totalCost - currentPathLength;
                ans.erase(remove_if(ans.begin(), ans.end(), [](const pair<int, vector<char>> &p)
                                    { return p.first == 2; }),
                          ans.end());
                ans.push_back({2, path});
            }
        }
        else
        {
            for (auto it : path)
            {
                ansPath.push_back(it);
            }
        }
        return true;
    }

    vector<intelligence> choose;
    char dir[4] = {'U', 'R', 'D', 'L'};

    for (int i = 0; i < 4; ++i)
    {
        int nextRow = currentRow + dy[i];
        int nextCol = currentCol + dx[i];
        if (isValid(maze, nextRow, nextCol, n, isVisited))
        {

            if (DeadEnd[currentRow * n + currentCol][i])
                continue; // this direction is dead end
            intelligence intel;
            intel.dir = dir[i];
            intel.index = i;
            double dist = getDistance(nextCol, End[0], nextRow, End[1]);
            double safeDist = max((float)dist, 0.f);                                 // so alpha does not get to infinity! we can not take distance as 0
            double alpha = log(safeDist + 1);                                        // the more distance the greater alpha
            double weight = pow(1.0 / (alpha + 1), 0.7);                             // distance more means less weight
            dist /= getDistance(start[0], End[0], start[1], End[1]);                 // to normalize the distance from 0 to 1
            double cost_normallized = (double)maze[nextRow][nextCol] / max_cellCost; // to normalize the cost from 0 to 1
            intel.direction_importance = dist * weight + cost_normallized * (1 - weight);
            choose.push_back(intel);
        }
    }

    sort(choose.begin(), choose.end(), [](const intelligence &a, const intelligence &b)
         { return a.direction_importance < b.direction_importance; });

    bool leadsToans = false;

    for (auto &a : choose)
    {
        int nextRow = currentRow + dy[a.index];
        int nextCol = currentCol + dx[a.index];
        path.push_back(a.dir);
        isVisited[nextRow][nextCol] = true;
        if (!TotalVisited[nextRow * n + nextCol])
        {
            totalTraversed++;
            TotalVisited[nextRow * n + nextCol] = true;
        }
        totalCost += maze[nextRow][nextCol] + 1; // path length increases by 1 for each call
        // cout << "total cost is " << totalCost << endl;
        currentPathLength++;

        leadsToans = (solve(window, maze, ans, path, isVisited, nextCol, nextRow, n, bestCost, shortestLength, costEffective, depth + 1) || leadsToans);
        if (!leadsToans)
        {
            auto key = make_tuple(currentCol, currentRow, a.index); // or encodeKey(...)
            if (deadEndMap.find(key) == deadEndMap.end())
            {
                deadEndMap[key] = true; // Mark as recorded
                // cout<<"Saved in the map ";
            }
            else
            {
                cout << "Duplicate for cell: " << currentRow << ", " << currentCol << " direction: " << a.index << endl;
            }

            DeadEnd[currentRow * n + currentCol][a.index] = true;
            // cout << "Is dead end for For cell :" << currentRow << " , " << currentCol << " For direction : " << a.index << endl;
        }
        path.pop_back();
        isVisited[nextRow][nextCol] = false;
        currentPathLength--;
        totalCost -= (maze[nextRow][nextCol] + 1);
        // cout << "after backtrack total path lenght is " << currentPathLength << endl;
        window.clear(sf::Color(30, 30, 30));
        drawRat(window, maze, currentRow, currentCol, isVisited, n, false);
        window.display();
        sf::sleep(sf::milliseconds((delayForBacktrack / (n / 8.88))));
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape))
            {
                window.close(); // Close the window
            }
        }
        if (!window.isOpen())
            return false;
    }

    return leadsToans;
}
void setCost(int **maze, int n)
{
    for (int i = 1; i < n - 1; ++i)
    {
        for (int j = 1; j < n - 1; ++j)
        {
            if (maze[i][j] == 1)
                maze[i][j] = rand() % max_cellCost + 1;
        }
    }
}
void drawTheAns(sf::RenderWindow &window, const vector<pair<int, vector<char>>> &ans, int **maze, bool **isVisited, int n);
int main()
{
    cout << "Is it debug mode? yes --> 1 no --> 0\n";
    int a = 0;
    while (1)
    {
        cin >> a;
        if (a == 1)
        {
            Isdebug = true;
            break;
        }
        else if (a == 0)
            break;
    }
    if (Isdebug)
        srand(125); // Seed the random number generator
    else
        srand(time(0)); // Seed the random number generator
    vector<pair<int, vector<char>>> ans;
    int n;
    cout << "Enter maze size (N): ";
    cin >> n;
    // Maze size should be odd for proper wall structure in this algorithm
    if (n % 2 == 0)
    {
        ++n;
        cout << "Maze size adjusted to " << n << " (must be odd for proper generation).\n";
    }
    cout << "1 for perfect maze 0 for imperfect maze\n";
    while (1)
    {
        int x;
        cin >> x;
        if (x == 0)
        {
            isPerfect = false;
            break;
        }

        else if (x == 1)
        {
            isPerfect = true;
            break;
        }
        else
            continue;
    }
    cellSize = (60 * 15) / n;
    // Adaptive delay: total time / number of cells
    animationDelay_ms = clamp((1500.f / (n * n)), 150.f, 250.f);
    delayForBacktrack = clamp((700.f / (n * n)), 100.f, 200.f);
    cout<<"Rapid simulation or slow? yes --> 1 no --> 0"<<endl;
    while (1)
    {
        cin >> a;
        if (a == 1)
        {
            animationDelay_ms/=2.f;
            delayForBacktrack/=2.f;
            break;
        }
        else if (a == 0)
            break;
    }
    // Calculate window size based on maze size and cell size
    int windowWidth = static_cast<int>(n * cellSize + 2 * offset);
    int windowHeight = static_cast<int>(n * cellSize + 2 * offset);

    // Create the SFML window
    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Maze Generation Simulation", sf::Style::Close);
    window.setFramerateLimit(60); // Limit framerate for smoother animation

    // Flag to indicate if the SFML window is still open (passed by reference to carveMaze)
    bool window_is_open = window.isOpen();

    // Dynamically allocate 2D array for the maze grid (int type)
    int **grid = new int *[n];
    for (int i = 0; i < n; ++i)
    {
        grid[i] = new int[n];
    }

    // Dynamically allocate 2D array for visited cells (bool type)
    bool **isVisited = new bool *[n];
    for (int i = 0; i < n; ++i)
    {
        isVisited[i] = new bool[n];
    }

    // Initialize all cells to walls (0) and unvisited (false)
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            grid[i][j] = 0;          // 0 for wall
            isVisited[i][j] = false; // Not visited
        }
    }
    for (int i = 0; i < n * n; ++i)
    {
        TotalVisited.push_back(false);
    }
    DeadEnd.resize(n * n, vector<bool>(4, false));
    // Flaw 7 Fix: Set the starting point for carving (usually an odd coordinate like (1,1))
    // The algorithm ensures outer border cells (0,X), (X,0), (N-1,X), (X,N-1) remain walls.
    int startCarvingX = 1;
    int startCarvingY = 1;
    // No need to set grid[1][0] or grid[5][6] here; the algorithm carves paths.

    // --- Initial Draw before starting the carving ---
    window.clear(sf::Color(30, 30, 30));
    drawMazeGeneration(window, grid, startCarvingX, startCarvingY, n, false); // Show initial state with carver at start
    window.display();
    sf::sleep(sf::milliseconds(100)); // Pause briefly before starting generation

    // --- Start the maze carving simulation ---
    // Flaw 7 Fix: Pass window_is_open to carveMaze
    carveMaze(window, grid, isVisited, startCarvingX, startCarvingY, n, window_is_open);
    if (!isPerfect)
        MakeItImperfect(window, grid, n);
    setCost(grid, n);
    window.clear(sf::Color(30, 30, 30));
    drawMazeGeneration(window, grid, startCarvingX, startCarvingY, n, false); // Show initial state with carver at start
    window.display();

    // Pre_determineDeadEnds(grid,n);

    cout << "\nMaze generation complete (or window closed).\n";
    // Prompt for solving or exiting BEFORE closing the window
    cout << "For solving the maze press 's' or press 'e' to end" << endl;
    char x;
    vector<char> path;
    bool doSolve = false;
    while (window.isOpen())
    {

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape))
            {
                window.close();
            }
        }

        cin >> x;
        if (x == 's' || x == 'S')
        {
            doSolve = true;
            break;
        }
        else if (x == 'e' || x == 'E')
        {
            doSolve = false;
            break;
        }
    }

    if (doSolve)
    {
        cout << "Select Starting point\n";
        bool WantMouseInput = true;
        bool selectStart = true;
        while (window.isOpen() && WantMouseInput)
        {
            sf::Event event;
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape))
                {
                    window.close();
                }

                if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
                {
                    sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
                    sf::Vector2f worldPos = window.mapPixelToCoords(pixelPos);
                    int col = (worldPos.x - offset) / cellSize;
                    int row = (worldPos.y - offset) / cellSize;

                    if (col >= 0 && col < n && row >= 0 && row < n && (grid[row][col] >= 1 || col == 0 || row == 0 || row == n - 1 || col == n - 1) && !((row == 0 && col == 0) || (row == n - 1 && col == n - 1) || (row == n - 1 && col == 0) || (row == 0 && col == n - 1)))
                    {
                        if (selectStart)
                        {
                            start[0] = col;
                            start[1] = row;
                            selectStart = false;
                            if (col == 0 || row == 0 || row == n - 1 || col == n - 1)
                                grid[row][col] = 1;
                            cout << "Starting point selected at: (" << start[1] << ", " << start[0] << ")\nSelect End point" << endl;
                        }
                        else if (!selectStart && !(col == start[0] && row == start[1]))
                        {

                            End[0] = col;
                            End[1] = row;
                            WantMouseInput = false;
                            if (col == 0 || row == 0 || row == n - 1 || col == n - 1)
                                grid[row][col] = 1;
                            cout << "End point selected at: (" << End[1] << ", " << End[0] << ")" << endl; // This line is present.
                            break;
                        }
                    }
                }
            }
            window.clear(sf::Color(30, 30, 30));
            drawMazeGeneration(window, grid, startCarvingX, startCarvingY, n, false); // Show initial state with carver at start
            window.display();
        }
        totalCost += grid[start[1]][start[0]];
        // Reset visited array
        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < n; ++j)
                isVisited[i][j] = false;
        }
        // counting total path number
        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < n; ++j)
            {
                if (grid[i][j] != 0)
                    ++totalPath;
            }
        }
        // Show initial state
        window.clear(sf::Color(30, 30, 30));
        drawRat(window, grid, start[1], start[0], isVisited, n, false);
        window.display();
        sf::sleep(sf::seconds(1));
        int bestCost = INT_MAX;
        int shortestPath = INT_MAX;
        int costEffective = bestCost;
        solve(window, grid, ans, path, isVisited, start[0], start[1], n, bestCost, shortestPath, costEffective);
        cout << "Press ESC or cross to exit\n";
        if (isPerfect)
        {

            for (int i = 0; i < n; ++i)
            {
                for (int j = 0; j < n; ++j)
                    isVisited[i][j] = false;
            }
            isVisited[start[1]][start[0]] = true;

            int currentX_anim = start[0];
            int currentY_anim = start[1];

            // Draw initial state (rat at start)
            window.clear(sf::Color(30, 30, 30));
            drawRat(window, grid, start[1], start[0], isVisited, n, false);
            window.display();
            sf::sleep(sf::milliseconds(animationDelay_ms / (n / 7.f)));

            for (char dir : ansPath)
            {
                sf::Event animEvent;
                while (window.pollEvent(animEvent))
                {
                    if (animEvent.type == sf::Event::Closed || (animEvent.type == sf::Event::KeyPressed && animEvent.key.code == sf::Keyboard::Escape))
                    {
                        window.close();
                        goto cleanup;
                    }
                }
                if (!window.isOpen())
                    break;

                if (dir == 'R')
                    currentX_anim++;
                else if (dir == 'L')
                    currentX_anim--;
                else if (dir == 'D')
                    currentY_anim++;
                else if (dir == 'U')
                    currentY_anim--;

                isVisited[currentY_anim][currentX_anim] = true;

                window.clear(sf::Color(30, 30, 30));
                drawRat(window, grid, currentY_anim, currentX_anim, isVisited, n, false);
                window.display();
                sf::sleep(sf::milliseconds(animationDelay_ms / (n / 7.f)));
            }
        }
        else
        {
            drawTheAns(window, ans, grid, isVisited, n);
            if (!window.isOpen())
                goto cleanup;
        }

        // Final Display Loop (after all animations are done)
        while (window.isOpen())
        {
            sf::Event event;
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape))
                {
                    window.close();
                }
            }
            window.clear(sf::Color(30, 30, 30));

            drawRat(window, grid, End[1], End[0], isVisited, n, isPerfect ? false : true);
            window.display();
            sf::sleep(sf::milliseconds(animationDelay_ms / (n / 7.f)));
        }
    }
    else // User chose not to solve (pressed 'e')
    {
        cout << "Press ESC or cross to exit\n";
        while (window.isOpen())
        {
            sf::Event event;
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape))
                {
                    window.close();
                }
            }
            window.clear(sf::Color(30, 30, 30));
            drawMazeGeneration(window, grid, -1, -1, n, false); // Draw static maze
            window.display();
        }
    }
    // --- Clean up dynamically allocated memory ---
cleanup:
    for (int i = 0; i < n; ++i)
    {
        delete[] grid[i];
        delete[] isVisited[i];
    }
    delete[] grid;
    delete[] isVisited;
    return 0;
}

void drawTheAns(sf::RenderWindow &window, const vector<pair<int, vector<char>>> &ans, int **maze, bool **isVisited, int n)
{
    cout << "ans print called" << endl;
    Route.clear();
    Route.resize(3);
    // Iterate through each solution found in 'ans'
    for (const auto &ans_pair : ans)
    {
        int type = ans_pair.first;
        if (type >= 3)
            break;
        int currentX = start[0];
        int currentY = start[1];
        vector<int> currentPathCells;
        currentPathCells.push_back(currentY * n + currentX); // Add the starting cell

        for (char dir : ans_pair.second)
        {
            if (dir == 'R')
                currentX++;
            else if (dir == 'L')
                currentX--;
            else if (dir == 'D')
                currentY++;
            else if (dir == 'U')
                currentY--;
            currentPathCells.push_back(currentY * n + currentX); // Add the cell after move
        }

        Route[type] = currentPathCells;
        ++type;
    }
    Path_toRender.assign(3, vector<bool>(n * n, false));
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < n; ++j)
            isVisited[i][j] = false;
    }
    for (int rank = 0; rank < Route.size(); ++rank)
    {
        for (int cell_indx : Route[rank])
        {
            isVisited[cell_indx / n][cell_indx % n] = true;
            Path_toRender[rank][cell_indx] = true;

            window.clear(sf::Color(30, 30, 30));
            // cout<<"Path type is "<<rank<<endl;
            drawRat(window, maze, cell_indx / n, cell_indx % n, isVisited, n, true);
            window.display();
            sf::sleep(sf::milliseconds(animationDelay_ms));
            sf::Event event;
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape))
                {
                    window.close();
                    return;
                }
            }
        }
    }
    sf::Event event;
    while (window.isOpen())
    {
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape))
            {
                window.close();
                return;
            }
        }
        window.clear(sf::Color(30, 30, 30));
        drawRat(window, maze, End[1], End[0], isVisited, n, true);
        window.display();
        sf::sleep(sf::milliseconds(animationDelay_ms));
    }
}