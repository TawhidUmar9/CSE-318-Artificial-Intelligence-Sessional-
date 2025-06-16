import React, { useState, useEffect, useCallback } from 'react';
import './App.css';

const API_BASE_URL = 'http://localhost:3001/api';
const POLLING_INTERVAL_MS = 500;

function App() {
  const [boardData, setBoardData] = useState([]);
  const [gameStatus, setGameStatus] = useState('Waiting for AI to start...');
  const [winner, setWinner] = useState('N');
  const [isProcessing, setIsProcessing] = useState(false);
  const [currentPlayer, setCurrentPlayer] = useState('N');

  const fetchGameState = useCallback(async () => {
    if (isProcessing) return;

    try {
      const response = await fetch(`${API_BASE_URL}/state`);
      if (!response.ok) {
        throw new Error(`HTTP error! Status: ${response.status}`);
      }
      const data = await response.json();

      setBoardData(data.grid);
      setWinner(data.winner);
      setCurrentPlayer(data.currentPlayer);

      if (data.winner !== 'N') {
        setGameStatus(`Game Over! Winner: ${data.winner === 'R' ? 'Red' : 'Blue'}!`);
      } else if (data.currentPlayer === 'R') {
        setGameStatus("Your Turn (Red)");
      } else if (data.currentPlayer === 'B') {
        setGameStatus("AI's Turn (Blue)");
      } else {
        setGameStatus("Game Starting...");
      }
    } catch (error) {
      console.error("Failed to fetch game state:", error);
      setGameStatus("Error connecting to server. Please ensure server is running.");
    }
  }, [isProcessing]);

  useEffect(() => {
    fetchGameState();
    const intervalId = setInterval(fetchGameState, POLLING_INTERVAL_MS);
    return () => clearInterval(intervalId);
  }, [fetchGameState]);

  const handleCellClick = async (row, col) => {
    if (isProcessing || currentPlayer !== 'R' || winner !== 'N') {
      return;
    }

    // Optional chaining added for safety if boardData/cell is null/undefined during a very fast update
    const clickedCell = boardData?.[row]?.[col];
    if (clickedCell?.color !== 'E' && clickedCell?.color !== 'R') {
      setGameStatus("Invalid move: Cell occupied by opponent or not empty. Try again.");
      return;
    }

    setIsProcessing(true);
    setGameStatus('Sending your move...');

    try {
      const response = await fetch(`${API_BASE_URL}/move`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ row, col, player: 'R' }), // 'R' for human player
      });

      if (!response.ok) {
        const errorData = await response.json();
        throw new Error(`Move failed: ${response.status} - ${errorData.error || 'Unknown error'}`);
      }
      // No need to set boardData/winner here, polling will update it after C++ agent runs
      // console.log('Move sent, waiting for server update...');

    } catch (error) {
      console.error("Error sending move:", error);
      setGameStatus(`Error: ${error.message}`);
    } finally {
      setIsProcessing(false); // Re-enable processing once the API call is done, fetchGameState will catch up
    }
  };

  const handleResetGame = async () => {
    setIsProcessing(true); // Disable interactions
    setGameStatus('Resetting game...');

    try {
      const response = await fetch(`${API_BASE_URL}/reset`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
      });

      if (!response.ok) {
        const errorData = await response.json();
        throw new Error(`Reset failed: ${response.status} - ${errorData.error || 'Unknown error'}`);
      }
      // Polling will handle fetching the new state after reset
      // console.log('Reset request sent, waiting for server update...');

    } catch (error) {
      console.error("Error resetting game:", error);
      setGameStatus(`Error: ${error.message}`);
    } finally {
      setIsProcessing(false); // Re-enable interactions
    }
  };

  // Helper to get dynamic CSS classes for cells
  const getCellClasses = (cell) => {
    let classes = 'cell';
    // Optional chaining added for safety
    if (cell?.color === 'R') classes += ' cell-red';
    else if (cell?.color === 'B') classes += ' cell-blue';
    else classes += ' cell-empty';

    // Determine if the cell is clickable based on game state and current player
    const canClick = (currentPlayer === 'R' && winner === 'N' && !isProcessing && (cell?.color === 'E' || cell?.color === 'R'));
    if (canClick) {
      classes += ' clickable';
    } else {
      classes += ' not-clickable'; // Visually distinguish non-clickable cells
    }
    return classes;
  };

  return (
    <div className={`App ${isProcessing ? 'processing' : ''}`}>
      <div className="anim-gradient-bg"></div> {/* Animated background div (first child) */}
      <h1>Chain Reaction</h1>
      <p className="game-status">{gameStatus}</p>
      {boardData.length > 0 ? (
        <div className="board-grid">
          {boardData.map((row, rIdx) => (
            <div key={`row-${rIdx}`} className="row">
              {row.map((cell, cIdx) => (
                <div
                  key={`${rIdx}-${cIdx}`}
                  className={getCellClasses(cell)}
                  onClick={() => handleCellClick(rIdx, cIdx)}
                >
                  {/* Render count only if greater than 0, wrapped in span for animation */}
                  {cell?.count > 0 ? <span>{cell.count}</span> : ''}
                </div>
              ))}
            </div>
          ))}
        </div>
      ) : (
        <p>Loading game board...</p>
      )}
      <button onClick={handleResetGame} className="reset-button" disabled={isProcessing}>
        {isProcessing ? 'Processing...' : 'Reset Game'}
      </button>
    </div>
  );
}

export default App;