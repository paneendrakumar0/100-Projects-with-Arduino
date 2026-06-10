'use client';

import Link from 'next/link';
import { motion, useMotionTemplate, useMotionValue } from 'framer-motion';
import { MouseEvent } from 'react';

export default function Home() {
  const mouseX = useMotionValue(0);
  const mouseY = useMotionValue(0);

  function handleMouseMove({ currentTarget, clientX, clientY }) {
    const { left, top } = currentTarget.getBoundingClientRect();
    mouseX.set(clientX - left);
    mouseY.set(clientY - top);
  }

  const container = {
    hidden: { opacity: 0 },
    show: {
      opacity: 1,
      transition: { staggerChildren: 0.15, delayChildren: 0.3 },
    },
  };

  const item = {
    hidden: { opacity: 0, y: 30 },
    show: { opacity: 1, y: 0, transition: { type: 'spring', stiffness: 200, damping: 20 } },
  };

  return (
    <motion.div 
      className="hero"
      variants={container}
      initial="hidden"
      animate="show"
      onMouseMove={handleMouseMove}
    >
      <motion.div
        className="hero-glow-bg"
        style={{
          background: useMotionTemplate`
            radial-gradient(
              600px circle at ${mouseX}px ${mouseY}px,
              rgba(0, 229, 255, 0.1),
              transparent 80%
            )
          `
        }}
      />
      <motion.div variants={item} className="hero-badge">
        <span className="badge-pulse"></span>
        Interactive Documentation
      </motion.div>

      <motion.h1 variants={item}>
        100 Days of <br/>
        <span className="glow-text">Arduino</span>
      </motion.h1>

      <motion.p variants={item} className="hero-subtitle">
        The ultimate zero-to-hero journey for embedded systems. Master microcontrollers,
        sensors, robotics, and advanced C++ programming without relying on delay().
      </motion.p>
      
      <motion.div variants={item} className="hero-buttons">
        <Link href="/learn/Chapter_01_What_Is_Arduino" className="btn btn-primary magnetic-btn">
          Start Beginner Guide
        </Link>
        <Link href="/days/Day_01_Millis_Blink" className="btn btn-secondary magnetic-btn">
          Dive into Day 1
        </Link>
      </motion.div>

      <motion.div variants={container} className="grid">
        <motion.div variants={item} whileHover={{ y: -10, scale: 1.02 }} className="card glass-card">
          <div className="card-icon">🚀</div>
          <h3>Zero to Hero</h3>
          <p>
            Never touched a wire? Start with our 10-chapter prerequisite guide covering
            electronics, coding, and hardware basics.
          </p>
        </motion.div>
        
        <motion.div variants={item} whileHover={{ y: -10, scale: 1.02 }} className="card glass-card">
          <div className="card-icon">🤖</div>
          <h3>Real-world Projects</h3>
          <p>
            Build autonomous robots, smart home devices, telemetry dashboards, and 
            understand advanced concepts like Kinematics and PID.
          </p>
        </motion.div>
        
        <motion.div variants={item} whileHover={{ y: -10, scale: 1.02 }} className="card glass-card">
          <div className="card-icon">⚡</div>
          <h3>Production Ready</h3>
          <p>
            Say goodbye to blocking code. Learn state machines, non-blocking loops,
            hardware interrupts, and FreeRTOS.
          </p>
        </motion.div>
      </motion.div>

      <style jsx>{`
        .hero {
          display: flex;
          flex-direction: column;
          align-items: center;
          text-align: center;
          padding: 6rem 0;
          position: relative;
        }

        .hero-glow-bg {
          position: absolute;
          top: 0;
          left: 0;
          right: 0;
          bottom: 0;
          z-index: -1;
          pointer-events: none;
          opacity: 0;
          transition: opacity 0.5s ease;
        }

        .hero:hover .hero-glow-bg {
          opacity: 1;
        }

        .hero-badge {
          display: inline-flex;
          align-items: center;
          gap: 0.5rem;
          background: rgba(0, 229, 255, 0.1);
          border: 1px solid rgba(0, 229, 255, 0.2);
          padding: 0.5rem 1rem;
          border-radius: 999px;
          font-size: 0.85rem;
          font-weight: 600;
          color: #00e5ff;
          margin-bottom: 2rem;
        }

        .badge-pulse {
          width: 8px;
          height: 8px;
          background: #00e5ff;
          border-radius: 50%;
          box-shadow: 0 0 10px #00e5ff;
          animation: pulse 2s infinite;
        }

        @keyframes pulse {
          0% { box-shadow: 0 0 0 0 rgba(0, 229, 255, 0.7); }
          70% { box-shadow: 0 0 0 10px rgba(0, 229, 255, 0); }
          100% { box-shadow: 0 0 0 0 rgba(0, 229, 255, 0); }
        }

        .hero h1 {
          font-size: 5.5rem;
          margin-bottom: 1.5rem;
          line-height: 1.05;
          margin-top: 0;
          letter-spacing: -0.05em;
        }

        .glow-text {
          background: linear-gradient(to right, #00e5ff, #7000ff);
          -webkit-background-clip: text;
          -webkit-text-fill-color: transparent;
          filter: drop-shadow(0 0 20px rgba(0, 229, 255, 0.4));
        }

        .hero-subtitle {
          font-size: 1.25rem;
          color: var(--text-secondary);
          max-width: 650px;
          margin: 0 auto 3rem auto;
          line-height: 1.8;
        }

        .hero-buttons {
          display: flex;
          gap: 1.5rem;
          justify-content: center;
          margin-bottom: 5rem;
        }

        .btn {
          padding: 1rem 2.5rem;
          border-radius: 12px;
          font-weight: 600;
          font-size: 1.05rem;
          transition: all 0.3s cubic-bezier(0.16, 1, 0.3, 1);
          border: none;
          cursor: pointer;
          text-decoration: none;
          display: inline-flex;
          align-items: center;
          justify-content: center;
        }

        .btn-primary {
          background: #fff;
          color: #000;
          box-shadow: 0 10px 25px -5px rgba(255, 255, 255, 0.3);
        }

        .btn-primary:hover {
          transform: translateY(-4px);
          box-shadow: 0 15px 35px -5px rgba(255, 255, 255, 0.5);
          color: #000;
          background: #f0f0f0;
        }

        .btn-secondary {
          background: rgba(255, 255, 255, 0.05);
          color: white;
          border: 1px solid rgba(255, 255, 255, 0.1);
          backdrop-filter: blur(10px);
        }

        .btn-secondary:hover {
          background: rgba(255, 255, 255, 0.1);
          transform: translateY(-4px);
          color: white;
          border-color: rgba(255, 255, 255, 0.2);
          box-shadow: 0 10px 25px -5px rgba(0, 0, 0, 0.5);
        }

        .grid {
          display: grid;
          grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
          gap: 2rem;
          width: 100%;
        }

        .glass-card {
          background: linear-gradient(145deg, rgba(30, 30, 40, 0.6) 0%, rgba(15, 15, 20, 0.4) 100%);
          border: 1px solid rgba(255, 255, 255, 0.05);
          padding: 2.5rem;
          border-radius: 20px;
          backdrop-filter: blur(20px);
          text-align: left;
          position: relative;
          overflow: hidden;
        }

        .glass-card::before {
          content: '';
          position: absolute;
          top: 0;
          left: 0;
          right: 0;
          height: 1px;
          background: linear-gradient(90deg, transparent, rgba(255,255,255,0.2), transparent);
        }

        .card-icon {
          font-size: 2.5rem;
          margin-bottom: 1.5rem;
          background: rgba(255, 255, 255, 0.05);
          width: 64px;
          height: 64px;
          display: flex;
          align-items: center;
          justify-content: center;
          border-radius: 16px;
          border: 1px solid rgba(255, 255, 255, 0.1);
        }

        .glass-card h3 {
          margin-top: 0;
          font-size: 1.4rem;
          margin-bottom: 1rem;
        }
        
        .glass-card p {
          margin: 0;
          font-size: 0.95rem;
          line-height: 1.6;
        }
      `}</style>
    </motion.div>
  );
}
