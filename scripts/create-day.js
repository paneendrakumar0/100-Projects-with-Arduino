const fs = require('fs');
const path = require('path');
const readline = require('readline');

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout
});

const REPO_ROOT = path.join(__dirname, '..');

function getExistingDays() {
  const dirs = fs.readdirSync(REPO_ROOT, { withFileTypes: true })
    .filter(dirent => dirent.isDirectory() && dirent.name.startsWith('Day_'))
    .map(dirent => dirent.name);
  return dirs;
}

function prompt(query) {
  return new Promise(resolve => rl.question(query, resolve));
}

async function main() {
  console.log('\n🚀 Arduino 100 Days Project Generator 🚀\n');
  
  const existing = getExistingDays();
  const nextDayNumber = existing.length + 1;
  
  console.log(`Current projects found: ${existing.length}`);
  console.log(`Suggested next Day number: ${nextDayNumber}\n`);
  
  const dayStr = await prompt(`Enter the Day number (default: ${nextDayNumber}): `);
  const dayNum = parseInt(dayStr) || nextDayNumber;
  const dayPadded = String(dayNum).padStart(2, '0');
  
  const rawTitle = await prompt('Enter the Project Title (e.g. "Laser Turret"): ');
  if (!rawTitle) {
    console.error('Title is required!');
    process.exit(1);
  }
  
  const safeTitle = rawTitle.replace(/[^a-zA-Z0-9]/g, '_').replace(/_+/g, '_');
  const folderName = `Day_${dayPadded}_${safeTitle}`;
  const folderPath = path.join(REPO_ROOT, folderName);
  
  if (fs.existsSync(folderPath)) {
    console.error(`\nError: Directory ${folderName} already exists!`);
    process.exit(1);
  }
  
  console.log(`\nCreating ${folderName}...`);
  fs.mkdirSync(folderPath);
  
  const inoContent = `/*
 * Day ${dayNum}: ${rawTitle}
 * 100 Days of Arduino Masterclass
 */

void setup() {
  Serial.begin(115200);
  Serial.println("Day ${dayNum} starting...");
}

void loop() {
  // Your code here
}
`;

  fs.writeFileSync(path.join(folderPath, `${folderName}.ino`), inoContent);
  
  const readmeContent = `# Day ${dayNum}: ${rawTitle}

## The Concept


## Hardware Required


## Wiring Guide


## The Code

\`\`\`cpp
${inoContent.trim()}
\`\`\`

## What We Learned

`;

  fs.writeFileSync(path.join(folderPath, 'README.md'), readmeContent);
  
  console.log('\n✅ Project scaffolded successfully!');
  console.log(`Open /${folderName}/${folderName}.ino to start coding!`);
  
  rl.close();
}

main().catch(console.error);
