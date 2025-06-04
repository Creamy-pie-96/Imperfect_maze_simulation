#include <iostream>
#include <SFML/Graphics.hpp>
#include <ctime>
#include <vector> // For std::vector (though raw pointers are still used for maze/visited)
#include <cmath>  // For abs (not strictly needed here)

using namespace std;

// Forward declaration for ratInmaze (will be removed later if not used)
// int ratInmaze(vector<vector<int>> maze); // REMOVED: Not defined in this file, causes linker error.

int dx[4] = {0, 1, 0, -1}; // Change in row (for y-coordinate)
int dy[4] = {-1, 0, 1, 0}; // Change in column (for x-coordinate)

float cellSize;
const float offset = 50.f;
float animationDelay_ms; // Milliseconds
float delayForBacktrack;
int End[2]{-1, -1};
int start[2] = {-1, -1};
vector<bool> TotalVisited;
int totalTraversed = 1;
int currentPathLength = 1;
int totalPath = 0;
struct intelligence
{
    char dir;
    int index;
    int distance;
};

int getDistance(int x1, int x2, int y1, int y2)
{
    return abs(x1 - x2) + abs(y1 - y2);
}
void drawMazeGeneration(sf::RenderWindow &window, int **maze, int currentX, int currentY, int size)
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
            Cell.setOutlineThickness(1.f);          // Default thin border
            Cell.setOutlineColor(sf::Color::White); // Default white border

            if (maze[i][j] == 0) // It's a wall
            {
                Cell.setFillColor(sf::Color(50, 50, 50)); // Dark grey for walls
            }
            else // It's a path (maze[i][j] == 1)
            {
                Cell.setFillColor(sf::Color(200, 200, 200)); // Light grey for paths
            }

            // Now, apply the special highlight if it's the current carving position
            if (i == currentY && j == currentX)
            {
                Cell.setOutlineThickness(4.f);         // Thicker border
                Cell.setOutlineColor(sf::Color::Cyan); // Cyan border
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
        }
    }
}

void shuffle(int array[], int size)
{
    for (int i = size - 1; i > 0; --i)
    {
        int j = rand() % (i + 1); // any random index from i to 0
        if (i != j)               // protect XOR swap from zeroing
        {
            array[i] ^= array[j];
            array[j] ^= array[i];
            array[i] ^= array[j];
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
    drawMazeGeneration(window, maze, x, y, n); // Renamed call
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
            drawMazeGeneration(window, maze, x, y, n); // Renamed call, show carver returning to (x,y)
            window.display();
            // delayForBacktrack = std::clamp((10000 / (n * n)), 200, 400);
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
void drawRat(sf::RenderWindow &window, int **maze, int currentRow, int currentCol, bool **visited, int n)
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

            if (maze[row][col] == 0) // obstical
            {
                Cell.setFillColor(sf::Color(50, 50, 50));
            }
            else // for visited cells
            {
                // If the cell has been visited during the current exploration path
                if (visited[row][col])
                {
                    Cell.setFillColor(sf::Color(173, 255, 235, 180));  // Pale aqua-teal fill
                    Cell.setOutlineColor(sf::Color(0, 255, 200, 200)); // Vibrant cyan outline
                    Cell.setOutlineThickness(3.5f);
                }
                else if (TotalVisited[row * n + col])
                {
                    Cell.setFillColor(sf::Color(180, 100, 100)); // Lighter grey for unvisited passable cells
                }
                else
                {
                    Cell.setFillColor(sf::Color(200, 200, 200));
                }
            }
            if (start[1] != -1 && row == start[1] && col == start[0])
                Cell.setFillColor(sf::Color(0, 180, 0));
            if (End[1] != -1 && row == End[1] && col == End[0])
                Cell.setFillColor(sf::Color(100, 0, 0));
            window.draw(Cell);
        }
    }

    sf::Text text1;
    text1.setFont(font);
    text1.setFillColor(sf::Color::White);
    text1.setCharacterSize(25);
    text1.setString("Total cell: " + to_string(totalPath));
    text1.setPosition(10.f, 10.f);
    sf::Text text2;
    text2.setFont(font);
    text2.setFillColor(sf::Color::White);
    text2.setCharacterSize(25);
    text2.setString("Total searched cell: " + to_string(totalTraversed));
    text2.setPosition(window.getSize().x / 3.f - 25.f, 10.f);
    sf::Text text3;
    text3.setFont(font);
    text3.setFillColor(sf::Color::White);
    text3.setCharacterSize(25);
    text3.setString("Current path length: " + to_string(currentPathLength));
    text3.setPosition(window.getSize().x - 300.f, 10.f);
    window.draw(text1);
    window.draw(text2);
    window.draw(text3);
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
bool solve(sf::RenderWindow &window, int **maze, vector<char> &path, bool **isVisited, int currentCol, int currentRow, int n)
{
    isVisited[currentRow][currentCol] = true;
    if (!window.isOpen())
        return false;

    window.clear(sf::Color(30, 30, 30));
    drawRat(window, maze, currentRow, currentCol, isVisited, n);
    window.display();
    sf::sleep(sf::milliseconds(animationDelay_ms / (n / 8.88)));

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

    if (currentCol == End[0] && currentRow == End[1])
        return true; // Goal reached

    vector<intelligence> choose;
    char dir[4] = {'U', 'R', 'D', 'L'};

    for (int i = 0; i < 4; ++i)
    {
        int nextRow = currentRow + dy[i];
        int nextCol = currentCol + dx[i];
        if (isValid(maze, nextRow, nextCol, n, isVisited))
        {
            intelligence intel;
            intel.dir = dir[i];
            intel.index = i;
            intel.distance = getDistance(nextCol, End[0], nextRow, End[1]);
            choose.push_back(intel);
        }
    }

    sort(choose.begin(), choose.end(), [](const intelligence &a, const intelligence &b)
         { return a.distance < b.distance; });

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
        currentPathLength++;

        if (solve(window, maze, path, isVisited, nextCol, nextRow, n))
            return true;

        path.pop_back();
        isVisited[nextRow][nextCol] = false;
        currentPathLength--;
        window.clear(sf::Color(30, 30, 30));
        drawRat(window, maze, currentRow, currentCol, isVisited, n);
        window.display();
        sf::sleep(sf::milliseconds(delayForBacktrack / (n / 8.88)));
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

    return false;
}

int main()
{
    srand(time(0)); // Seed the random number generator

    int n;
    cout << "Enter maze size (N): ";
    cin >> n;

    // Maze size should be odd for proper wall structure in this algorithm
    if (n % 2 == 0)
    {
        ++n;
        cout << "Maze size adjusted to " << n << " (must be odd for proper generation).\n";
    }
    cellSize = (60 * 15) / n;
    // Adaptive delay: total time / number of cells
    animationDelay_ms = std::clamp((1500.f / (n * n)), 150.f, 250.f);
    delayForBacktrack = std::clamp((700.f / (n * n)), 100.f, 200.f);
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
    // Flaw 7 Fix: Set the starting point for carving (usually an odd coordinate like (1,1))
    // The algorithm ensures outer border cells (0,X), (X,0), (N-1,X), (X,N-1) remain walls.
    int startCarvingX = 1;
    int startCarvingY = 1;
    // No need to set grid[1][0] or grid[5][6] here; the algorithm carves paths.

    // --- Initial Draw before starting the carving ---
    window.clear(sf::Color(30, 30, 30));
    drawMazeGeneration(window, grid, startCarvingX, startCarvingY, n); // Show initial state with carver at start
    window.display();
    sf::sleep(sf::seconds(1)); // Pause briefly before starting generation

    // --- Start the maze carving simulation ---
    // Flaw 7 Fix: Pass window_is_open to carveMaze
    carveMaze(window, grid, isVisited, startCarvingX, startCarvingY, n, window_is_open);

    cout << "\nMaze generation complete (or window closed).\n";
    // grid[0][0]=1;
    // grid[n-1][n-1]=1;
    // Prompt for solving or exiting BEFORE closing the window
    cout << "For solving the maze press 's' or press 'e' to end" << endl;
    char x;
    vector<char> path;
    bool doSolve = false;
    while (true)
    {
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

                    if (col >= 0 && col < n && row >= 0 && row < n && (grid[row][col] == 1 || col == 0 || row == 0))
                    {
                        if (selectStart)
                        {
                            start[0] = col;
                            start[1] = row;
                            selectStart = false;
                            grid[row][col] = 1;
                            cout << "Starting point selected at: (" << start[1] << ", " << start[0] << ")\nSelect End point" << endl;
                        }
                        else if (!selectStart)
                        {

                            End[0] = col;
                            End[1] = row;
                            WantMouseInput = false;
                            grid[row][col] = 1;
                            cout << "End point selected at: (" << End[1] << ", " << End[0] << ")" << endl; // This line is present.
                            break;
                        }
                    }
                }
            }
            window.clear(sf::Color(30, 30, 30));
            drawMazeGeneration(window, grid, startCarvingX, startCarvingY, n); // Show initial state with carver at start
            window.display();
        }
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
                if (grid[i][j] == 1)
                    ++totalPath;
            }
        }
        // Show initial state
        window.clear(sf::Color(30, 30, 30));
        drawRat(window, grid, start[1], start[0], isVisited, n);
        window.display();
        sf::sleep(sf::seconds(1));
        solve(window, grid, path, isVisited, start[0], start[1], n);
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
            drawRat(window, grid, End[1], End[0], isVisited, n);
            window.display();
        }
    }
    else
    {
        cout << "Press ESC or cross to exit\n";
        // If user chose not to solve, just display the maze until window is closed
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
            drawMazeGeneration(window, grid, -1, -1, n);
            window.display();
        }
    }
    // --- Clean up dynamically allocated memory ---
    for (int i = 0; i < n; ++i)
    {
        delete[] grid[i];
        delete[] isVisited[i];
    }
    delete[] grid;
    delete[] isVisited;
    return 0;
}
