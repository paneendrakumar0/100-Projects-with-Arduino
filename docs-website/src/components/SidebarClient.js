'use client';

import Link from 'next/link';
import { usePathname } from 'next/navigation';
import { motion } from 'framer-motion';
import { useEffect, useState } from 'react';
import { Menu, X, Cpu } from 'lucide-react';
import SearchModal from './SearchModal';

export default function SidebarClient({ chapters, days, searchIndex }) {
  const [isOpen, setIsOpen] = useState(false);
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
  useEffect(() => {
    // eslint-disable-next-line react-hooks/set-state-in-effect
    setIsOpen(false);
  }, [pathname]);

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
            <Cpu size={28} color="#00e5ff" />
            <span>Arduino 100</span>
          </Link>
        </motion.div>

        <SearchModal searchIndex={searchIndex} />

      <motion.div variants={container} initial="hidden" animate="show" className="nav-section">
        <motion.div variants={item} className="nav-title">Zero to Hero Guide</motion.div>
        <ul className="nav-list">
          {chapters.map((chapter) => {
            const isActive = pathname === `/learn/${chapter.slug}`;
            return (
              <motion.li variants={item} key={chapter.slug} className="nav-item">
                <Link href={`/learn/${chapter.slug}`} className={`nav-link ${isActive ? 'active' : ''}`}>
                  {chapter.title}
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
            return (
              <motion.li variants={item} key={day.slug} className="nav-item">
                <Link href={`/days/${day.slug}`} className={`nav-link ${isActive ? 'active' : ''}`}>
                  Day {day.dayNumber}: {day.title}
                </Link>
              </motion.li>
            );
          })}
        </ul>
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
          backdrop-filter: blur(24px);
          -webkit-backdrop-filter: blur(24px);
          border-right: 1px solid var(--glass-border);
          overflow-y: auto;
          padding: 2.5rem;
          z-index: 50;
          display: flex;
          flex-direction: column;
        }

        .logo {
          font-family: 'Outfit', sans-serif;
          font-size: 1.7rem;
          font-weight: 900;
          color: #fff;
          text-decoration: none;
          display: flex;
          align-items: center;
          gap: 0.75rem;
          margin-bottom: 2.5rem;
        }

        .logo span {
          background: linear-gradient(135deg, #00e5ff, #7000ff);
          -webkit-background-clip: text;
          -webkit-text-fill-color: transparent;
        }

        .nav-section {
          margin-bottom: 2.5rem;
        }

        .nav-title {
          font-size: 0.75rem;
          text-transform: uppercase;
          letter-spacing: 0.15em;
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
          padding: 0.6rem 1rem;
          color: var(--text-secondary);
          border-radius: 8px;
          transition: all 0.3s ease;
          font-size: 0.95rem;
          font-weight: 500;
          position: relative;
          overflow: hidden;
        }

        .nav-link:hover {
          color: #fff;
          background: rgba(255, 255, 255, 0.03);
          transform: translateX(4px);
        }

        .nav-link.active {
          color: #fff;
          background: linear-gradient(90deg, rgba(0, 229, 255, 0.15) 0%, rgba(0,0,0,0) 100%);
          border-left: 3px solid var(--accent-primary);
          border-radius: 0 8px 8px 0;
          text-shadow: 0 0 10px rgba(0, 229, 255, 0.5);
        }

        .nav-link.active::before {
          content: '';
          position: absolute;
          left: 0;
          top: 0;
          bottom: 0;
          width: 1px;
          background: var(--accent-primary);
          box-shadow: 0 0 10px 2px var(--accent-primary);
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
