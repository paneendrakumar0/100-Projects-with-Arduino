import { getLearnArduinoChapter, getLearnArduinoChapters } from '@/lib/markdown';
import MarkdownRenderer from '@/components/MarkdownRenderer';
import TableOfContents from '@/components/TableOfContents';
import Pagination from '@/components/Pagination';

export async function generateStaticParams() {
  const chapters = getLearnArduinoChapters();
  return chapters.map((chapter) => ({
    chapter: chapter.slug,
  }));
}

export default async function ChapterPage({ params }) {
  const resolvedParams = await params;
  const chapters = getLearnArduinoChapters();
  const currentIndex = chapters.findIndex(c => c.slug === resolvedParams.chapter);
  
  if (currentIndex === -1) {
    return <div>Chapter not found</div>;
  }

  const chapter = chapters[currentIndex];
  
  const prevChapter = currentIndex > 0 ? chapters[currentIndex - 1] : null;
  const nextChapter = currentIndex < chapters.length - 1 ? chapters[currentIndex + 1] : null;

  const prev = prevChapter ? { url: `/learn/${prevChapter.slug}`, title: prevChapter.title } : null;
  const next = nextChapter ? { url: `/learn/${nextChapter.slug}`, title: nextChapter.title } : null;

  return (
    <div className="page-with-toc">
      <article className="prose prose-invert">
        <MarkdownRenderer content={chapter.content} />
        <Pagination prev={prev} next={next} />
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
          padding-bottom: 4rem;
        }
      `}</style>
    </div>
  );
}
