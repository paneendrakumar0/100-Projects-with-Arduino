const fs = require('fs');
const path = require('path');

const REPO_ROOT = path.join(__dirname, '..');
const OUTPUT_FILE = path.join(REPO_ROOT, '100_Days_of_Arduino_Book.md');

function getLearnChapters() {
  const dir = path.join(REPO_ROOT, 'Learn_Arduino');
  if (!fs.existsSync(dir)) return [];
  return fs.readdirSync(dir)
    .filter(f => f.endsWith('.md') && f !== 'README.md')
    .sort((a, b) => a.localeCompare(b))
    .map(f => path.join(dir, f));
}

function getDayLogs() {
  const dirs = fs.readdirSync(REPO_ROOT, { withFileTypes: true })
    .filter(dirent => dirent.isDirectory() && dirent.name.startsWith('Day_'))
    .map(dirent => dirent.name);
  
  return dirs.sort((a, b) => {
    const aMatch = a.match(/Day_(\d+)/);
    const bMatch = b.match(/Day_(\d+)/);
    const aNum = aMatch ? parseInt(aMatch[1], 10) : 0;
    const bNum = bMatch ? parseInt(bMatch[1], 10) : 0;
    return aNum - bNum;
  }).map(d => path.join(REPO_ROOT, d, 'README.md'));
}

async function main() {
  console.log('📚 Generating E-Book Markdown...');
  let bookContent = `# 100 Days of Arduino Masterclass\n\n`;
  bookContent += `*Compiled dynamically from the master repository*\n\n`;

  const chapters = getLearnChapters();
  const days = getDayLogs();

  console.log('Generating Table of Contents...');
  let toc = `## Table of Contents\n\n`;
  
  toc += `### Part 1: Arduino Theory & Foundations\n`;
  chapters.forEach(file => {
    const content = fs.readFileSync(file, 'utf8');
    const match = content.match(/^#\s+(.*)/m);
    const title = match ? match[1].trim() : path.basename(file, '.md');
    const anchor = title.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/(^-|-$)/g, '');
    toc += `- [${title}](#${anchor})\n`;
  });

  toc += `\n### Part 2: The 100 Days Projects\n`;
  days.forEach(file => {
    if (fs.existsSync(file)) {
      const content = fs.readFileSync(file, 'utf8');
      const match = content.match(/^#\s+(.*)/m);
      const title = match ? match[1].trim() : path.basename(path.dirname(file));
      const anchor = title.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/(^-|-$)/g, '');
      toc += `- [${title}](#${anchor})\n`;
    }
  });
  
  bookContent += toc + `\n---\n\n`;

  console.log(`Found ${chapters.length} theory chapters.`);
  bookContent += `# Part 1: Arduino Theory & Foundations\n\n`;
  
  chapters.forEach(file => {
    const content = fs.readFileSync(file, 'utf8');
    // Remove frontmatter if present
    const cleanContent = content.replace(/---[\\s\\S]*?---/g, '').trim();
    bookContent += cleanContent + '\n\n---\n\n';
  });
  
  console.log(`Found ${days.length} project logs.`);
  bookContent += `# Part 2: The 100 Days Projects\n\n`;
  
  days.forEach(file => {
    if (fs.existsSync(file)) {
      const content = fs.readFileSync(file, 'utf8');
      const cleanContent = content.replace(/---[\\s\\S]*?---/g, '').trim();
      bookContent += cleanContent + '\n\n---\n\n';
    }
  });
  
  fs.writeFileSync(OUTPUT_FILE, bookContent);
  console.log(`✅ E-Book Markdown successfully generated at: ${OUTPUT_FILE}`);
  console.log('You can now convert this file to PDF using Pandoc or Markdown-to-PDF plugins!');
}

main().catch(console.error);
