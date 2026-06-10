import { getLearnArduinoChapters, get100DaysLogs } from './markdown';

function stripMarkdown(md) {
  if (!md) return '';
  return md
    .replace(/[#_*~`\[\]()]/g, '') // Remove markdown characters
    .replace(/\n+/g, ' ') // Replace newlines with spaces
    .substring(0, 150)
    .trim() + '...';
}

export function getSearchIndex() {
  const chapters = getLearnArduinoChapters();
  const days = get100DaysLogs();

  const chapterIndex = chapters.map(c => ({
    type: 'chapter',
    slug: c.slug,
    title: c.title,
    excerpt: stripMarkdown(c.content)
  }));

  const daysIndex = days.map(d => ({
    type: 'day',
    slug: d.slug,
    title: d.title,
    excerpt: stripMarkdown(d.content),
    dayNumber: d.dayNumber
  }));

  return [...chapterIndex, ...daysIndex];
}
