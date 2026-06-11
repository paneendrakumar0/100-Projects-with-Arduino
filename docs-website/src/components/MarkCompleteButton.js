'use client';

import React, { useState, useEffect } from 'react';
import { CheckCircle } from 'lucide-react';

export default function MarkCompleteButton({ dayId }) {
  const [isCompleted, setIsCompleted] = useState(false);

  useEffect(() => {
    const completedDays = JSON.parse(localStorage.getItem('completedDays') || '[]');
    if (completedDays.includes(dayId)) {
      setIsCompleted(true);
    }
  }, [dayId]);

  const toggleComplete = () => {
    let completedDays = JSON.parse(localStorage.getItem('completedDays') || '[]');
    
    if (isCompleted) {
      completedDays = completedDays.filter(id => id !== dayId);
    } else {
      if (!completedDays.includes(dayId)) {
        completedDays.push(dayId);
      }
    }
    
    localStorage.setItem('completedDays', JSON.stringify(completedDays));
    setIsCompleted(!isCompleted);
    
    // Dispatch a custom event so the Sidebar can react immediately
    window.dispatchEvent(new Event('progressUpdated'));
  };

  return (
    <div className="mt-12 mb-8 p-6 bg-zinc-900 border border-zinc-800 rounded-lg flex items-center justify-between">
      <div>
        <h3 className="text-lg font-semibold text-zinc-100 mb-1">
          Finished with this day?
        </h3>
        <p className="text-zinc-400 text-sm">
          Mark it as complete to track your progress in the sidebar.
        </p>
      </div>
      <button
        onClick={toggleComplete}
        className={`flex items-center gap-2 px-6 py-3 rounded-full font-medium transition-all ${
          isCompleted 
            ? 'bg-green-600/10 text-green-400 border border-green-600/20 hover:bg-green-600/20' 
            : 'bg-indigo-600 text-white hover:bg-indigo-500'
        }`}
      >
        <CheckCircle className={`w-5 h-5 ${isCompleted ? 'text-green-400' : 'text-indigo-200'}`} />
        {isCompleted ? 'Completed' : 'Mark as Complete'}
      </button>
    </div>
  );
}
