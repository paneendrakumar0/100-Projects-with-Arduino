'use client';

import { useState } from 'react';
import { Check, Copy } from 'lucide-react';

export default function CopyButton({ text }) {
  const [isCopied, setIsCopied] = useState(false);

  const handleCopy = async () => {
    try {
      await navigator.clipboard.writeText(text);
      setIsCopied(true);
      setTimeout(() => setIsCopied(false), 2000);
    } catch (err) {
      console.error('Failed to copy text: ', err);
    }
  };

  return (
    <button
      onClick={handleCopy}
      className="copy-btn"
      aria-label="Copy to clipboard"
    >
      {isCopied ? <Check size={16} color="#00e5ff" /> : <Copy size={16} />}
      <style jsx>{`
        .copy-btn {
          position: absolute;
          top: 0.75rem;
          right: 0.75rem;
          background: rgba(255, 255, 255, 0.1);
          border: 1px solid rgba(255, 255, 255, 0.1);
          color: #a1a1aa;
          padding: 0.4rem;
          border-radius: 6px;
          cursor: pointer;
          transition: all 0.2s ease;
          display: flex;
          align-items: center;
          justify-content: center;
          opacity: 0;
        }

        .copy-btn:hover {
          color: #fff;
          background: rgba(255, 255, 255, 0.2);
        }
      `}</style>
    </button>
  );
}
