# Learning Resources — AI-Accelerated Development Workflows

This folder is **not about clocksmith's code**. It is about the tools and workflows you use to *work on* clocksmith and sculpture projects with GitHub Copilot — from someone who has only ever used the GitHub web interface, all the way to using Copilot as a real coding partner across multiple repositories in VS Code.

Read these guides in order. Each one builds on the last.

---

## Guides in this folder

| # | File | What it covers |
|---|---|---|
| 1 | [01-vscode-and-copilot-setup.md](01-vscode-and-copilot-setup.md) | Install VS Code; install the PlatformIO, C/C++, and Copilot extensions; sign in to GitHub; verify everything works |
| 2 | [02-copilot-in-vscode.md](02-copilot-in-vscode.md) | Inline code completions; the Copilot Chat panel; giving Copilot context with `@workspace`, `@file`, and `#file`; prompting effectively; Copilot Edits for multi-file changes |
| 3 | [03-github-cli.md](03-github-cli.md) | Install and authenticate the `gh` CLI; create repos; clone; open pull requests; the specific commands used across the clocksmith workflow |

---

## Where to go next

Once you are comfortable with the tools here, these functional guides (in [`../docs/`](../docs/)) show how to apply them to a real clocksmith project:

- [docs/using-copilot-to-scaffold-a-project.md](../docs/using-copilot-to-scaffold-a-project.md) — use Copilot to create a sculpture project repo and work across both repos simultaneously
- [docs/using-as-a-library.md](../docs/using-as-a-library.md) — how clocksmith is consumed as a PlatformIO library dependency
- [docs/adding-a-motor-driver.md](../docs/adding-a-motor-driver.md) — step-by-step motor driver walkthrough

---

> **Note for future sessions:** If you ask GitHub Copilot to help with a clocksmith sculpture project, point it at the `docs/` guides above. This `learning/` folder is for human reading only — Copilot already has the architectural context it needs from `.github/copilot-instructions.md`.
