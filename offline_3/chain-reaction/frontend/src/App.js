import React, { useState, useEffect, useCallback } from 'react';
import './App.css';

const API_BASE_URL = 'http://localhost:3001/api';
const POLLING_INTERVAL_MS = 500;

function App() {
  const [boardData, setBoardData] = useState([]);
  const [gameStatus, setGameStatus] = useState('Waiting for AI to start...');
  const [winner, setWinner] = useState('N');
  const [isProcessing, setIsProcessing] = useState(false);

  const fetchGameState = useCallback(async () => {
    if (isProcessing) return;
    try {
      const response = await fetch(`${API_BASE_URL}/state`);
      if (!response.ok) {
        throw new Error(`HTTP error! Status: ${response.status}`);
      }
      const data = await response.json();
      console.log('Fetched state:', data);
      setBoardData(data.grid);
      setWinner(data.winner);
      if (data.winner !== 'N') {
        setGameStatus(`Game Over! Winner: ${data.winner === 'R' ? 'Red' : 'Blue'}!`);
      } else if (data.currentPlayer === 'R') {
        setGameStatus("Your Turn (Red)");
      } else {
        setGameStatus("AI's Turn (Blue)");
      }
    } catch (error) {
      console.error("Failed to fetch game state:", error);
      setGameStatus("Error connecting to server.");
    }
  }, [isProcessing]);

  useEffect(() => {
    fetchGameState();
    const intervalId = setInterval(fetchGameState, POLLING_INTERVAL_MS);
    return () => clearInterval(intervalId);
  }, [fetchGameState]);

  const handleCellClick = async (row, col) => {
    console.log(`Clicked cell (${row}, ${col})`);
    if (gameStatus !== "Your Turn (Red)" || winner !== 'N') {
      console.log('Cannot click: Not your turn or game is over.', { gameStatus, winner });
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
        body: JSON.stringify({ row, col, player: 'R' }),
      });
      if (!response.ok) {
        const errorData = await response.json();
        console.error('Move error:', errorData);
        throw new Error(`Move failed: ${response.status} - ${errorData.error || 'Unknown error'}`);
      }
      const data = await response.json();
      console.log('Move response:', data);
      setBoardData(data.grid);
      setWinner(data.winner);
      setGameStatus(data.winner !== 'N' ? `Game Over! Winner: ${data.winner === 'R' ? 'Red' : 'Blue'}!` : "Your Turn (Red)");
    } catch (error) {
      console.error("Error sending move:", error);
      setGameStatus(`Error: ${error.message}`);
    } finally {
      setIsProcessing(false);
    }
  };

  const handleResetGame = async () => {
    console.log('Resetting game');
    setIsProcessing(true);
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
        console.error('Reset error:', errorData);
        throw new Error(`Reset failed: ${response.status} - ${errorData.error || 'Unknown error'}`);
      }
      const data = await response.json();
      console.log('Reset response:', data);
      setBoardData(data.grid);
      setWinner(data.winner);
      setGameStatus(data.currentPlayer === 'R' ? "Your Turn (Red)" : "AI's Turn (Blue)");
    } catch (error) {
      console.error("Error resetting game:", error);
      setGameStatus(`Error: ${error.message}`);
    } finally {
      setIsProcessing(false);
    }
  };

  const getCellClasses = (cell) => {
    let classes = 'cell';
    if (cell.color === 'R') classes += ' cell-red';
    else if (cell.color === 'B') classes += ' cell-blue';
    else classes += ' cell-empty';
    return classes;
  };

  return (
    <div className="App">
      <h1>Chain Reaction Game</h1>
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
                  {cell.count > 0 ? cell.count : ''}
                </div>
              ))}
            </div>
          ))}
        </div>
      ) : (
        <p>Board is loading or empty.</p>
      )}
      <button onClick={handleResetGame} className="reset-button">
        Reset Game
      </button>
    </div>
  );
}

export default App;