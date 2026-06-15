'use client';

import ReactMarkdown from 'react-markdown';
import rehypeHighlight from 'rehype-highlight';
import rehypeRaw from 'rehype-raw';
import rehypeSlug from 'rehype-slug';
import 'highlight.js/styles/atom-one-dark.css';
import CopyButton from './CopyButton';
import { motion } from 'framer-motion';

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
        h1: ({node, ...props}) => <motion.h1 initial={{opacity: 0, y: 10}} whileInView={{opacity: 1, y: 0}} viewport={{once: true}} {...props} />,
        h2: ({node, ...props}) => <motion.h2 initial={{opacity: 0, y: 10}} whileInView={{opacity: 1, y: 0}} viewport={{once: true}} {...props} />,
        h3: ({node, ...props}) => <motion.h3 initial={{opacity: 0, y: 10}} whileInView={{opacity: 1, y: 0}} viewport={{once: true}} {...props} />,
        p: ({node, ...props}) => <motion.p initial={{opacity: 0, y: 10}} whileInView={{opacity: 1, y: 0}} viewport={{once: true, margin: "-50px"}} {...props} />,
        li: ({node, ...props}) => <motion.li initial={{opacity: 0, x: -10}} whileInView={{opacity: 1, x: 0}} viewport={{once: true}} {...props} />,
        pre({ node, children, ...props }) {
          const rawText = extractText(children);
          return (
            <motion.div 
              className="code-block-wrapper"
              initial={{opacity: 0, scale: 0.98}}
              whileInView={{opacity: 1, scale: 1}}
              viewport={{once: true}}
              transition={{duration: 0.4}}
            >
              <div className="ambient-glow"></div>
              <CopyButton text={rawText} />
              <pre {...props}>{children}</pre>
              <style jsx>{`
                .code-block-wrapper {
                  position: relative;
                  margin: 2rem 0;
                }
                .ambient-glow {
                  position: absolute;
                  inset: 0;
                  background: linear-gradient(135deg, rgba(0, 240, 255, 0.15), rgba(57, 255, 20, 0.05));
                  filter: blur(20px);
                  opacity: 0.5;
                  border-radius: inherit;
                  z-index: -1;
                  transition: opacity 0.3s ease;
                }
                .code-block-wrapper:hover .ambient-glow {
                  opacity: 0.8;
                }
                .code-block-wrapper:hover :global(.copy-btn) {
                  opacity: 1;
                }
                .code-block-wrapper pre {
                  margin: 0 !important;
                }
              `}</style>
            </motion.div>
          );
        }
      }}
    >
      {content}
    </ReactMarkdown>
  );
}
