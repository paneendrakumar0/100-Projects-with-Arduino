'use client';

import { motion } from 'framer-motion';
import { Award, Zap, Cpu, Compass, Radio } from 'lucide-react';
import { useEffect, useState } from 'react';

const BADGES = [
  { id: 'starter', name: 'Circuit Starter', req: 5, icon: Zap, color: '#ffb800' },
  { id: 'intermediate', name: 'Logic Builder', req: 25, icon: Cpu, color: '#00f0ff' },
  { id: 'advanced', name: 'System Integrator', req: 50, icon: Radio, color: '#39ff14' },
  { id: 'master', name: 'Autonomy Master', req: 100, icon: Compass, color: '#ff00ff' },
];

export default function ProgressDashboard() {
  const [completed, setCompleted] = useState(0);

  useEffect(() => {
    const update = () => {
      const days = JSON.parse(localStorage.getItem('completedDays') || '[]');
      setCompleted(days.length);
    };
    update();
    window.addEventListener('progressUpdated', update);
    return () => window.removeEventListener('progressUpdated', update);
  }, []);

  const progressPercentage = (completed / 100) * 100;
  const strokeDashoffset = 283 - (283 * progressPercentage) / 100;

  return (
    <div className="dashboard">
      <div className="dashboard-header">
        <h2>Your Journey</h2>
        <p>Track your progress through the 100 Days of Arduino masterclass.</p>
      </div>

      <div className="stats-grid">
        <div className="stat-card radial-card">
          <div className="radial-progress">
            <svg viewBox="0 0 100 100">
              <circle cx="50" cy="50" r="45" className="bg-circle" />
              <motion.circle 
                cx="50" cy="50" r="45" 
                className="progress-circle" 
                initial={{ strokeDashoffset: 283 }}
                animate={{ strokeDashoffset }}
                transition={{ duration: 1.5, ease: "easeOut" }}
              />
            </svg>
            <div className="radial-text">
              <span className="value">{completed}</span>
              <span className="label">/ 100</span>
            </div>
          </div>
          <h3>Days Completed</h3>
        </div>

        <div className="badges-container">
          <h3>Achievements</h3>
          <div className="badges-grid">
            {BADGES.map((badge, idx) => {
              const isUnlocked = completed >= badge.req;
              const Icon = badge.icon;
              return (
                <motion.div 
                  key={badge.id}
                  className={`badge-item ${isUnlocked ? 'unlocked' : 'locked'}`}
                  initial={{ opacity: 0, y: 20 }}
                  animate={{ opacity: 1, y: 0 }}
                  transition={{ delay: idx * 0.1 }}
                >
                  <div className="badge-icon" style={{ borderColor: isUnlocked ? badge.color : 'rgba(255,255,255,0.1)' }}>
                    <Icon size={24} color={isUnlocked ? badge.color : '#64748b'} />
                  </div>
                  <div className="badge-info">
                    <h4>{badge.name}</h4>
                    <span>{badge.req} Days Required</span>
                  </div>
                </motion.div>
              );
            })}
          </div>
        </div>
      </div>

      <style jsx>{`
        .dashboard {
          background: var(--glass-bg);
          border: 1px solid var(--glass-border);
          border-radius: 16px;
          padding: 2rem;
          margin: 3rem 0;
          box-shadow: 0 20px 40px -10px rgba(0,0,0,0.5);
        }
        .dashboard-header {
          margin-bottom: 2rem;
        }
        .dashboard-header h2 {
          margin-top: 0;
          margin-bottom: 0.5rem;
          border: none;
        }
        .stats-grid {
          display: grid;
          grid-template-columns: 1fr 2fr;
          gap: 2rem;
        }
        .stat-card {
          background: rgba(0,0,0,0.2);
          border-radius: 12px;
          padding: 2rem;
          display: flex;
          flex-direction: column;
          align-items: center;
          justify-content: center;
          border: 1px solid rgba(255,255,255,0.05);
        }
        .radial-progress {
          position: relative;
          width: 150px;
          height: 150px;
          margin-bottom: 1.5rem;
        }
        .radial-progress svg {
          transform: rotate(-90deg);
          width: 100%;
          height: 100%;
        }
        .bg-circle {
          fill: none;
          stroke: rgba(255,255,255,0.05);
          stroke-width: 8;
        }
        .progress-circle {
          fill: none;
          stroke: var(--accent-primary);
          stroke-width: 8;
          stroke-dasharray: 283;
          stroke-linecap: round;
          filter: drop-shadow(0 0 8px rgba(0,240,255,0.5));
        }
        .radial-text {
          position: absolute;
          inset: 0;
          display: flex;
          flex-direction: column;
          align-items: center;
          justify-content: center;
        }
        .radial-text .value {
          font-family: 'Outfit', sans-serif;
          font-size: 2.5rem;
          font-weight: 900;
          line-height: 1;
          color: #fff;
        }
        .radial-text .label {
          font-size: 0.8rem;
          color: var(--text-muted);
        }
        .badges-container {
          background: rgba(0,0,0,0.2);
          border-radius: 12px;
          padding: 2rem;
          border: 1px solid rgba(255,255,255,0.05);
        }
        .badges-container h3 {
          margin-top: 0;
          font-size: 1.25rem;
          margin-bottom: 1.5rem;
        }
        .badges-grid {
          display: grid;
          grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
          gap: 1rem;
        }
        .badge-item {
          display: flex;
          align-items: center;
          gap: 1rem;
          padding: 1rem;
          background: rgba(255,255,255,0.03);
          border-radius: 8px;
          border: 1px solid rgba(255,255,255,0.05);
          transition: all 0.3s;
        }
        .badge-item.unlocked {
          background: linear-gradient(135deg, rgba(255,255,255,0.05), rgba(255,255,255,0.01));
          border-color: rgba(255,255,255,0.1);
        }
        .badge-icon {
          width: 48px;
          height: 48px;
          border-radius: 50%;
          border: 2px solid transparent;
          display: flex;
          align-items: center;
          justify-content: center;
          background: rgba(0,0,0,0.3);
        }
        .badge-info h4 {
          margin: 0 0 0.25rem 0;
          font-size: 0.95rem;
          color: #fff;
        }
        .badge-item.locked .badge-info h4 {
          color: var(--text-muted);
        }
        .badge-info span {
          font-size: 0.75rem;
          color: var(--text-muted);
        }
        @media (max-width: 768px) {
          .stats-grid {
            grid-template-columns: 1fr;
          }
        }
      `}</style>
    </div>
  );
}
