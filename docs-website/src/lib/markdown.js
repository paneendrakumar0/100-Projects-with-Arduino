import fs from 'fs';
import path from 'path';
import matter from 'gray-matter';

// The root of the Next.js app is `docs-website`.
// The root of the repository is one level up.
const REPO_ROOT = path.join(process.cwd(), '..');

export function getLearnArduinoChapters() {
  const dir = path.join(REPO_ROOT, 'Learn_Arduino');
  if (!fs.existsSync(dir)) return [];
  
  const files = fs.readdirSync(dir);
  const chapters = files.filter(f => f.endsWith('.md') && f !== 'README.md');
  
  const data = chapters.map(fileName => {
    const filePath = path.join(dir, fileName);
    const content = fs.readFileSync(filePath, 'utf8');
    const { data: frontmatter, content: mdContent } = matter(content);
    
    // Fallback to parsing the first H1 if no frontmatter title exists
    const match = mdContent.match(/^#\s+(.*)/m);
    const title = match ? match[1] : fileName.replace('.md', '');
    
    return {
      slug: fileName.replace('.md', ''),
      title,
      content: mdContent
    };
  });
  
  // Sort chapters alphabetically (Chapter_01, Chapter_02, etc.)
  return data.sort((a, b) => a.slug.localeCompare(b.slug));
}

export function getLearnArduinoChapter(slug) {
  const chapters = getLearnArduinoChapters();
  return chapters.find(c => c.slug === slug);
}

export function get100DaysLogs() {
  if (!fs.existsSync(REPO_ROOT)) return [];
  
  const dirs = fs.readdirSync(REPO_ROOT, { withFileTypes: true })
    .filter(dirent => dirent.isDirectory() && dirent.name.startsWith('Day_'))
    .map(dirent => dirent.name);
    
  const data = dirs.map(dirName => {
    const readmePath = path.join(REPO_ROOT, dirName, 'README.md');
    let mdContent = '';
    let title = dirName.replace(/_/g, ' ');
    let wokwiId = null;
    
    if (fs.existsSync(readmePath)) {
      const content = fs.readFileSync(readmePath, 'utf8');
      const parsed = matter(content);
      mdContent = parsed.content;
      if (parsed.data && parsed.data.wokwi_id) {
        wokwiId = parsed.data.wokwi_id;
      }
      const match = mdContent.match(/^#\s+(.*)/m);
      if (match) title = match[1];
    }
    
    // Extract day number for correct numeric sorting
    const dayMatch = dirName.match(/Day_(\d+)/);
    const dayNumber = dayMatch ? parseInt(dayMatch[1], 10) : 0;
    
    return {
      slug: dirName,
      title,
      content: mdContent,
      dayNumber,
      wokwi_id: wokwiId
    };
  });
  
  return data.sort((a, b) => a.dayNumber - b.dayNumber);
}

export function getDayLog(slug) {
  const days = get100DaysLogs();
  return days.find(d => d.slug === slug);
}
