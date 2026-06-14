'use client';

import React from 'react';
import { Code2 } from 'lucide-react';

export default function ViewSourceButton({ dayId }) {
  const repoUrl = `https://github.com/paneendrakumar0/100-Projects-with-Arduino/tree/main/${dayId}`;

  return (
    <div className="mt-8 pt-8 border-t border-zinc-800">
      <h3 className="text-lg font-semibold text-zinc-100 mb-4">
        Source Code
      </h3>
      <a 
        href={repoUrl}
        target="_blank"
        rel="noopener noreferrer"
        className="inline-flex items-center gap-2 px-6 py-3 bg-zinc-800 hover:bg-zinc-700 text-zinc-100 font-medium rounded-xl transition-all border border-zinc-700 hover:border-zinc-500"
      >
        <Code2 size={18} />
        <span>View Source on GitHub</span>
      </a>
    </div>
  );
}
