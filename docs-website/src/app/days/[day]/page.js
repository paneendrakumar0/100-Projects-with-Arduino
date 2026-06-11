import { getDayLog, get100DaysLogs } from '@/lib/markdown';
import MarkdownRenderer from '@/components/MarkdownRenderer';
import TableOfContents from '@/components/TableOfContents';
import Pagination from '@/components/Pagination';
import MarkCompleteButton from '@/components/MarkCompleteButton';

export async function generateStaticParams() {
  const days = get100DaysLogs();
  return days.map((day) => ({
    day: day.slug,
  }));
}

export default async function DayPage({ params }) {
  const resolvedParams = await params;
  const days = get100DaysLogs();
  const currentIndex = days.findIndex(d => d.slug === resolvedParams.day);

  if (currentIndex === -1) {
    return <div>Day not found</div>;
  }

  const day = days[currentIndex];
  
  const prevDay = currentIndex > 0 ? days[currentIndex - 1] : null;
  const nextDay = currentIndex < days.length - 1 ? days[currentIndex + 1] : null;

  const prev = prevDay ? { url: `/days/${prevDay.slug}`, title: prevDay.title } : null;
  const next = nextDay ? { url: `/days/${nextDay.slug}`, title: nextDay.title } : null;

  return (
    <div className="page-with-toc">
      <article className="prose prose-invert">
        <MarkdownRenderer content={day.content} />
        <MarkCompleteButton dayId={day.slug} />
        <Pagination prev={prev} next={next} />
      </article>
      <TableOfContents />
    </div>
  );
}
