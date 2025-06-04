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

// Renamed from Print_maze for clarity and consistency.
// Removed 'bool isEnd' parameter and related cost text logic.
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
                Cell.setOutlineThickness(3.f);         // Thicker border
                Cell.setOutlineColor(sf::Color::Cyan); // Cyan border
                Cell.setFillColor(sf::Color(255, 255, 0));
            }
            window.draw(Cell);
            // REMOVED: if (isEnd) block and sf::Text costText logic
            // Maze generation does not involve costs or an 'isEnd' display.
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

    sf::sleep(sf::milliseconds(animationDelay_ms)); // Flaw 5 Fix: Use sf::milliseconds

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
            int delayForBacktrack = std::clamp((10000 / (n * n)), 200, 400); 
            sf::sleep(sf::milliseconds(delayForBacktrack)); // Flaw 5 Fix: Use sf::milliseconds

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
    cellSize = (60 * 15) / n;         // this also might need to chage!
    const int totalSimTime_ms = 20000; // 7 seconds of glory

    // Cap minimum and maximum animation delay per cell
    const int minDelay_ms = 300;  // Minimum visibility for humans
    const int maxDelay_ms = 500; // Avoid snail mode on small mazes

    // Total number of cells or steps
    int totalSteps = n * n;

    // Adaptive delay: total time / number of cells
    animationDelay_ms = std::clamp(totalSimTime_ms / totalSteps, minDelay_ms, maxDelay_ms);
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

    // Flaw 7 Fix: Set the starting point for carving (usually an odd coordinate like (1,1))
    // The algorithm ensures outer border cells (0,X), (X,0), (N-1,X), (X,N-1) remain walls.
    int startCarvingX = 0;
    int startCarvingY = 0;
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

    // Flaw 8, 9, 10 Fix: Remove the flawed post-generation loop and add a correct final display loop.
    // This loop will keep the final generated maze on screen until the window is closed.
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

        // Clear and redraw the final maze state
        window.clear(sf::Color(30, 30, 30));
        // Pass -1,-1 for currentX,currentY so no cell is highlighted as 'current'
        drawMazeGeneration(window, grid, -1, -1, n);
        window.display();
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
