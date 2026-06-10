# Contributing

Thanks for helping improve 100 Days of Arduino. The best contributions make the guide clearer, safer, easier to build, or easier to maintain.

## Good Contributions

- Fix wiring mistakes, pin conflicts, library issues, or unsafe power guidance.
- Improve explanations without changing the learning objective of a day.
- Add photos, diagrams, oscilloscope captures, serial output examples, or troubleshooting notes.
- Improve code readability while keeping examples approachable.
- Add board-specific notes for Uno, Nano, Mega, Leonardo, ESP32, or compatible boards.
- Improve the docs website, search, navigation, accessibility, or static build quality.

## Project Folder Format

Each day should keep this structure:

```text
Day_XX_Project_Name/
|-- Day_XX_Project_Name.ino
`-- README.md
```

The README should cover:

- What the project builds
- Components required
- Pin-to-pin wiring
- How the circuit works
- How the code works
- Upload and test steps
- Troubleshooting notes
- Safe power or voltage notes where relevant

## Code Style

- Prefer clear constants for pin numbers and timing values.
- Prefer non-blocking patterns with `millis()` when timing matters.
- Avoid `String` on memory-limited AVR boards unless there is a strong reason.
- Keep interrupt service routines short.
- Avoid hidden hardware assumptions; document board-specific pins.
- Add comments only where the concept is not obvious to a learner.

## Documentation Style

- Write for a learner who is careful but new.
- Use exact component names and pin labels.
- Mention expected Serial Monitor output when useful.
- Prefer short troubleshooting bullets over vague advice.
- Include safety notes for motors, relays, high current, batteries, and mixed voltages.

## Before Opening a Pull Request

Run the website checks if you changed the docs site:

```bash
cd docs-website
npm run lint
npm run build
```

If you changed Arduino sketches, compile the affected sketch in the Arduino IDE or Arduino CLI and mention the board target you used.

## Commit Messages

Use focused commit messages such as:

- `Fix Day 07 ultrasonic timeout handling`
- `Clarify relay wiring safety notes`
- `Improve docs website search reset`
- `Add ESP32 notes for FreeRTOS examples`
