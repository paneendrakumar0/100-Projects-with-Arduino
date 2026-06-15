'use client';

import { motion } from 'framer-motion';

export default function ArduinoSimulator({ activePins = [] }) {
  // A simplified SVG representation of an Arduino Uno
  // Highlight active pins if they are passed in the prop
  
  const isPinActive = (pin) => activePins.includes(pin.toString());

  return (
    <div className="simulator-container">
      <div className="simulator-header">
        <span className="badge">Interactive Hardware</span>
        <h4>Arduino Uno R3</h4>
      </div>
      
      <div className="board">
        <svg viewBox="0 0 400 300" className="arduino-svg" xmlns="http://www.w3.org/2000/svg">
          {/* Base Board */}
          <rect x="10" y="10" width="380" height="280" rx="10" fill="#0c4a6e" stroke="#0ea5e9" strokeWidth="2" />
          
          {/* USB Port */}
          <rect x="5" y="30" width="30" height="40" rx="2" fill="#94a3b8" />
          
          {/* Power Jack */}
          <rect x="5" y="220" width="30" height="30" rx="2" fill="#1e293b" />
          
          {/* Microcontroller */}
          <rect x="150" y="100" width="100" height="40" rx="2" fill="#0f172a" />
          <text x="200" y="125" fill="#475569" fontSize="12" textAnchor="middle" fontFamily="monospace">ATmega328P</text>
          
          {/* Digital Pins Top */}
          <g transform="translate(100, 20)">
            <rect width="250" height="15" fill="#1e293b" />
            {[...Array(14)].map((_, i) => (
              <g key={`d-${13-i}`} transform={`translate(${10 + i * 16}, 0)`}>
                <motion.circle 
                  cx="5" cy="7.5" r="4" 
                  fill={isPinActive(13-i) ? "#39ff14" : "#334155"} 
                  animate={{
                    boxShadow: isPinActive(13-i) ? "0 0 10px #39ff14" : "none",
                  }}
                />
                <text x="5" y="28" fill="#94a3b8" fontSize="8" textAnchor="middle">{13-i}</text>
              </g>
            ))}
          </g>
          
          {/* Analog Pins Bottom */}
          <g transform="translate(200, 265)">
            <rect width="120" height="15" fill="#1e293b" />
            {[...Array(6)].map((_, i) => (
              <g key={`a-${i}`} transform={`translate(${10 + i * 18}, 0)`}>
                <motion.circle 
                  cx="5" cy="7.5" r="4" 
                  fill={isPinActive(`A${i}`) ? "#ffb800" : "#334155"} 
                />
                <text x="5" y="-10" fill="#94a3b8" fontSize="8" textAnchor="middle">A{i}</text>
              </g>
            ))}
          </g>

          {/* Power Pins Bottom */}
          <g transform="translate(80, 265)">
            <rect width="100" height="15" fill="#1e293b" />
            <text x="10" y="-10" fill="#94a3b8" fontSize="8">3.3V</text>
            <text x="35" y="-10" fill="#94a3b8" fontSize="8">5V</text>
            <text x="60" y="-10" fill="#94a3b8" fontSize="8">GND</text>
            <text x="85" y="-10" fill="#94a3b8" fontSize="8">GND</text>
            <motion.circle cx="15" cy="7.5" r="4" fill={isPinActive('3.3V') ? "#ef4444" : "#334155"} />
            <motion.circle cx="40" cy="7.5" r="4" fill={isPinActive('5V') ? "#ef4444" : "#334155"} />
            <motion.circle cx="65" cy="7.5" r="4" fill={isPinActive('GND') ? "#00f0ff" : "#334155"} />
            <motion.circle cx="90" cy="7.5" r="4" fill={isPinActive('GND') ? "#00f0ff" : "#334155"} />
          </g>
        </svg>
      </div>

      <style jsx>{`
        .simulator-container {
          background: rgba(10, 13, 20, 0.8);
          border: 1px solid rgba(255, 255, 255, 0.1);
          border-radius: 12px;
          padding: 1.5rem;
          margin: 2rem 0;
          box-shadow: 0 20px 40px -10px rgba(0,0,0,0.5);
        }
        .simulator-header {
          display: flex;
          align-items: center;
          gap: 1rem;
          margin-bottom: 1.5rem;
        }
        .badge {
          background: rgba(0, 240, 255, 0.1);
          color: #00f0ff;
          padding: 0.2rem 0.6rem;
          border-radius: 999px;
          font-size: 0.75rem;
          font-weight: 700;
          border: 1px solid rgba(0, 240, 255, 0.2);
        }
        h4 {
          margin: 0;
          font-family: 'Outfit', sans-serif;
          color: #fff;
        }
        .board {
          display: flex;
          justify-content: center;
          padding: 1rem;
          background: rgba(0,0,0,0.3);
          border-radius: 8px;
          box-shadow: inset 0 0 20px rgba(0,0,0,0.5);
        }
        .arduino-svg {
          width: 100%;
          max-width: 500px;
          height: auto;
        }
      `}</style>
    </div>
  );
}
