'use client';

import { useEffect, useState } from 'react';
import { motion } from 'framer-motion';

export default function TableOfContents() {
  const [headings, setHeadings] = useState([]);
  const [activeId, setActiveId] = useState('');

  useEffect(() => {
    // Find all h2 and h3 elements within the prose container
    const elements = Array.from(document.querySelectorAll('.prose h2, .prose h3'))
      .map((elem) => ({
        id: elem.id,
        text: elem.innerText,
        level: Number(elem.nodeName.charAt(1)),
        top: elem.getBoundingClientRect().top + window.scrollY,
      }));
    setHeadings(elements);

    const handleScroll = () => {
      const scrollPosition = window.scrollY + 100;
      let currentId = '';
      
      for (const heading of elements) {
        if (scrollPosition >= heading.top) {
          currentId = heading.id;
        } else {
          break;
        }
      }
      setActiveId(currentId);
    };

    window.addEventListener('scroll', handleScroll);
    return () => window.removeEventListener('scroll', handleScroll);
  }, []);

  if (headings.length === 0) return null;

  return (
    <aside className="toc-container">
      <h4 className="toc-title">On this page</h4>
      <nav>
        <ul>
          {headings.map((heading) => (
            <li 
              key={heading.id} 
              className={`toc-item level-${heading.level} ${activeId === heading.id ? 'active' : ''}`}
            >
              <a href={`#${heading.id}`}>{heading.text}</a>
              {activeId === heading.id && (
                <motion.div 
                  className="active-indicator" 
                  layoutId="activeIndicator"
                  initial={false}
                  transition={{ type: 'spring', stiffness: 300, damping: 30 }}
                />
              )}
            </li>
          ))}
        </ul>
      </nav>

      <style jsx>{`
        .toc-container {
          position: fixed;
          top: 8rem;
          right: 2rem;
          width: 250px;
          display: none;
          max-height: calc(100vh - 10rem);
          overflow-y: auto;
        }

        @media (min-width: 1400px) {
          .toc-container {
            display: block;
          }
        }

        .toc-title {
          font-size: 0.8rem;
          text-transform: uppercase;
          letter-spacing: 0.1em;
          color: var(--text-secondary);
          margin-bottom: 1rem;
          font-weight: 700;
        }

        ul {
          list-style: none;
          padding: 0;
          margin: 0;
          border-left: 1px solid var(--glass-border);
        }

        .toc-item {
          position: relative;
          margin-bottom: 0.5rem;
        }

        .toc-item a {
          display: block;
          padding: 0.25rem 0;
          color: #888;
          font-size: 0.85rem;
          transition: color 0.2s;
          text-decoration: none;
        }
        
        .toc-item.level-2 a {
          padding-left: 1rem;
        }

        .toc-item.level-3 a {
          padding-left: 2rem;
          font-size: 0.8rem;
        }

        .toc-item:hover a {
          color: #fff;
        }

        .toc-item.active a {
          color: #00e5ff;
          font-weight: 500;
        }

        .active-indicator {
          position: absolute;
          left: -1px;
          top: 0;
          bottom: 0;
          width: 2px;
          background: #00e5ff;
          box-shadow: 0 0 10px #00e5ff;
        }
      `}</style>
    </aside>
  );
}
