#!/usr/bin/env node

const inquirer = require('inquirer');
const chalk = require('chalk');
const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');

const REPO_ROOT = path.join(__dirname, '..');

// Helpers
function getExistingDays() {
  return fs.readdirSync(REPO_ROOT, { withFileTypes: true })
    .filter(dirent => dirent.isDirectory() && dirent.name.startsWith('Day_'))
    .map(dirent => dirent.name);
}

function runCommand(command, args, cwd) {
  return new Promise((resolve, reject) => {
    const child = spawn(command, args, { cwd, stdio: 'inherit', shell: true });
    child.on('close', code => {
      if (code === 0) resolve();
      else reject(new Error(`Command failed with code ${code}`));
    });
  });
}

// Actions
async function scaffoldDay() {
  const existing = getExistingDays();
  const nextDayNumber = existing.length + 1;

  console.log(chalk.blue(`\nCurrent projects found: ${existing.length}`));
  
  const answers = await inquirer.prompt([
    {
      type: 'input',
      name: 'dayNum',
      message: `Enter the Day number (default: ${nextDayNumber}):`,
      default: nextDayNumber,
      validate: val => !isNaN(parseInt(val)) || 'Please enter a valid number'
    },
    {
      type: 'input',
      name: 'title',
      message: 'Enter the Project Title (e.g. "Laser Turret"):',
      validate: val => val.trim().length > 0 || 'Title is required'
    },
    {
      type: 'input',
      name: 'wokwiId',
      message: 'Enter Wokwi Project ID (Optional, leave blank if none):'
    }
  ]);

  const dayNum = parseInt(answers.dayNum);
  const dayPadded = String(dayNum).padStart(2, '0');
  const safeTitle = answers.title.replace(/[^a-zA-Z0-9]/g, '_').replace(/_+/g, '_');
  const folderName = `Day_${dayPadded}_${safeTitle}`;
  const folderPath = path.join(REPO_ROOT, folderName);

  if (fs.existsSync(folderPath)) {
    console.log(chalk.red(`\nError: Directory ${folderName} already exists!`));
    return;
  }

  console.log(chalk.green(`\nCreating ${folderName}...`));
  fs.mkdirSync(folderPath);

  const inoContent = `/*
 * Day ${dayNum}: ${answers.title}
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

  const frontmatter = answers.wokwiId.trim() ? `---\nwokwi_id: "${answers.wokwiId.trim()}"\n---\n\n` : '';

  const readmeContent = `${frontmatter}# Day ${dayNum}: ${answers.title}

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
  console.log(chalk.green('✅ Project scaffolded successfully!'));
  console.log(chalk.cyan(`Open /${folderName}/${folderName}.ino to start coding!`));
}

async function generateEbook() {
  console.log(chalk.yellow('\nGenerating E-Book...'));
  try {
    await runCommand('node', ['scripts/generate-ebook.js'], REPO_ROOT);
    console.log(chalk.green('\n✅ E-book generated!'));
  } catch (err) {
    console.log(chalk.red('\nFailed to generate E-book.'));
  }
}

async function compileAll() {
  console.log(chalk.yellow('\nCompiling all Arduino projects...'));
  try {
    await runCommand('powershell', ['-File', 'compile.ps1'], REPO_ROOT);
    console.log(chalk.green('\n✅ Compilation check complete!'));
  } catch (err) {
    console.log(chalk.red('\nCompilation failed.'));
  }
}

async function main() {
  console.clear();
  console.log(chalk.bgBlue.white.bold('\n 🚀 100 Days of Arduino - Ultimate CLI 🚀 \n'));

  while (true) {
    const { action } = await inquirer.prompt([
      {
        type: 'list',
        name: 'action',
        message: 'What would you like to do?',
        choices: [
          { name: '🏗️  Scaffold a new Day Project', value: 'scaffold' },
          { name: '📚 Generate the Markdown E-Book', value: 'ebook' },
          { name: '🛠️  Compile all Arduino files', value: 'compile' },
          new inquirer.Separator(),
          { name: '🚪 Exit', value: 'exit' }
        ]
      }
    ]);

    if (action === 'scaffold') {
      await scaffoldDay();
    } else if (action === 'ebook') {
      await generateEbook();
    } else if (action === 'compile') {
      await compileAll();
    } else {
      console.log(chalk.cyan('\nHappy building! 👋\n'));
      break;
    }
    console.log('\n');
  }
}

main().catch(err => {
  console.error(chalk.red('Fatal error:'), err);
  process.exit(1);
});
