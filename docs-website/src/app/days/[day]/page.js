import { getDayLog, get100DaysLogs } from '@/lib/markdown';
import MarkdownRenderer from '@/components/MarkdownRenderer';
import TableOfContents from '@/components/TableOfContents';
import Pagination from '@/components/Pagination';
import MarkCompleteButton from '@/components/MarkCompleteButton';
import ViewSourceButton from '@/components/ViewSourceButton';
import TwitterShareButton from '@/components/TwitterShareButton';

export async function generateStaticParams() {
  const days = get100DaysLogs();
  return days.map((day) => ({
    day: day.slug,
  }));
}

export async function generateMetadata({ params }) {
  const resolvedParams = await params;
  const day = getDayLog(resolvedParams.day);
  if (!day) return {};
  
  return {
    title: `${day.title} | 100 Days of Arduino`,
    description: `Learn how to build ${day.title} in the 100 Days of Arduino Masterclass.`,
    openGraph: {
      title: `${day.title} | 100 Days of Arduino`,
      description: `Learn how to build ${day.title} in the 100 Days of Arduino Masterclass.`,
      type: 'article',
    },
    twitter: {
      card: 'summary_large_image',
      title: `${day.title} | 100 Days of Arduino`,
      description: `Learn how to build ${day.title} in the 100 Days of Arduino Masterclass.`,
    }
  };
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
        <div className="flex justify-between items-start">
          <ViewSourceButton dayId={day.slug} />
        </div>
        <MarkdownRenderer content={day.content} />
        <div className="flex flex-col sm:flex-row items-center justify-between gap-4 mt-8 pt-8 border-t border-zinc-800">
          <MarkCompleteButton dayId={day.slug} />
          <TwitterShareButton 
            text={`I just built ${day.title} from the 100 Days of Arduino Masterclass! 🛠️⚡`}
            url={`https://github.com/paneendrakumar0/100-Projects-with-Arduino/tree/main/${day.slug}`}
          />
        </div>
        <Pagination prev={prev} next={next} />
      </article>
      <TableOfContents />
    </div>
  );
}
