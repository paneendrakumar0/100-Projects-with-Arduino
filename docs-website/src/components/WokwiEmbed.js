'use client';

import { motion } from 'framer-motion';

export default function WokwiEmbed({ projectId }) {
  if (!projectId) return null;

  return (
    <motion.div 
      className="wokwi-container"
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.5 }}
    >
      <div className="wokwi-header">
        <span className="badge">Interactive Simulation</span>
        <h4>Test this code in your browser</h4>
      </div>
      
      <div className="wokwi-iframe-wrapper">
        <iframe 
          src={`https://wokwi.com/projects/${projectId}?embed=1`}
          width="100%" 
          height="500px" 
          style={{ border: 'none', borderRadius: '8px' }}
          title="Wokwi Arduino Simulator"
          allow="autoplay; camera; microphone; midi; vr; xr-spatial-tracking"
        ></iframe>
      </div>

      <style jsx>{`
        .wokwi-container {
          background: rgba(10, 13, 20, 0.8);
          border: 1px solid rgba(255, 255, 255, 0.1);
          border-radius: 12px;
          padding: 1.5rem;
          margin: 2rem 0;
          box-shadow: 0 20px 40px -10px rgba(0,0,0,0.5);
        }
        .wokwi-header {
          display: flex;
          align-items: center;
          gap: 1rem;
          margin-bottom: 1.5rem;
        }
        .badge {
          background: rgba(57, 255, 20, 0.1);
          color: #39ff14;
          padding: 0.2rem 0.6rem;
          border-radius: 999px;
          font-size: 0.75rem;
          font-weight: 700;
          border: 1px solid rgba(57, 255, 20, 0.2);
        }
        h4 {
          margin: 0;
          font-family: 'Outfit', sans-serif;
          color: #fff;
        }
        .wokwi-iframe-wrapper {
          width: 100%;
          border-radius: 8px;
          overflow: hidden;
          background: #1e1e1e;
        }
      `}</style>
    </motion.div>
  );
}
