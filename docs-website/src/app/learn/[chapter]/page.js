import { getLearnArduinoChapter, getLearnArduinoChapters } from '@/lib/markdown';
import MarkdownRenderer from '@/components/MarkdownRenderer';
import TableOfContents from '@/components/TableOfContents';

export async function generateStaticParams() {
  const chapters = getLearnArduinoChapters();
  return chapters.map((chapter) => ({
    chapter: chapter.slug,
  }));
}

export default async function ChapterPage({ params }) {
  const resolvedParams = await params;
  const chapter = getLearnArduinoChapter(resolvedParams.chapter);

  if (!chapter) {
    return <div>Chapter not found</div>;
  }

  return (
    <div className="page-with-toc">
      <article className="prose prose-invert">
        <MarkdownRenderer content={chapter.content} />
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
