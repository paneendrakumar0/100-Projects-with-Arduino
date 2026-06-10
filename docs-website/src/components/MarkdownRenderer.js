'use client';

import ReactMarkdown from 'react-markdown';
import rehypeHighlight from 'rehype-highlight';
import rehypeRaw from 'rehype-raw';
import rehypeSlug from 'rehype-slug';
import 'highlight.js/styles/atom-one-dark.css';
import CopyButton from './CopyButton';

// Utility to extract raw text from React node children
function extractText(node) {
  if (typeof node === 'string') return node;
  if (Array.isArray(node)) return node.map(extractText).join('');
  if (node && node.props && node.props.children) {
    return extractText(node.props.children);
  }
  return '';
}

export default function MarkdownRenderer({ content }) {
  return (
    <ReactMarkdown
      rehypePlugins={[rehypeHighlight, rehypeRaw, rehypeSlug]}
      components={{
        pre({ node, children, ...props }) {
          const rawText = extractText(children);
          return (
            <div className="code-block-wrapper">
              <CopyButton text={rawText} />
              <pre {...props}>{children}</pre>
              <style jsx>{`
                .code-block-wrapper {
                  position: relative;
                }
                .code-block-wrapper:hover :global(.copy-btn) {
                  opacity: 1;
                }
              `}</style>
            </div>
          );
        }
      }}
    >
      {content}
    </ReactMarkdown>
  );
}
