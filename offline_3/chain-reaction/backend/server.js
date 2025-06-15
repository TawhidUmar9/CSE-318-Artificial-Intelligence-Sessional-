const express = require('express');
const cors = require('cors'); // Enables Cross-Origin Resource Sharing
const bodyParser = require('body-parser'); // Parses JSON request bodies
const fs = require('fs').promises; // For asynchronous file operations
const { execFile } = require('child_process'); // To execute the C++ agent
const path = require('path'); // For resolving file paths

const app = express();
const PORT = 3001; // Node.js server will run on this port. React connects here.

// --- File Paths ---
// __dirname is the directory where server.js is located (e.g., chain-reaction-game/backend)
// C_PLUS_PLUS_AGENT_PATH: Points to your compiled C++ executable, 'game_engine_ai', which is in the same 'backend' folder
const C_PLUS_PLUS_AGENT_PATH = path.join(__dirname, 'game_engine_ai');
// GAME_STATE_FILE: Points to the shared communication file 'input.txt', also in the 'backend' folder
const GAME_STATE_FILE = path.join(__dirname, 'input.txt');

// --- Middleware ---
app.use(cors()); // Enable CORS for requests from your React frontend (usually on localhost:3000)
app.use(bodyParser.json()); // Parse incoming JSON request bodies

// --- Helper Functions for Board Data Conversion ---
// These mimic your C++ agent's file format (e.g., "1R", "0")

// Parses a single cell string (e.g., "1R", "0") from the file content into a JS object
function parseCellString(cellStr) {
    if (cellStr === '0') {
        return { count: 0, color: 'E' }; // 'E' for Empty
    }
    const count = parseInt(cellStr.slice(0, -1), 10);
    const color = cellStr.slice(-1); // Last character is the color
    return { count, color };
}

// Converts a JavaScript grid (array of cell objects) into the string format for the file
function boardGridToString(gridData) {
    let result = '';
    for (let r = 0; r < gridData.length; r++) {
        for (let c = 0; c < gridData[r].length; c++) {
            const cell = gridData[r][c];
            if (cell.count === 0) {
                result += '0';
            } else {
                result += `${cell.count}${cell.color}`;
            }
            if (c < gridData[r].length - 1) { // Add space unless it's the last cell in the row
                result += ' ';
            }
        }
        result += '\n'; // Newline after each row
    }
    return result;
}

// Reads the GAME_STATE_FILE and parses its content into a structured game state object
async function readGameStateFromFile() {
    try {
        const fileContent = await fs.readFile(GAME_STATE_FILE, 'utf8');
        const lines = fileContent.trim().split('\n');

        if (lines.length < 1) {
            throw new Error('Game state file is empty or malformed.');
        }

        const header = lines[0];
        let currentPlayer = 'N'; // 'N' for Not determined, will be 'R' or 'B'
        let winner = 'N'; // 'N' if game is ongoing, 'R' or 'B' if game over

        // Determine current player and check for winner based on file header
        if (header.startsWith('Human Move:')) {
            currentPlayer = 'B'; // If Human just moved, it's AI's turn (Blue)
        } else if (header.startsWith('AI Move:')) {
            currentPlayer = 'R'; // If AI just moved, it's Human's turn (Red)
        } else if (header.startsWith('Game Over! Winner:')) {
            winner = header.split(': ')[1].trim(); // Extract winner from header
            currentPlayer = 'N'; // Game is over, no active player
        }

        // Parse the grid content (assuming a 9x6 board)
        const gridLines = lines.slice(1);
        const grid = gridLines.map(line => line.split(' ').map(parseCellString));

        // Basic winner check based on orb counts (as a fallback/consistency check)
        // Your C++ engine's is_game_over() and get_winner() are the primary source of truth.
        let redOrbCount = 0;
        let blueOrbCount = 0;
        grid.forEach(row => row.forEach(cell => {
            if (cell.color === 'R') redOrbCount += cell.count;
            if (cell.color === 'B') blueOrbCount += cell.count;
        }));

        if (redOrbCount === 0 && blueOrbCount > 0 && winner === 'N') winner = 'B';
        else if (blueOrbCount === 0 && redOrbCount > 0 && winner === 'N') winner = 'R';

        return { header, grid, currentPlayer, winner };

    } catch (error) {
        console.error(`Error reading ${GAME_STATE_FILE}:`, error.message);
        // Return a default empty board state if file read fails, to keep frontend functional
        return {
            header: 'Initializing:',
            grid: Array(9).fill(0).map(() => Array(6).fill({ count: 0, color: 'E' })), // Default 9x6 empty grid
            currentPlayer: 'R', // Assume Red player starts on a fresh/error board
            winner: 'N'
        };
    }
}

// --- API Endpoints ---

// GET /api/state: Frontend calls this to get the latest game state
app.get('/api/state', async (req, res) => {
    const gameState = await readGameStateFromFile();
    res.json(gameState);
});

// POST /api/move: Frontend calls this when a human player makes a move
app.post('/api/move', async (req, res) => {
    const { row, col, player } = req.body; // Expects { row: int, col: int, player: 'R' }

    // Human player is 'R' by convention in this setup
    const HUMAN_PLAYER_CHAR = 'R'; 

    // 1. Basic input validation for coordinates
    if (!Number.isInteger(row) || !Number.isInteger(col) || row < 0 || row >= 9 || col < 0 || col >= 6) {
        return res.status(400).json({ error: 'Invalid move coordinates.' });
    }

    try {
        let gameState = await readGameStateFromFile();

        // 2. Validate move context (turn, game over, cell validity)
        if (gameState.currentPlayer !== HUMAN_PLAYER_CHAR || gameState.winner !== 'N') {
            return res.status(400).send('Not your turn or game is already over.');
        }
        const targetCell = gameState.grid[row][col];
        if (targetCell.color !== 'E' && targetCell.color !== HUMAN_PLAYER_CHAR) {
            return res.status(400).send('Invalid move: Cell is occupied by opponent.');
        }

        // 3. Apply human move in Node.js memory (Simplified: just orb placement)
        // The C++ agent will handle the full explosion logic when it reads this file.
        let updatedGrid = JSON.parse(JSON.stringify(gameState.grid)); // Create a deep copy of the grid
        if (updatedGrid[row][col].color === 'E') {
            updatedGrid[row][col].color = HUMAN_PLAYER_CHAR;
            updatedGrid[row][col].count = 1;
        } else {
            updatedGrid[row][col].count++; // Increment count if human's own orb
        }

        // 4. Write human move to input.txt for C++ agent to read
        const humanMoveContent = `Human Move:\n${boardGridToString(updatedGrid)}`;
        await fs.writeFile(GAME_STATE_FILE, humanMoveContent, 'utf8');

        // 5. Execute C++ Agent and wait for it to complete its turn
        console.log(`Executing C++ agent: ${C_PLUS_PLUS_AGENT_PATH}`);
        const { stdout, stderr } = await new Promise((resolve, reject) => {
            // Set 'cwd' (current working directory) for the agent process
            // This ensures the C++ agent finds 'input.txt' correctly, as 'input.txt' is in the same folder as server.js.
            execFile(C_PLUS_PLUS_AGENT_PATH, { cwd: __dirname }, (error, stdout, stderr) => {
                if (error) {
                    console.error('C++ Agent execution error:', error);
                    console.error('Agent stderr:', stderr);
                    reject(new Error(`C++ Agent failed: ${error.message} \nStderr: ${stderr}`));
                } else {
                    console.log('C++ Agent stdout:', stdout);
                    resolve({ stdout, stderr });
                }
            });
        });

        // 6. Read the updated state from input.txt after AI's move
        const newGameState = await readGameStateFromFile();
        res.json(newGameState); // Send the AI's updated board state back to the frontend

    } catch (err) {
        console.error('Failed to process move:', err);
        res.status(500).json({ error: 'Failed to process move', details: err.message });
    }
});

// IMPORTANT: This serves your built React frontend.
// During development (npm start), React serves itself on port 3000.
// This block is primarily for when you deploy a 'built' React app (npm run build)
// and want Node.js to serve all static assets.
// For local development, you generally keep this commented out or avoid it,
// as React's dev server handles asset serving.
// app.use(express.static(path.join(__dirname, '../frontend/dist')));
// app.get('*', (req, res) => {
//   res.sendFile(path.join(__dirname, '../frontend/dist/index.html'));
// });


// --- Start Server ---
app.listen(PORT, () => {
    console.log(`Node.js server listening on http://localhost:${PORT}`);
    console.log(`Ensure your C++ executable is compiled and located at: ${C_PLUS_PLUS_AGENT_PATH}`);
    console.log(`Shared game state file is at: ${GAME_STATE_FILE}`);

    // Initialize/Reset input.txt on server start to ensure a clean game
    const initialBoardGrid = Array(9).fill(0).map(() => Array(6).fill({ count: 0, color: 'E' })); // Create an empty 9x6 board
    // *** MODIFIED LINE HERE: Make AI implicitly move first ***
    const initialFileContent = `AI Move:\n${boardGridToString(initialBoardGrid)}`;
    // ******************************************************
    fs.writeFile(GAME_STATE_FILE, initialFileContent, 'utf8')
        .then(() => console.log(`'${GAME_STATE_FILE}' initialized for AI to make the first move.`))
        .catch(err => console.error(`Failed to initialize '${GAME_STATE_FILE}':`, err));
});