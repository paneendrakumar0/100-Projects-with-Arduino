'use client';

import React from 'react';
import { Github } from 'lucide-react';

export default function ViewSourceButton({ dayId }) {
  const repoUrl = `https://github.com/paneendrakumar0/100-Projects-with-Arduino/tree/main/${dayId}`;

  return (
    <a
      href={repoUrl}
      target="_blank"
      rel="noopener noreferrer"
      className="inline-flex items-center gap-2 px-4 py-2 bg-zinc-800 hover:bg-zinc-700 text-zinc-100 text-sm font-medium rounded-md transition-colors border border-zinc-700 hover:border-zinc-600 mb-6"
    >
      <Github size={18} />
      <span>View Source on GitHub</span>
    </a>
  );
}
