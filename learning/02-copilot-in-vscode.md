# Guide 2 — Using GitHub Copilot in VS Code

**Who this is for:** Someone who has Copilot installed and signed in (see [01-vscode-and-copilot-setup.md](01-vscode-and-copilot-setup.md)) but is used to the web interface and hasn't used the in-editor experience yet.

**What you will learn:** The three ways Copilot works in VS Code, how to give it the right context, and how to write prompts that get useful results the first time.

---

## The three modes of Copilot in VS Code

### 1. Inline completions ("ghost text")

As you type, Copilot predicts what comes next and shows it in grey. Press `Tab` to accept. Press `Esc` to dismiss. Press `Alt+]` / `Option+]` to cycle through alternative suggestions.

This is useful for filling in routine code once you have started writing the structure yourself. It works best when:
- You have already written the class name and method signature
- Your file contains some relevant context (other methods, `#include`s)
- The file name or class name hints at the purpose (e.g. `ULN2003StepperMotor.hpp`)

### 2. Copilot Chat (the conversation panel)

Open with `Ctrl+Shift+I` (Windows/Linux) or `Cmd+Shift+I` (macOS).

This is the equivalent of the web interface you are already used to, but it runs inside VS Code and has access to your open files and workspace. You can have a back-and-forth conversation, ask Copilot to explain code, generate entire files, or plan out a feature before writing it.

### 3. Copilot Edits (multi-file changes)

Open with `Ctrl+Shift+I` and switch to the **Edits** tab, or press `Ctrl+Shift+Alt+I`.

Copilot Edits lets you describe a change that spans multiple files in one instruction. Copilot opens a diff view for each affected file and you accept or reject each change individually. This is the right tool for:
- Adding a new motor driver AND updating `main.cpp` to register it
- Adding a new slot to clocksmith's `HardwareRegistry.hpp` AND using it in your sculpture project

---

## Giving Copilot context

The biggest difference between the web interface and VS Code Copilot is **context**. In VS Code, you control exactly what Copilot can see.

### @workspace

```
@workspace What files would I need to change to add a second-hand motor?
```

`@workspace` tells Copilot to index every file in your open workspace (all folders if you are using a multi-root workspace). Use this for architectural questions or cross-file changes. It is the most powerful context but also the slowest to respond — give it a few seconds.

### @file (ask about a specific file)

```
@file /src/main.cpp What is this file responsible for?
```

Scopes the answer to a single file. Useful when you want to understand one piece of the codebase without noise from unrelated files.

### #file (attach a file to your message)

```
Generate a new motor driver that follows the same pattern as #file:lib/drivers/ULN2003StepperMotor.hpp
```

Attaches a specific file to your message as a reference. Type `#file` and VS Code will show a picker. This is the most precise way to say "do it like this file".

### The active editor

If you have a file open and selected, Copilot Chat automatically has access to that file's content even without an explicit `@file`. When you ask "explain this function" with a function highlighted, Copilot uses the selection.

### .github/copilot-instructions.md

When you open a repository in VS Code, Copilot automatically reads `.github/copilot-instructions.md` if it exists. This file is how repository owners pre-load Copilot with project-specific rules so you don't have to repeat them in every prompt.

The clocksmith repository has one of these files. It means Copilot already knows:
- The `IMotor`, `IDisplay`, and `IClockCore` contracts
- The no-`delay()` rule
- That `main.cpp` is the only hardware seam
- The normalised position convention

When working in your sculpture project, create your own `.github/copilot-instructions.md` describing your specific hardware. See [docs/using-copilot-to-scaffold-a-project.md](../docs/using-copilot-to-scaffold-a-project.md) for a template.

---

## Slash commands in Copilot Chat

Type `/` in the chat input to see the available slash commands:

| Command | What it does |
|---|---|
| `/explain` | Explains the selected code or a named symbol |
| `/fix` | Identifies and fixes a bug in the selected code |
| `/tests` | Generates unit tests for the selected function or class |
| `/doc` | Writes a documentation comment for the selected code |
| `/new` | Scaffolds a new file or project based on your description |

Example:

```
/explain the setTarget method in IMotor.hpp
```

```
/fix the update() method in ULN2003StepperMotor.hpp — it calls delay() which is not allowed
```

---

## Effective prompting — practical rules

These habits make a measurable difference in the quality of Copilot's output.

### Be specific about inputs, outputs, and constraints

Poor prompt:
```
Write a motor driver
```

Better prompt:
```
@workspace Write a motor driver for a 28BYJ-48 stepper motor driven by a ULN2003 board.
Use AccelStepper (waspinator/AccelStepper). Constructor takes totalSteps and four pin numbers.
Follow the IMotor interface in clocksmith/include/IMotor.hpp.
No delay() anywhere. Clamp negative positions to 0.0 but allow values above 1.0.
Save to lib/drivers/ULN2003StepperMotor.hpp.
```

### Name the files you want read and the files you want written

```
Read clocksmith/hal/motors/StepperMotorStub.hpp as a template.
Generate my-wall-clock/lib/drivers/ServoMotor.hpp following the same structure.
```

### Ask for reasoning before code on complex decisions

```
@workspace Before writing any code: should a buzzer that chimes on the hour be 
modelled as a new IDisplay, a new interface IBuzzer, or driven directly from 
main.cpp without a new interface? Explain the tradeoffs.
```

Once Copilot answers, you can say "go with option 2" and it will write the code using the approach it just reasoned about.

### Break large tasks into steps

Instead of asking for an entire working clock in one prompt, ask for one driver at a time, verify each one, then ask for `main.cpp` once all drivers exist.

### Iterate with follow-up messages

If the first result is almost right, don't start over. Say:

```
Good, but the isAtTarget() method should delegate to AccelStepper::distanceToGo() 
instead of comparing _currentStep to _targetStep directly.
```

Copilot tracks the conversation context and will revise only the part you flagged.

---

## A complete example session

Here is a realistic Copilot Chat session for adding a new motor driver to a sculpture project, from scratch.

**Opening message:**
```
@workspace I'm starting a new sculpture project at my-wall-clock/. I need a motor 
driver for a 28BYJ-48 stepper with a ULN2003 board. AccelStepper library. 
Pins 8, 9, 10, 11. 2048 steps per revolution.

Read clocksmith/include/IMotor.hpp for the interface and 
clocksmith/hal/motors/StepperMotorStub.hpp for the pattern.
Write the driver to my-wall-clock/lib/drivers/ULN2003StepperMotor.hpp.
```

**Follow-up (if Copilot missed something):**
```
The constructor should store the AccelStepper instance as a private member, 
not create it on the heap with new.
```

**Verification step:**
```
/explain the setTarget method in the file you just wrote. Does it correctly 
handle positions greater than 1.0?
```

**Final step:**
```
@workspace Now update my-wall-clock/src/main.cpp to declare a static 
ULN2003StepperMotor for the hour hand, register it under 
Slots::Motor::HOUR_HAND, and call its update() in loop().
```

---

## What to read next

- [03-github-cli.md](03-github-cli.md) — install the `gh` CLI to create and manage repositories from the terminal
- [docs/using-copilot-to-scaffold-a-project.md](../docs/using-copilot-to-scaffold-a-project.md) — apply everything here to a complete clocksmith sculpture project
