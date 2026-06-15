'use client';

import { motion } from 'framer-motion';
import Link from 'next/link';
import { ArrowRight, ChevronLeft, ChevronRight } from 'lucide-react';
import { useState, useRef } from 'react';

export default function DayCarousel({ days = [] }) {
  const scrollRef = useRef(null);
  const [canScrollLeft, setCanScrollLeft] = useState(false);
  const [canScrollRight, setCanScrollRight] = useState(true);

  if (!days || days.length === 0) return null;

  const handleScroll = () => {
    if (scrollRef.current) {
      const { scrollLeft, scrollWidth, clientWidth } = scrollRef.current;
      setCanScrollLeft(scrollLeft > 0);
      setCanScrollRight(scrollLeft < scrollWidth - clientWidth - 10);
    }
  };

  const scroll = (direction) => {
    if (scrollRef.current) {
      const amount = direction === 'left' ? -300 : 300;
      scrollRef.current.scrollBy({ left: amount, behavior: 'smooth' });
    }
  };

  return (
    <div className="carousel-wrapper">
      <div className="carousel-header">
        <h3>Up Next</h3>
        <div className="carousel-controls">
          <button 
            onClick={() => scroll('left')} 
            disabled={!canScrollLeft}
            aria-label="Scroll left"
          >
            <ChevronLeft size={20} />
          </button>
          <button 
            onClick={() => scroll('right')} 
            disabled={!canScrollRight}
            aria-label="Scroll right"
          >
            <ChevronRight size={20} />
          </button>
        </div>
      </div>

      <div 
        className="carousel-track" 
        ref={scrollRef}
        onScroll={handleScroll}
      >
        {days.map((day, idx) => (
          <motion.div 
            key={day.slug}
            className="carousel-card"
            whileHover={{ y: -5 }}
            transition={{ type: "spring", stiffness: 300 }}
          >
            <div className="card-badge">Day {day.dayNumber || idx + 1}</div>
            <h4>{day.title}</h4>
            <p>{day.excerpt ? day.excerpt.substring(0, 60) + '...' : 'Continue your hardware journey.'}</p>
            <Link href={`/days/${day.slug}`} className="card-link">
              Start Project <ArrowRight size={16} />
            </Link>
          </motion.div>
        ))}
      </div>

      <style jsx>{`
        .carousel-wrapper {
          margin: 4rem 0 2rem;
          padding-top: 2rem;
          border-top: 1px solid var(--glass-border);
        }
        .carousel-header {
          display: flex;
          justify-content: space-between;
          align-items: center;
          margin-bottom: 1.5rem;
        }
        .carousel-header h3 {
          margin: 0;
          font-size: 1.5rem;
        }
        .carousel-controls {
          display: flex;
          gap: 0.5rem;
        }
        .carousel-controls button {
          background: rgba(255,255,255,0.05);
          border: 1px solid var(--glass-border);
          color: var(--text-primary);
          width: 36px;
          height: 36px;
          border-radius: 50%;
          display: flex;
          align-items: center;
          justify-content: center;
          cursor: pointer;
          transition: all 0.2s;
        }
        .carousel-controls button:disabled {
          opacity: 0.3;
          cursor: not-allowed;
        }
        .carousel-controls button:not(:disabled):hover {
          background: rgba(255,255,255,0.1);
          color: var(--accent-primary);
        }
        .carousel-track {
          display: flex;
          gap: 1.5rem;
          overflow-x: auto;
          padding-bottom: 1.5rem;
          scroll-snap-type: x mandatory;
          scrollbar-width: none;
          -ms-overflow-style: none;
        }
        .carousel-track::-webkit-scrollbar {
          display: none;
        }
        .carousel-card {
          min-width: 280px;
          flex-shrink: 0;
          background: linear-gradient(180deg, rgba(255,255,255,0.03) 0%, rgba(255,255,255,0.01) 100%);
          border: 1px solid var(--glass-border);
          border-radius: 12px;
          padding: 1.5rem;
          display: flex;
          flex-direction: column;
          scroll-snap-align: start;
        }
        .card-badge {
          align-self: flex-start;
          background: rgba(0, 240, 255, 0.1);
          color: var(--accent-primary);
          padding: 0.2rem 0.6rem;
          border-radius: 999px;
          font-size: 0.75rem;
          font-weight: 700;
          margin-bottom: 1rem;
        }
        .carousel-card h4 {
          margin: 0 0 0.5rem;
          font-size: 1.1rem;
          color: #fff;
        }
        .carousel-card p {
          margin: 0 0 1.5rem;
          font-size: 0.85rem;
          color: var(--text-muted);
          flex: 1;
        }
        .card-link {
          display: flex;
          align-items: center;
          gap: 0.5rem;
          font-weight: 600;
          font-size: 0.9rem;
          color: var(--text-primary);
          transition: color 0.2s;
        }
        .card-link:hover {
          color: var(--accent-primary);
        }
      `}</style>
    </div>
  );
}
