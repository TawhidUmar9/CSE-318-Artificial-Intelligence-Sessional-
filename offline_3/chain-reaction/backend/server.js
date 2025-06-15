const express = require('express');
const cors = require('cors');
const bodyParser = require('body-parser');
const fs = require('fs').promises;
const { execFile } = require('child_process');
const path = require('path');

const app = express();
const PORT = 3001;
const EXEC_TIMEOUT_MS = 10000;

const C_PLUS_PLUS_AGENT_PATH = path.join(__dirname, 'game_engine_ai'); 
const GAME_STATE_FILE = path.join(__dirname, 'input.txt');

app.use(cors());
app.use(bodyParser.json());

function parseCellString(cellStr) {
    if (cellStr === '0') {
        return { count: 0, color: 'E' };
    }
    const count = parseInt(cellStr.slice(0, -1), 10);
    const color = cellStr.slice(-1);
    return { count, color };
}

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
            if (c < gridData[r].length - 1) {
                result += ' ';
            }
        }
        result += '\n';
    }
    return result;
}

async function readGameStateFromFile() {
    try {
        const fileContent = await fs.readFile(GAME_STATE_FILE, 'utf8');
        const lines = fileContent.trim().split('\n');
        if (lines.length < 1) {
            console.error('Game state file is empty or malformed.');
            return await resetGameState(false); 
        }

        const header = lines[0];
        let currentPlayer = 'N';
        let winner = 'N';

        if (header.startsWith('Human Move:')) {
            currentPlayer = 'B';
        } else if (header.startsWith('AI Move:')) {
            currentPlayer = 'R';
        } else if (header.startsWith('Game Over! Winner:')) {
            winner = header.split(': ')[1].trim();
            currentPlayer = 'N';
        } else if (header.startsWith('Initializing:')) {
            currentPlayer = 'B';
            console.error(`Unknown header: "${header}", resetting game state.`);
            return await resetGameState(false);
        }

        const gridLines = lines.slice(1);
        if (gridLines.length !== 9) {
            console.error(`Invalid grid: Expected 9 rows, got ${gridLines.length}. Resetting.`);
            return await resetGameState(false);
        }

        const grid = gridLines.map(line => {
            const cells = line.split(' ');
            if (cells.length !== 6) {
                console.error(`Invalid grid row: Expected 6 columns, got ${cells.length}. Resetting.`);
                throw new Error('Invalid grid: Expected 6 columns per row'); 
            }
            return cells.map(parseCellString);
        });

        return { header, grid, currentPlayer, winner };

    } catch (error) {
        console.error(`Error reading ${GAME_STATE_FILE}:`, error.message);
        return await resetGameState(true);
    }
}

async function resetGameState(runAIFirstMove = true) {
    try {
        const initialBoardGrid = Array(9).fill(0).map(() => Array(6).fill({ count: 0, color: 'E' }));
        const initialFileContent = `Initializing:\n${boardGridToString(initialBoardGrid)}`;
        await fs.writeFile(GAME_STATE_FILE, initialFileContent, 'utf8');

        if (runAIFirstMove) {
            const { stdout, stderr } = await new Promise((resolve, reject) => {
                const child = execFile(C_PLUS_PLUS_AGENT_PATH, { cwd: __dirname }, (error, stdout, stderr) => {
                    if (error) {
                        console.error('C++ Agent first move error:', error);
                        console.error('Agent stderr:', stderr);
                        reject(new Error(`C++ Agent failed: ${error.message} \nStderr: ${stderr}`));
                    } else {
                        resolve({ stdout, stderr });
                    }
                });
                setTimeout(() => {
                    child.kill();
                    reject(new Error('C++ Agent timed out during first move'));
                }, EXEC_TIMEOUT_MS);
            });
            console.log('AI first move completed.');
        } else {
            console.log('Skipping AI first move execution during reset.');
        }

        const newGameState = await readGameStateFromFile();
        return newGameState;

    } catch (err) {
        console.error('Failed to reset game state:', err);
        return {
            header: 'Error during reset',
            grid: Array(9).fill(0).map(() => Array(6).fill({ count: 0, color: 'E' })),
            currentPlayer: 'N',
            winner: 'N'
        };
    }
}

app.get('/api/state', async (req, res) => {
    const gameState = await readGameStateFromFile();
    res.json(gameState);
});

app.post('/api/move', async (req, res) => {
    const { row, col, player } = req.body;
    const HUMAN_PLAYER_CHAR = 'R';
    if (!Number.isInteger(row) || !Number.isInteger(col) || row < 0 || row >= 9 || col < 0 || col >= 6) {
        return res.status(400).json({ error: 'Invalid move coordinates.' });
    }

    try {
        let gameState = await readGameStateFromFile();
        if (gameState.currentPlayer !== HUMAN_PLAYER_CHAR || gameState.winner !== 'N') {
            return res.status(400).json({ error: 'Not your turn or game is already over.' });
        }

        const targetCell = gameState.grid[row][col];
        if (targetCell.color !== 'E' && targetCell.color !== HUMAN_PLAYER_CHAR) {
            return res.status(400).json({ error: 'Invalid move: Cell is occupied by opponent.' });
        }

        const humanMoveContent = `Human Move: ${row} ${col}\n${boardGridToString(gameState.grid)}`;
        await fs.writeFile(GAME_STATE_FILE, humanMoveContent, 'utf8');

        const { stdout, stderr } = await new Promise((resolve, reject) => {
            const child = execFile(C_PLUS_PLUS_AGENT_PATH, { cwd: __dirname }, (error, stdout, stderr) => {
                if (error) {
                    console.error('C++ Agent execution error:', error);
                    console.error('Agent stderr:', stderr);
                    reject(new Error(`C++ Agent failed: ${error.message} \nStderr: ${stderr}`));
                } else {
                    console.log('C++ Agent stdout:', stdout);
                    resolve({ stdout, stderr });
                }
            });
            setTimeout(() => {
                child.kill();
                reject(new Error('C++ Agent timed out during move'));
            }, EXEC_TIMEOUT_MS);
        });

        const newGameState = await readGameStateFromFile();
        res.json(newGameState);

    } catch (err) {
        console.error('Failed to process move:', err);
        res.status(500).json({ error: 'Failed to process move', details: err.message });
    }
});

app.post('/api/reset', async (req, res) => {
    try {
        const newGameState = await resetGameState(true);
        res.json(newGameState);
    } catch (err) {
        console.error('Failed to reset game:', err);
        res.status(500).json({ error: 'Failed to reset game', details: err.message });
    }
});

app.listen(PORT, async () => {
    console.log(`Node.js server listening on http://localhost:${PORT}`);
    console.log(`Ensure C++ executable is at: ${C_PLUS_PLUS_AGENT_PATH}`);
    console.log(`Game state file: ${GAME_STATE_FILE}`);
    // Initial reset when server starts
    try {
        await resetGameState(true);
        console.log('Game initialized with AI\'s first move.');
    } catch (err) {
        console.error(`Failed to initialize game on server start:`, err);
    }
});