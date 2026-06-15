'use client';

import { useState } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import Link from 'next/link';
import { Cpu, ChevronDown, ChevronUp } from 'lucide-react';

export default function HardwareGrid({ glossary }) {
  const [searchTerm, setSearchTerm] = useState('');
  const [expandedItems, setExpandedItems] = useState({});

  const toggleItem = (name) => {
    setExpandedItems(prev => ({ ...prev, [name]: !prev[name] }));
  };

  const filtered = glossary.filter(item => 
    item.name.toLowerCase().includes(searchTerm.toLowerCase())
  );

  return (
    <div className="hardware-wrapper">
      <div className="search-bar mb-8">
        <input 
          type="text" 
          placeholder="Search for a component (e.g. MPU6050, L298N, LED)..." 
          value={searchTerm}
          onChange={(e) => setSearchTerm(e.target.value)}
          className="w-full bg-zinc-900/80 border border-zinc-800 rounded-xl px-6 py-4 text-white outline-none focus:border-cyan-500 transition-colors shadow-inner"
        />
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
        {filtered.map((item, i) => (
          <motion.div 
            key={item.name}
            initial={{ opacity: 0, y: 20 }}
            animate={{ opacity: 1, y: 0 }}
            transition={{ delay: i * 0.02 }}
            className="glass-card rounded-xl border border-zinc-800/60 bg-zinc-900/40 backdrop-blur-md overflow-hidden shadow-xl"
          >
            <div 
              className="p-5 flex justify-between items-center cursor-pointer hover:bg-zinc-800/40 transition-colors"
              onClick={() => toggleItem(item.name)}
            >
              <div className="flex items-center gap-3 overflow-hidden">
                <div className="p-2 bg-zinc-800 rounded-lg text-cyan-400 shrink-0 shadow-inner border border-zinc-700">
                  <Cpu size={18} />
                </div>
                <h3 className="font-bold text-zinc-100 truncate">{item.name}</h3>
              </div>
              <div className="flex items-center gap-3 shrink-0">
                <span className="text-xs font-mono bg-zinc-800 border border-zinc-700 px-2 py-1 rounded text-zinc-400 shadow-sm">
                  {item.usedIn.length} project{item.usedIn.length !== 1 ? 's' : ''}
                </span>
                {expandedItems[item.name] ? <ChevronUp size={18} className="text-zinc-500" /> : <ChevronDown size={18} className="text-zinc-500" />}
              </div>
            </div>

            <AnimatePresence>
              {expandedItems[item.name] && (
                <motion.div
                  initial={{ height: 0, opacity: 0 }}
                  animate={{ height: 'auto', opacity: 1 }}
                  exit={{ height: 0, opacity: 0 }}
                  className="px-5 pb-5 border-t border-zinc-800/50 pt-4 bg-zinc-900/80"
                >
                  <ul className="space-y-3">
                    {item.usedIn.map(day => (
                      <li key={day.slug}>
                        <Link href={`/days/${day.slug}`} className="flex items-center gap-3 text-zinc-400 hover:text-cyan-400 transition-colors group">
                          <span className="w-12 text-xs font-mono text-zinc-500 group-hover:text-cyan-500 bg-zinc-800 px-1 py-0.5 rounded text-center shrink-0">
                            Day {day.dayNumber}
                          </span>
                          <span className="text-sm truncate font-medium">{day.title}</span>
                        </Link>
                      </li>
                    ))}
                  </ul>
                </motion.div>
              )}
            </AnimatePresence>
          </motion.div>
        ))}
      </div>
      {filtered.length === 0 && (
        <div className="text-center py-16">
          <Cpu size={48} className="mx-auto text-zinc-700 mb-4" />
          <p className="text-zinc-500 text-lg">
            No components found matching "{searchTerm}"
          </p>
        </div>
      )}
    </div>
  );
}
