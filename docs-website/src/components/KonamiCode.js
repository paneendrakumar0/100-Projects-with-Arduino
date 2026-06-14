'use client';

import { useEffect, useState } from 'react';

const KONAMI_CODE = [
  'ArrowUp',
  'ArrowUp',
  'ArrowDown',
  'ArrowDown',
  'ArrowLeft',
  'ArrowRight',
  'ArrowLeft',
  'ArrowRight',
  'b',
  'a'
];

export default function KonamiCode() {
  const [inputSequence, setInputSequence] = useState([]);

  const triggerEasterEgg = () => {
    // Add the easter egg class to the body
    document.body.classList.add('konami-activated');
    
    // Play a retro sound if possible, or just log
    console.log('🎮 KONAMI CODE ACTIVATED 🎮');
    
    // Remove after 5 seconds
    setTimeout(() => {
      document.body.classList.remove('konami-activated');
    }, 5000);
  };

  useEffect(() => {
    const handleKeyDown = (e) => {
      const key = e.key;
      
      setInputSequence((prev) => {
        const nextSequence = [...prev, key];
        
        // Keep only the last N keystrokes
        if (nextSequence.length > KONAMI_CODE.length) {
          nextSequence.shift();
        }

        // Check if the sequence matches
        if (nextSequence.join(',') === KONAMI_CODE.join(',')) {
          triggerEasterEgg();
          return []; // Reset after triggering
        }

        return nextSequence;
      });
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, []);

  return (
    <style jsx global>{`
      @keyframes rgb-spin {
        0% { filter: hue-rotate(0deg) contrast(1.5) saturate(2); transform: perspective(1000px) rotateX(0deg) rotateY(0deg) scale(1); }
        50% { transform: perspective(1000px) rotateX(10deg) rotateY(180deg) scale(0.9); }
        100% { filter: hue-rotate(360deg) contrast(1.5) saturate(2); transform: perspective(1000px) rotateX(0deg) rotateY(360deg) scale(1); }
      }
      
      .konami-activated .app-container {
        animation: rgb-spin 5s cubic-bezier(0.25, 0.46, 0.45, 0.94) forwards;
        box-shadow: 0 0 100px rgba(0, 255, 255, 0.5), 0 0 200px rgba(255, 0, 255, 0.5);
        border-radius: 20px;
        overflow: hidden;
      }
      
      .konami-activated {
        background: #000;
        overflow: hidden;
      }
    `}</style>
  );
}
