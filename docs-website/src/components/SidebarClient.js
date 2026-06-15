'use client';

import Link from 'next/link';
import { usePathname } from 'next/navigation';
import { motion } from 'framer-motion';
import { useEffect, useState } from 'react';
import { Menu, X, Cpu, CheckCircle2, Award } from 'lucide-react';
import SearchModal from './SearchModal';

export default function SidebarClient({ chapters, days, searchIndex }) {
  const [isOpen, setIsOpen] = useState(false);
  const [completedDays, setCompletedDays] = useState([]);
  const pathname = usePathname();

  const container = {
    hidden: { opacity: 0 },
    show: {
      opacity: 1,
      transition: { staggerChildren: 0.05, delayChildren: 0.2 },
    },
  };

  const item = {
    hidden: { opacity: 0, x: -20 },
    show: { opacity: 1, x: 0, transition: { type: 'spring', stiffness: 300, damping: 24 } },
  };

  // Close sidebar on route change on mobile
  const [prevPathname, setPrevPathname] = useState(pathname);
  if (pathname !== prevPathname) {
    setPrevPathname(pathname);
    setIsOpen(false);
  }

  // Handle local progress tracking
  useEffect(() => {
    const updateProgress = () => {
      setCompletedDays(JSON.parse(localStorage.getItem('completedDays') || '[]'));
    };

    updateProgress();
    window.addEventListener('progressUpdated', updateProgress);
    return () => window.removeEventListener('progressUpdated', updateProgress);
  }, []);

  return (
    <>
      <button 
        className="mobile-toggle"
        onClick={() => setIsOpen(!isOpen)}
        aria-label="Toggle Menu"
      >
        {isOpen ? <X size={24} /> : <Menu size={24} />}
      </button>

      <div className={`sidebar-overlay ${isOpen ? 'open' : ''}`} onClick={() => setIsOpen(false)}></div>

      <aside className={`sidebar ${isOpen ? 'open' : ''}`}>
        <motion.div
          initial={{ opacity: 0, y: -20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.5 }}
        >
          <Link href="/" className="logo">
            <Cpu size={28} color="#00d4ff" />
            <span>Arduino 100</span>
          </Link>
        </motion.div>

        <SearchModal searchIndex={searchIndex} />

      <motion.div variants={container} initial="hidden" animate="show" className="nav-section">
        <motion.div variants={item} className="nav-title">Zero to Hero Guide</motion.div>
        <ul className="nav-list">
          {chapters.map((chapter) => {
            const isActive = pathname === `/learn/${chapter.slug}`;
            const isCompleted = completedDays.includes(chapter.slug);
            return (
              <motion.li variants={item} key={chapter.slug} className="nav-item">
                <Link href={`/learn/${chapter.slug}`} className={`nav-link ${isActive ? 'active' : ''} flex items-center justify-between`}>
                  <span>{chapter.title}</span>
                  {isCompleted && <CheckCircle2 className="w-4 h-4 text-green-400" />}
                </Link>
              </motion.li>
            );
          })}
        </ul>
      </motion.div>

      <motion.div variants={container} initial="hidden" animate="show" className="nav-section">
        <motion.div variants={item} className="nav-title">100 Days Journey</motion.div>
        <ul className="nav-list">
          {days.map((day) => {
            const isActive = pathname === `/days/${day.slug}`;
            const isCompleted = completedDays.includes(day.slug);
            return (
              <motion.li variants={item} key={day.slug} className="nav-item">
                <Link href={`/days/${day.slug}`} className={`nav-link ${isActive ? 'active' : ''} flex items-center justify-between`}>
                  <span>Day {day.dayNumber}: {day.title}</span>
                  {isCompleted && <CheckCircle2 className="w-4 h-4 text-green-400" />}
                </Link>
              </motion.li>
            );
          })}
        </ul>
      </motion.div>
      <motion.div variants={container} initial="hidden" animate="show" className="nav-section mt-8">
        <div className="border-t border-zinc-800/50 pt-6">
          <Link href="/certificate" className="flex items-center gap-3 p-4 bg-gradient-to-r from-indigo-500/10 to-purple-500/10 hover:from-indigo-500/20 hover:to-purple-500/20 border border-indigo-500/20 rounded-xl transition-all group">
            <div className="w-10 h-10 rounded-full bg-indigo-500/20 flex items-center justify-center group-hover:scale-110 transition-transform">
              <Award className="text-indigo-400 w-5 h-5" />
            </div>
            <div>
              <div className="text-sm font-bold text-indigo-300">Your Certificate</div>
              <div className="text-xs text-indigo-400/70">{completedDays.length}/100 Days Completed</div>
            </div>
          </Link>
        </div>
      </motion.div>
      </aside>

      <style jsx>{`
        .sidebar {
          width: var(--sidebar-width);
          position: fixed;
          top: 0;
          bottom: 0;
          left: 0;
          background: var(--glass-bg);
          backdrop-filter: blur(40px);
          -webkit-backdrop-filter: blur(40px);
          border-right: 1px solid var(--glass-border);
          box-shadow: inset -1px 0 0 0 var(--glass-border-highlight), 10px 0 30px -10px rgba(0,0,0,0.5);
          overflow-y: auto;
          padding: 2.5rem 2rem;
          z-index: 50;
          display: flex;
          flex-direction: column;
          transition: background 0.3s var(--spring-smooth);
        }

        .sidebar:hover {
          background: var(--glass-bg-hover);
        }

        .logo {
          font-family: 'Outfit', sans-serif;
          font-size: 1.8rem;
          font-weight: 900;
          color: #fff;
          text-decoration: none;
          display: flex;
          align-items: center;
          gap: 0.75rem;
          margin-bottom: 2.5rem;
        }

        .logo span {
          background: linear-gradient(135deg, #00d4ff, #79f2c0, #ffc857);
          -webkit-background-clip: text;
          -webkit-text-fill-color: transparent;
        }

        .nav-section {
          margin-bottom: 2.5rem;
        }

        .nav-title {
          font-size: 0.75rem;
          text-transform: uppercase;
          letter-spacing: 0.08em;
          color: var(--text-secondary);
          font-weight: 800;
          margin-bottom: 1rem;
        }

        .nav-list {
          list-style: none;
          padding: 0;
        }

        .nav-link {
          display: block;
          padding: 0.65rem 1rem;
          color: var(--text-secondary);
          border-radius: 8px;
          transition: all 0.3s var(--spring-bouncy);
          font-size: 0.95rem;
          font-weight: 500;
          position: relative;
          overflow: hidden;
          margin-bottom: 0.25rem;
          border: 1px solid transparent;
        }

        .nav-link:hover {
          color: var(--text-primary);
          background: rgba(255, 255, 255, 0.04);
          transform: translateX(4px);
          border-color: rgba(255, 255, 255, 0.05);
        }

        .nav-link.active {
          color: var(--text-primary);
          background: linear-gradient(90deg, rgba(0, 240, 255, 0.1) 0%, transparent 100%);
          border-left: 3px solid var(--accent-primary);
          border-radius: 0 8px 8px 0;
          border-color: transparent transparent transparent var(--accent-primary);
        }

        .nav-link.active::before {
          content: '';
          position: absolute;
          left: -3px;
          top: 0;
          bottom: 0;
          width: 3px;
          background: var(--accent-primary);
          box-shadow: 0 0 15px 2px var(--accent-primary);
        }

        .mobile-toggle {
          display: none;
          position: fixed;
          top: 1rem;
          right: 1rem;
          z-index: 100;
          background: var(--glass-bg);
          border: 1px solid var(--glass-border);
          color: #fff;
          padding: 0.5rem;
          border-radius: 8px;
          cursor: pointer;
          backdrop-filter: blur(10px);
        }

        .sidebar-overlay {
          display: none;
          position: fixed;
          top: 0;
          left: 0;
          right: 0;
          bottom: 0;
          background: rgba(0, 0, 0, 0.5);
          backdrop-filter: blur(4px);
          z-index: 40;
          opacity: 0;
          transition: opacity 0.3s ease;
          pointer-events: none;
        }

        .sidebar-overlay.open {
          opacity: 1;
          pointer-events: all;
        }

        @media (max-width: 768px) {
          .mobile-toggle {
            display: block;
          }
          .sidebar-overlay {
            display: block;
          }
          .sidebar {
            transform: translateX(-100%);
            transition: transform 0.3s cubic-bezier(0.16, 1, 0.3, 1);
            width: 80% !important;
            max-width: 340px;
          }
          .sidebar.open {
            transform: translateX(0);
          }
        }
      `}</style>
    </>
  );
}
