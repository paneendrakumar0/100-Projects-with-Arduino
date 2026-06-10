'use client';

import { useCallback, useState, useEffect, useRef } from 'react';
import { useRouter } from 'next/navigation';
import Fuse from 'fuse.js';
import { Search, X, FileText, Calendar } from 'lucide-react';
import { motion, AnimatePresence } from 'framer-motion';

export default function SearchModal({ searchIndex }) {
  const [isOpen, setIsOpen] = useState(false);
  const [query, setQuery] = useState('');
  const [results, setResults] = useState([]);
  const [selectedIndex, setSelectedIndex] = useState(0);
  const inputRef = useRef(null);
  const router = useRouter();

  const openSearch = useCallback(() => {
    setQuery('');
    setResults([]);
    setSelectedIndex(0);
    setIsOpen(true);
  }, []);

  // Initialize Fuse
  const fuse = useRef(null);
  useEffect(() => {
    fuse.current = new Fuse(searchIndex, {
      keys: ['title', 'excerpt', 'slug'],
      threshold: 0.3,
      includeMatches: true
    });
  }, [searchIndex]);

  // Keyboard shortcut (Cmd+K / Ctrl+K)
  useEffect(() => {
    const handleKeyDown = (e) => {
      if ((e.metaKey || e.ctrlKey) && e.key === 'k') {
        e.preventDefault();
        if (isOpen) {
          setIsOpen(false);
        } else {
          openSearch();
        }
      }
      if (e.key === 'Escape' && isOpen) {
        setIsOpen(false);
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [isOpen, openSearch]);

  // Auto-focus input when opened
  useEffect(() => {
    if (isOpen) {
      setTimeout(() => inputRef.current?.focus(), 100);
    }
  }, [isOpen]);

  // Handle search
  useEffect(() => {
    if (query.length > 1 && fuse.current) {
      const res = fuse.current.search(query).slice(0, 8); // Top 8 results
      setResults(res.map(r => r.item));
      setSelectedIndex(0);
    } else {
      setResults([]);
    }
  }, [query]);

  // Handle arrow navigation
  const handleKeyDown = (e) => {
    if (e.key === 'ArrowDown') {
      e.preventDefault();
      setSelectedIndex((prev) => (prev + 1) % (results.length || 1));
    } else if (e.key === 'ArrowUp') {
      e.preventDefault();
      setSelectedIndex((prev) => (prev - 1 + (results.length || 1)) % (results.length || 1));
    } else if (e.key === 'Enter') {
      e.preventDefault();
      if (results.length > 0) {
        handleSelect(results[selectedIndex]);
      }
    }
  };

  const handleSelect = (item) => {
    setIsOpen(false);
    if (item.type === 'chapter') {
      router.push(`/learn/${item.slug}`);
    } else {
      router.push(`/days/${item.slug}`);
    }
  };

  return (
    <>
      <button className="search-trigger" onClick={openSearch}>
        <Search size={18} />
        <span>Search documentation...</span>
        <kbd className="shortcut">Ctrl K</kbd>
      </button>

      <AnimatePresence>
        {isOpen && (
          <motion.div 
            className="search-backdrop"
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            exit={{ opacity: 0 }}
            onClick={() => setIsOpen(false)}
          >
            <motion.div 
              className="search-modal"
              initial={{ scale: 0.95, opacity: 0, y: -20 }}
              animate={{ scale: 1, opacity: 1, y: 0 }}
              exit={{ scale: 0.95, opacity: 0, y: -20 }}
              onClick={(e) => e.stopPropagation()}
            >
              <div className="search-header">
                <Search size={20} className="search-icon" />
                <input
                  ref={inputRef}
                  type="text"
                  placeholder="Search days, chapters, or concepts..."
                  value={query}
                  onChange={(e) => setQuery(e.target.value)}
                  onKeyDown={handleKeyDown}
                />
                <button className="close-btn" onClick={() => setIsOpen(false)}>
                  <X size={20} />
                </button>
              </div>

              {results.length > 0 && (
                <div className="search-results">
                  {results.map((item, index) => (
                    <div 
                      key={item.slug} 
                      className={`search-item ${index === selectedIndex ? 'selected' : ''}`}
                      onClick={() => handleSelect(item)}
                      onMouseEnter={() => setSelectedIndex(index)}
                    >
                      <div className="item-icon">
                        {item.type === 'chapter' ? <FileText size={18} /> : <Calendar size={18} />}
                      </div>
                      <div className="item-content">
                        <h4>{item.title}</h4>
                        <p>{item.excerpt.substring(0, 80)}...</p>
                      </div>
                      <div className="item-type">
                        {item.type === 'chapter' ? 'Guide' : `Day ${item.dayNumber}`}
                      </div>
                    </div>
                  ))}
                </div>
              )}
              
              {query.length > 1 && results.length === 0 && (
                <div className="search-empty">
                  No results found for &quot;{query}&quot;
                </div>
              )}
            </motion.div>
          </motion.div>
        )}
      </AnimatePresence>

      <style jsx>{`
        .search-trigger {
          display: flex;
          align-items: center;
          gap: 0.75rem;
          width: 100%;
          background: rgba(255, 255, 255, 0.05);
          border: 1px solid var(--glass-border);
          color: var(--text-secondary);
          padding: 0.75rem 1rem;
          border-radius: 8px;
          cursor: pointer;
          margin-bottom: 2rem;
          transition: all 0.3s ease;
        }

        .search-trigger:hover {
          background: rgba(255, 255, 255, 0.1);
          color: #fff;
          border-color: rgba(0, 229, 255, 0.3);
          box-shadow: 0 0 15px rgba(0, 229, 255, 0.1);
        }

        .search-trigger span {
          flex: 1;
          text-align: left;
          font-size: 0.9rem;
        }

        .shortcut {
          background: rgba(0,0,0,0.3);
          border: 1px solid rgba(255,255,255,0.1);
          padding: 0.2rem 0.4rem;
          border-radius: 4px;
          font-size: 0.75rem;
          font-family: 'Fira Code', monospace;
        }

        .search-backdrop {
          position: fixed;
          top: 0;
          left: 0;
          right: 0;
          bottom: 0;
          background: rgba(0, 0, 0, 0.6);
          backdrop-filter: blur(8px);
          z-index: 9999;
          display: flex;
          align-items: flex-start;
          justify-content: center;
          padding-top: 10vh;
        }

        .search-modal {
          background: #111;
          width: 100%;
          max-width: 600px;
          border-radius: 16px;
          border: 1px solid rgba(255, 255, 255, 0.1);
          box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.8), 0 0 0 1px rgba(0, 229, 255, 0.1);
          overflow: hidden;
        }

        .search-header {
          display: flex;
          align-items: center;
          padding: 1rem 1.5rem;
          border-bottom: 1px solid rgba(255, 255, 255, 0.05);
        }

        .search-icon {
          color: var(--accent-primary);
        }

        .search-header input {
          flex: 1;
          background: transparent;
          border: none;
          color: #fff;
          font-size: 1.1rem;
          padding: 0.5rem 1rem;
          outline: none;
          font-family: 'Inter', sans-serif;
        }

        .search-header input::placeholder {
          color: #666;
        }

        .close-btn {
          background: transparent;
          border: none;
          color: #666;
          cursor: pointer;
          transition: color 0.2s;
        }

        .close-btn:hover {
          color: #fff;
        }

        .search-results {
          max-height: 60vh;
          overflow-y: auto;
          padding: 0.5rem;
        }

        .search-item {
          display: flex;
          align-items: center;
          gap: 1rem;
          padding: 1rem;
          border-radius: 8px;
          cursor: pointer;
          transition: all 0.2s;
        }

        .search-item.selected {
          background: rgba(0, 229, 255, 0.1);
          border-left: 3px solid var(--accent-primary);
        }

        .item-icon {
          color: #888;
          display: flex;
          align-items: center;
          justify-content: center;
          background: rgba(255, 255, 255, 0.05);
          width: 40px;
          height: 40px;
          border-radius: 8px;
        }

        .search-item.selected .item-icon {
          color: var(--accent-primary);
          background: rgba(0, 229, 255, 0.15);
        }

        .item-content {
          flex: 1;
        }

        .item-content h4 {
          margin: 0 0 0.25rem 0;
          color: #fff;
          font-size: 1rem;
        }

        .item-content p {
          margin: 0;
          font-size: 0.8rem;
          color: #888;
        }

        .item-type {
          font-size: 0.75rem;
          color: #666;
          background: rgba(255,255,255,0.05);
          padding: 0.2rem 0.5rem;
          border-radius: 4px;
        }

        .search-empty {
          padding: 3rem 1rem;
          text-align: center;
          color: #666;
        }
      `}</style>
    </>
  );
}
