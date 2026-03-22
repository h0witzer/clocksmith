# Guide 1 — VS Code and GitHub Copilot Setup

**Who this is for:** Someone who has used GitHub Copilot through the web interface (github.com/copilot) but has never set it up in VS Code.

**What you will end up with:** VS Code configured for Arduino / PlatformIO development with GitHub Copilot active and working.

---

## Step 1 — Install VS Code

Download and install VS Code from [https://code.visualstudio.com](https://code.visualstudio.com).

Choose the installer for your operating system (Windows, macOS, or Linux). The default install options are fine.

---

## Step 2 — Install the essential extensions

Extensions are add-ons that give VS Code new capabilities. You need four of them.

Open VS Code. Press `Ctrl+Shift+X` (Windows/Linux) or `Cmd+Shift+X` (macOS) to open the Extensions panel. Search for each name below and click **Install**.

### 1. PlatformIO IDE

> Search name: **PlatformIO IDE**  
> Publisher: PlatformIO

This replaces the Arduino IDE for professional embedded development. It manages board support packages, libraries, and the build toolchain automatically. It also gives you a proper project structure with `src/`, `lib/`, and `platformio.ini`.

After installing, VS Code will prompt you to reload the window. Do that before continuing.

### 2. C/C++

> Search name: **C/C++**  
> Publisher: Microsoft

Provides IntelliSense (code completion, go-to-definition, error highlighting) for C and C++ code. Copilot uses IntelliSense data to understand your code better — install this before Copilot.

### 3. GitHub Copilot

> Search name: **GitHub Copilot**  
> Publisher: GitHub

The core extension. Provides inline "ghost text" completions as you type — press `Tab` to accept, `Esc` to dismiss.

### 4. GitHub Copilot Chat

> Search name: **GitHub Copilot Chat**  
> Publisher: GitHub

Adds the Chat panel where you can have a full conversation with Copilot, ask questions about your code, and issue multi-file editing instructions. This is the most powerful way to use Copilot.

> **Note:** As of late 2024, GitHub Copilot and GitHub Copilot Chat are bundled together in a single extension. If you see one combined listing, install that — you get both.

---

## Step 3 — Sign in to GitHub

After installing the Copilot extension, VS Code will show a banner or a notification asking you to sign in. Click it. Alternatively:

1. Open the Command Palette: `Ctrl+Shift+P` / `Cmd+Shift+P`
2. Type `Copilot: Sign in` and press Enter
3. VS Code will open a browser window — log in to GitHub and click **Authorize**

Your GitHub account needs an active Copilot subscription (or you can use the free tier if your account qualifies — check [github.com/settings/copilot](https://github.com/settings/copilot)).

---

## Step 4 — Verify Copilot is working

### Check the status bar

At the bottom-right corner of the VS Code window you should see a small Copilot icon (looks like the GitHub Copilot logo). If it is lit up (not greyed out), Copilot is active.

### Test inline completions

1. Open (or create) any `.cpp` or `.ino` file
2. Type `void setup() {` and press Enter
3. You should see grey "ghost text" suggesting a completion
4. Press `Tab` to accept it, or keep typing to dismiss it

### Test Copilot Chat

1. Press `Ctrl+Shift+I` (Windows/Linux) or `Cmd+Shift+I` (macOS)  
   — or click the chat icon in the left sidebar
2. Type: `What is the IMotor interface used for in this project?`
3. Copilot should reply with a relevant answer based on the files in your workspace

If the chat panel does not open, make sure the GitHub Copilot Chat extension is installed and you are signed in.

---

## Step 5 — Install the PlatformIO command-line tools

The PlatformIO IDE extension installs `pio` (the PlatformIO CLI) automatically. To use it from your terminal outside VS Code:

### macOS / Linux

Add the PlatformIO scripts directory to your PATH. Add this line to your `~/.bashrc`, `~/.zshrc`, or equivalent shell config file:

```sh
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

Then reload your shell:

```sh
source ~/.zshrc   # or source ~/.bashrc
```

### Windows

The PlatformIO IDE extension adds `pio` to your PATH automatically during installation. Open a new Command Prompt or PowerShell window and run `pio --version` to confirm.

### Verify

```sh
pio --version
# expected output: PlatformIO Core, version X.Y.Z
```

---

## Step 6 — Open a clocksmith project in VS Code

Clone the clocksmith repository (or your sculpture project) and open it:

```sh
git clone https://github.com/h0witzer/clocksmith.git
code clocksmith
```

VS Code will detect the `platformio.ini` file and the PlatformIO IDE extension will activate automatically. The first time you open a PlatformIO project it downloads the board support packages — this may take a few minutes.

---

## Troubleshooting

| Symptom | Fix |
|---|---|
| Copilot icon is greyed out | Click the icon → sign in to GitHub |
| No inline completions appear | Check that Copilot is enabled for C++ files: `Ctrl+Shift+P` → `Copilot: Enable Completions` |
| `pio` command not found | Reload the terminal after adding to PATH; on Windows open a new terminal window |
| PlatformIO doesn't recognise the board | Run `pio pkg install` from the project root |

---

## What to read next

- [02-copilot-in-vscode.md](02-copilot-in-vscode.md) — how to use Copilot Chat, `@workspace`, and effective prompting techniques
- [03-github-cli.md](03-github-cli.md) — install the `gh` CLI to create and manage repositories from the terminal
