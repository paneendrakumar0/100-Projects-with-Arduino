import React from 'react';
import { Edit3 } from 'lucide-react';

export default function EditPageButton({ filePath }) {
  const repoBaseUrl = 'https://github.com/paneendrakumar0/100-Projects-with-Arduino/edit/main';
  const editUrl = `${repoBaseUrl}/${filePath}`;

  return (
    <a
      href={editUrl}
      target="_blank"
      rel="noopener noreferrer"
      className="inline-flex items-center gap-2 px-3 py-2 text-zinc-400 hover:text-zinc-100 hover:bg-zinc-800 rounded-md text-sm font-medium transition-colors"
    >
      <Edit3 size={16} />
      <span>Edit this page on GitHub</span>
    </a>
  );
}
