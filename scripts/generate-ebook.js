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
    .map(f => ({
      file: path.join(dir, f),
      dirName: 'Learn_Arduino'
    }));
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
  }).map(d => ({
    file: path.join(REPO_ROOT, d, 'README.md'),
    dirName: d,
    fullDir: path.join(REPO_ROOT, d)
  }));
}

function processMarkdown(content, dirName) {
  // Remove frontmatter
  let cleanContent = content.replace(/---[\s\S]*?---/g, '').trim();
  
  // Fix relative image paths (e.g., ![alt](image.png) -> ![alt](Day_XX/image.png))
  // Ignore external urls starting with http:// or https://
  cleanContent = cleanContent.replace(/!\[([^\]]*)\]\((?!http)([^)]+)\)/g, (match, alt, imgPath) => {
    // If the path is already absolute or relative to root, leave it alone
    if (imgPath.startsWith('/') || imgPath.startsWith(dirName)) return match;
    return `![${alt}](${dirName}/${imgPath})`;
  });

  return cleanContent;
}

function getSourceCodeBlocks(fullDir) {
  if (!fs.existsSync(fullDir)) return '';
  const files = fs.readdirSync(fullDir)
    .filter(f => f.endsWith('.ino') || f.endsWith('.cpp') || f.endsWith('.h'));
  
  if (files.length === 0) return '';
  
  let codeBlocks = `\n\n### Source Code\n\n`;
  files.forEach(file => {
    const code = fs.readFileSync(path.join(fullDir, file), 'utf8');
    const ext = path.extname(file).substring(1);
    const lang = ext === 'ino' ? 'cpp' : ext; // highlight.js uses cpp for ino
    codeBlocks += `**\`${file}\`**\n\n\`\`\`${lang}\n${code}\n\`\`\`\n\n`;
  });
  
  return codeBlocks;
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
  chapters.forEach(chapter => {
    const content = fs.readFileSync(chapter.file, 'utf8');
    const match = content.match(/^#\s+(.*)/m);
    const title = match ? match[1].trim() : path.basename(chapter.file, '.md');
    const anchor = title.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/(^-|-$)/g, '');
    toc += `- [${title}](#${anchor})\n`;
  });

  toc += `\n### Part 2: The 100 Days Projects\n`;
  days.forEach(day => {
    if (fs.existsSync(day.file)) {
      const content = fs.readFileSync(day.file, 'utf8');
      const match = content.match(/^#\s+(.*)/m);
      const title = match ? match[1].trim() : day.dirName;
      const anchor = title.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/(^-|-$)/g, '');
      toc += `- [${title}](#${anchor})\n`;
    }
  });
  
  bookContent += toc + `\n---\n\n`;

  console.log(`Found ${chapters.length} theory chapters.`);
  bookContent += `<div style="page-break-after: always;"></div>\n\n`;
  bookContent += `# Part 1: Arduino Theory & Foundations\n\n`;
  
  chapters.forEach(chapter => {
    const content = fs.readFileSync(chapter.file, 'utf8');
    const cleanContent = processMarkdown(content, chapter.dirName);
    bookContent += cleanContent + '\n\n<div style="page-break-after: always;"></div>\n\n---\n\n';
  });
  
  console.log(`Found ${days.length} project logs.`);
  bookContent += `# Part 2: The 100 Days Projects\n\n`;
  
  days.forEach(day => {
    if (fs.existsSync(day.file)) {
      const content = fs.readFileSync(day.file, 'utf8');
      let cleanContent = processMarkdown(content, day.dirName);
      
      // Auto-inject source code
      const codeBlocks = getSourceCodeBlocks(day.fullDir);
      cleanContent += codeBlocks;

      bookContent += cleanContent + '\n\n<div style="page-break-after: always;"></div>\n\n---\n\n';
    }
  });
  
  fs.writeFileSync(OUTPUT_FILE, bookContent);
  console.log(`✅ E-Book Markdown successfully generated at: ${OUTPUT_FILE}`);
  console.log('You can now convert this file to PDF using Pandoc or Markdown-to-PDF plugins!');
}

main().catch(console.error);
