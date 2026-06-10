import { getDayLog, get100DaysLogs } from '@/lib/markdown';
import MarkdownRenderer from '@/components/MarkdownRenderer';
import TableOfContents from '@/components/TableOfContents';

export async function generateStaticParams() {
  const days = get100DaysLogs();
  return days.map((day) => ({
    day: day.slug,
  }));
}

export default async function DayPage({ params }) {
  const resolvedParams = await params;
  const day = getDayLog(resolvedParams.day);

  if (!day) {
    return <div>Day not found</div>;
  }

  return (
    <div className="page-with-toc">
      <article className="prose prose-invert">
        <MarkdownRenderer content={day.content} />
      </article>
      <TableOfContents />
      <style jsx>{`
        .page-with-toc {
          display: flex;
          position: relative;
          max-width: 1400px;
          margin: 0 auto;
        }
        .prose {
          flex: 1;
          max-width: 850px;
        }
      `}</style>
    </div>
  );
}
