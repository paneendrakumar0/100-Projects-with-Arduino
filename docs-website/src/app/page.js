'use client';

import Link from 'next/link';
import { motion, useMotionTemplate, useMotionValue } from 'framer-motion';
import { ArrowRight, BookOpen, CircuitBoard, Cpu, Gauge, Radio, Wrench } from 'lucide-react';
import ProgressDashboard from '@/components/ProgressDashboard';
import ArduinoSimulator from '@/components/ArduinoSimulator';

const learningTracks = [
  {
    title: 'Beginner runway',
    href: '/learn/Chapter_01_What_Is_Arduino',
    icon: BookOpen,
    detail: 'Electronics basics, Arduino IDE, C++ syntax, breadboard habits, and first upload flow.',
  },
  {
    title: 'Daily builds',
    href: '/days/Day_01_Millis_Blink',
    icon: CircuitBoard,
    detail: 'One focused project per day with code, wiring notes, and concept checkpoints.',
  },
  {
    title: 'Embedded systems',
    href: '/days/Day_51_Modbus_RS485',
    icon: Cpu,
    detail: 'Registers, protocols, interrupts, storage, buses, and robust firmware patterns.',
  },
  {
    title: 'Robotics autonomy',
    href: '/days/Day_96_Differential_Drive_Odometry',
    icon: Gauge,
    detail: 'Odometry, sensor fusion, PID tuning, path tracking, and the final integration arc.',
  },
];

const pillars = [
  { label: 'projects', value: '100', icon: Wrench },
  { label: 'learning phases', value: '5', icon: BookOpen },
  { label: 'hardware domains', value: '12+', icon: CircuitBoard },
  { label: 'robotics capstones', value: '10+', icon: Radio },
];

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
      transition: { staggerChildren: 0.12, delayChildren: 0.16 },
    },
  };

  const item = {
    hidden: { opacity: 0, y: 24 },
    show: { opacity: 1, y: 0, transition: { type: 'spring', stiffness: 180, damping: 22 } },
  };

  return (
    <motion.div
      className="home"
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
              560px circle at ${mouseX}px ${mouseY}px,
              rgba(0, 212, 255, 0.10),
              transparent 78%
            )
          `,
        }}
      />

      <section className="hero-panel">
        <motion.div variants={item} className="hero-copy">
          <div className="hero-badge">
            <span className="badge-pulse"></span>
            Interactive embedded systems guide
          </div>

          <h1>
            100 Days of <span>Arduino</span>
          </h1>

          <p className="hero-subtitle">
            A practical path from your first LED circuit to robotics, sensor fusion,
            control loops, communication buses, low-power firmware, and autonomy.
          </p>

          <div className="hero-buttons">
            <Link href="/learn/Chapter_01_What_Is_Arduino" className="btn btn-primary">
              Start learning <ArrowRight size={18} />
            </Link>
            <Link href="/days/Day_01_Millis_Blink" className="btn btn-secondary">
              Open Day 1
            </Link>
          </div>
        </motion.div>

        <motion.div variants={item} className="route-map" aria-label="Curriculum overview">
          <div className="route-header">
            <span>Route map</span>
            <strong>Beginner to autonomy</strong>
          </div>
          <div className="route-steps">
            <span>Foundations</span>
            <span>Interfaces</span>
            <span>Motion</span>
            <span>Systems</span>
            <span>Autonomy</span>
          </div>
          <div className="route-line">
            <span></span>
            <span></span>
            <span></span>
            <span></span>
            <span></span>
          </div>
        </motion.div>
      </section>

      <motion.section variants={container} className="stat-grid" aria-label="Guide highlights">
        {pillars.map(({ label, value, icon: Icon }) => (
          <motion.div variants={item} className="stat-card" key={label}>
            <Icon size={20} />
            <strong>{value}</strong>
            <span>{label}</span>
          </motion.div>
        ))}
      </motion.section>

      <motion.section variants={container} className="track-grid" aria-label="Learning tracks">
        {learningTracks.map(({ title, href, icon: Icon, detail }) => (
          <motion.div variants={item} whileHover={{ y: -4 }} className="track-card" key={title}>
            <Icon size={24} />
            <h2>{title}</h2>
            <p>{detail}</p>
            <Link href={href} className="track-link">
              Continue <ArrowRight size={16} />
            </Link>
          </motion.div>
        ))}
      </motion.section>

      <motion.section variants={item} className="build-band">
        <div>
          <span>Suggested next move</span>
          <h2>Build the first non-blocking circuit before touching sensors.</h2>
        </div>
        <Link href="/days/Day_01_Millis_Blink" className="btn btn-secondary">
          Start Day 1
        </Link>
      </motion.section>

      <motion.section variants={item}>
        <ProgressDashboard />
      </motion.section>

      <motion.section variants={item}>
        <div className="section-header">
          <h2>Interactive Hardware Simulator</h2>
          <p>See components come to life as you learn. Hover or click to interact.</p>
        </div>
        <ArduinoSimulator activePins={['13', 'GND', '5V', 'A0']} />
      </motion.section>

      <style jsx>{`
        .home {
          display: flex;
          flex-direction: column;
          gap: 2rem;
          padding: 2rem 0 4rem;
          position: relative;
        }

        .hero-glow-bg {
          position: absolute;
          inset: 0;
          z-index: -1;
          pointer-events: none;
          opacity: 0;
          transition: opacity 0.45s ease;
        }

        .home:hover .hero-glow-bg {
          opacity: 1;
        }

        .hero-panel {
          display: grid;
          grid-template-columns: minmax(0, 1.25fr) minmax(280px, 0.75fr);
          gap: 3rem;
          align-items: center;
        }

        .hero-copy {
          text-align: left;
        }

        .hero-badge {
          display: inline-flex;
          align-items: center;
          gap: 0.5rem;
          background: rgba(0, 212, 255, 0.10);
          border: 1px solid rgba(0, 212, 255, 0.24);
          padding: 0.5rem 1rem;
          border-radius: 999px;
          font-size: 0.85rem;
          font-weight: 700;
          color: #7be8ff;
          margin-bottom: 2rem;
        }

        .badge-pulse {
          width: 8px;
          height: 8px;
          background: #79f2c0;
          border-radius: 50%;
          box-shadow: 0 0 10px #79f2c0;
          animation: pulse 2s infinite;
        }

        @keyframes pulse {
          0% { box-shadow: 0 0 0 0 rgba(121, 242, 192, 0.65); }
          70% { box-shadow: 0 0 0 10px rgba(121, 242, 192, 0); }
          100% { box-shadow: 0 0 0 0 rgba(121, 242, 192, 0); }
        }

        .home h1 {
          font-size: clamp(3.2rem, 8vw, 6.4rem);
          margin: 0 0 1.5rem;
          line-height: 0.95;
        }

        .home h1 span {
          background: linear-gradient(90deg, #79f2c0, #00d4ff, #ffc857);
          -webkit-background-clip: text;
          -webkit-text-fill-color: transparent;
        }

        .hero-subtitle {
          font-size: 1.25rem;
          color: var(--text-secondary);
          max-width: 720px;
          margin: 0 0 2.5rem;
          line-height: 1.8;
        }

        .hero-buttons {
          display: flex;
          gap: 1rem;
          flex-wrap: wrap;
        }

        .btn {
          padding: 1rem 2rem;
          border-radius: 8px;
          font-weight: 700;
          font-size: 1rem;
          transition: all 0.25s cubic-bezier(0.16, 1, 0.3, 1);
          border: none;
          cursor: pointer;
          text-decoration: none;
          display: inline-flex;
          align-items: center;
          justify-content: center;
          gap: 0.5rem;
        }

        .btn-primary {
          background: #79f2c0;
          color: #03120d;
          box-shadow: 0 12px 30px -14px rgba(121, 242, 192, 0.75);
        }

        .btn-primary:hover {
          transform: translateY(-3px);
          color: #03120d;
          background: #a9ffd9;
        }

        .btn-secondary {
          background: rgba(255, 255, 255, 0.05);
          color: white;
          border: 1px solid rgba(255, 255, 255, 0.12);
        }

        .btn-secondary:hover {
          background: rgba(255, 255, 255, 0.1);
          transform: translateY(-3px);
          color: white;
          border-color: rgba(255, 255, 255, 0.24);
        }

        .route-map {
          min-height: 360px;
          border: 1px solid var(--glass-border);
          border-radius: 8px;
          background:
            linear-gradient(145deg, rgba(17, 24, 39, 0.82), rgba(8, 10, 14, 0.94)),
            repeating-linear-gradient(90deg, transparent 0, transparent 42px, rgba(255,255,255,0.04) 43px);
          padding: 1.5rem;
          display: flex;
          flex-direction: column;
          justify-content: space-between;
          box-shadow: 0 24px 60px -40px rgba(0,0,0,0.85);
        }

        .route-header {
          display: flex;
          flex-direction: column;
          gap: 0.5rem;
        }

        .route-header span,
        .build-band span {
          color: var(--accent-warm);
          font-size: 0.78rem;
          font-weight: 800;
          text-transform: uppercase;
        }

        .route-header strong {
          font-family: 'Outfit', sans-serif;
          font-size: 1.8rem;
        }

        .route-steps {
          display: grid;
          grid-template-columns: repeat(5, 1fr);
          gap: 0.5rem;
          font-size: 0.75rem;
          color: var(--text-secondary);
          text-align: center;
        }

        .route-line {
          display: grid;
          grid-template-columns: repeat(5, 1fr);
          align-items: center;
          gap: 0.5rem;
        }

        .route-line span {
          height: 10px;
          border-radius: 999px;
          background: linear-gradient(90deg, #79f2c0, #00d4ff, #ffc857);
          box-shadow: 0 0 24px rgba(0, 212, 255, 0.24);
        }

        .stat-grid,
        .track-grid {
          display: grid;
          grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
          gap: 1rem;
        }

        .stat-card,
        .track-card {
          border: 1px solid var(--glass-border);
          border-radius: 8px;
          background: rgba(255, 255, 255, 0.04);
          padding: 1.25rem;
        }

        .stat-card {
          display: grid;
          gap: 0.35rem;
        }

        .stat-card svg,
        .track-card svg {
          color: var(--accent-primary);
        }

        .stat-card strong {
          font-family: 'Outfit', sans-serif;
          font-size: 2rem;
          color: #fff;
          line-height: 1;
        }

        .stat-card span {
          color: var(--text-secondary);
          font-size: 0.9rem;
        }

        .track-grid {
          grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
        }

        .track-card h2 {
          border: 0;
          padding: 0;
          margin: 1rem 0 0.75rem;
          font-size: 1.25rem;
        }

        .track-card p {
          font-size: 0.95rem;
          min-height: 6.2rem;
        }

        .track-link {
          display: inline-flex;
          align-items: center;
          gap: 0.4rem;
          font-weight: 700;
        }

        .build-band {
          border: 1px solid rgba(255, 200, 87, 0.24);
          border-radius: 8px;
          padding: 1.5rem;
          background: rgba(255, 200, 87, 0.06);
          display: flex;
          justify-content: space-between;
          gap: 2rem;
          align-items: center;
        }

        .build-band h2 {
          border: 0;
          padding: 0;
          margin: 0.35rem 0 0;
          font-size: 1.45rem;
        }

        @media (max-width: 900px) {
          .hero-panel {
            grid-template-columns: 1fr;
          }

          .route-map {
            min-height: 260px;
          }
        }

        @media (max-width: 620px) {
          .home h1 {
            font-size: 3.2rem;
          }

          .hero-buttons,
          .build-band {
            align-items: stretch;
            flex-direction: column;
          }

          .btn {
            width: 100%;
          }
        }
      `}</style>
    </motion.div>
  );
}
