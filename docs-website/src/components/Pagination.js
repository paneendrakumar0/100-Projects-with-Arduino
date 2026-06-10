'use client';

import Link from 'next/link';
import { ChevronLeft, ChevronRight } from 'lucide-react';

export default function Pagination({ prev, next }) {
  if (!prev && !next) return null;

  return (
    <nav className="pagination-container">
      {prev ? (
        <Link href={prev.url} className="pagination-link prev">
          <ChevronLeft size={20} className="icon" />
          <div className="content">
            <span className="label">Previous</span>
            <span className="title">{prev.title}</span>
          </div>
        </Link>
      ) : (
        <div className="empty-spacer"></div>
      )}

      {next ? (
        <Link href={next.url} className="pagination-link next">
          <div className="content">
            <span className="label">Next</span>
            <span className="title">{next.title}</span>
          </div>
          <ChevronRight size={20} className="icon" />
        </Link>
      ) : (
        <div className="empty-spacer"></div>
      )}

      <style jsx>{`
        .pagination-container {
          display: flex;
          justify-content: space-between;
          align-items: center;
          margin-top: 4rem;
          padding-top: 2rem;
          border-top: 1px solid rgba(255, 255, 255, 0.1);
          gap: 1rem;
        }

        .pagination-link {
          display: flex;
          align-items: center;
          gap: 1rem;
          padding: 1.5rem;
          background: rgba(255, 255, 255, 0.03);
          border: 1px solid rgba(255, 255, 255, 0.05);
          border-radius: 12px;
          text-decoration: none;
          flex: 1;
          transition: all 0.3s ease;
          position: relative;
          overflow: hidden;
        }

        .pagination-link:hover {
          background: rgba(255, 255, 255, 0.06);
          border-color: rgba(0, 229, 255, 0.3);
          transform: translateY(-2px);
          box-shadow: 0 10px 25px -5px rgba(0, 0, 0, 0.5), 0 0 15px rgba(0, 229, 255, 0.1);
        }

        .pagination-link.next {
          text-align: right;
          justify-content: flex-end;
        }

        .icon {
          color: #666;
          transition: color 0.3s ease;
        }

        .pagination-link:hover .icon {
          color: #00e5ff;
        }

        .content {
          display: flex;
          flex-direction: column;
        }

        .label {
          font-size: 0.75rem;
          text-transform: uppercase;
          letter-spacing: 0.1em;
          color: #666;
          margin-bottom: 0.25rem;
        }

        .title {
          font-size: 1rem;
          color: #fff;
          font-weight: 500;
        }

        .empty-spacer {
          flex: 1;
        }

        @media (max-width: 640px) {
          .pagination-container {
            flex-direction: column;
          }
          .pagination-link {
            width: 100%;
          }
          .pagination-link.next {
            text-align: left;
            justify-content: flex-start;
            flex-direction: row-reverse;
          }
        }
      `}</style>
    </nav>
  );
}
