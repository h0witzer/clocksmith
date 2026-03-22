# Guide 3 — The GitHub CLI (`gh`)

**Who this is for:** Someone who creates and manages repositories through the GitHub web interface and wants to do the same things faster from the terminal.

**What you will learn:** Install and authenticate `gh`; create repositories; clone; open pull requests; and the specific `gh` commands used throughout the clocksmith workflow.

---

## What is the GitHub CLI?

`gh` is GitHub's official command-line tool. Instead of opening a browser to create a repo, file an issue, or review a pull request, you type a command in your terminal. The results are the same — `gh` talks to the same GitHub API — but you stay in your editor context and can chain commands together in scripts.

---

## Step 1 — Install `gh`

### macOS

```sh
brew install gh
```

If you don't have Homebrew: [https://brew.sh](https://brew.sh)

### Windows

Using winget (built into Windows 10/11):
```sh
winget install --id GitHub.cli
```

Or download the installer from [https://cli.github.com](https://cli.github.com).

### Linux (Debian / Ubuntu)

```sh
(type -p wget >/dev/null || (sudo apt update && sudo apt install wget -y)) \
  && sudo mkdir -p -m 755 /etc/apt/keyrings \
  && out=$(mktemp) && wget -nv -O$out https://cli.github.com/packages/githubcli-archive-keyring.gpg \
  && cat $out | sudo tee /etc/apt/keyrings/githubcli-archive-keyring.gpg > /dev/null \
  && sudo chmod go+r /etc/apt/keyrings/githubcli-archive-keyring.gpg \
  && echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" | sudo tee /etc/apt/sources.list.d/github-cli.list > /dev/null \
  && sudo apt update \
  && sudo apt install gh -y
```

### Verify the installation

```sh
gh --version
# expected: gh version X.Y.Z (YYYY-MM-DD)
```

---

## Step 2 — Authenticate

```sh
gh auth login
```

`gh` will ask you a series of questions:

1. **Where do you use GitHub?** → `GitHub.com`
2. **What is your preferred protocol?** → `HTTPS` (simpler) or `SSH` (if you already have an SSH key set up)
3. **How would you like to authenticate?** → `Login with a web browser`

Follow the browser prompt to authorise the CLI. When it succeeds, your terminal will show:

```
✓ Logged in as YOUR_USERNAME
```

### Verify

```sh
gh auth status
# expected: Logged in to github.com as YOUR_USERNAME (...)
```

---

## Step 3 — Essential commands

### Create a new repository

```sh
# Create a public repo and clone it locally in one step:
gh repo create my-wall-clock \
  --public \
  --description "Kinetic wall clock using clocksmith" \
  --clone

cd my-wall-clock
```

Flags you will commonly use:

| Flag | Meaning |
|---|---|
| `--public` | Visible to everyone |
| `--private` | Only you and collaborators |
| `--description "..."` | Sets the repo's About description on GitHub |
| `--clone` | Clones the new repo to a local folder immediately |

### Clone an existing repository

```sh
gh repo clone h0witzer/clocksmith
# equivalent to: git clone https://github.com/h0witzer/clocksmith.git
```

### View your repositories

```sh
gh repo list
# shows your repos with visibility and description

gh repo list --limit 50
# show up to 50 repos
```

### Open the current repo in your browser

```sh
gh repo view --web
```

Useful when you want to quickly check the GitHub UI without leaving the terminal.

---

## Step 4 — Working with branches and pull requests

### Create a branch and a pull request

```sh
# standard git branch workflow:
git checkout -b feature/add-second-hand

# make your changes, commit them:
git add lib/drivers/ULN2003StepperMotor.hpp
git commit -m "Add ULN2003 stepper motor driver"

# push and open a PR in one command:
gh pr create \
  --title "Add ULN2003 stepper motor driver" \
  --body "Implements IMotor for the 28BYJ-48 stepper with ULN2003 board using AccelStepper." \
  --base main
```

### List open pull requests

```sh
gh pr list
```

### View a pull request in the browser

```sh
gh pr view --web
```

### Check the status of your PR's CI checks

```sh
gh pr checks
```

### Merge a pull request

```sh
gh pr merge --squash
# or --merge (merge commit) or --rebase
```

---

## Step 5 — The commands used in the clocksmith workflow

These are the specific commands referenced throughout the clocksmith documentation, collected here for quick reference.

### Setting up a new sculpture project

```sh
# 1. Create the repo and clone it
gh repo create my-wall-clock --public --clone
cd my-wall-clock

# 2. Scaffold PlatformIO project (needs pio installed — see Guide 1)
pio project init --board uno --ide vscode

# 3. Open in VS Code
code .
```

### Updating the clocksmith framework from a fork

```sh
# Fork clocksmith (only needed once)
gh repo fork h0witzer/clocksmith --clone
cd clocksmith

# Create a branch for your change
git checkout -b feature/add-ibuzzer-interface

# ... make changes ...

# Push and open a PR back to the upstream repo
gh pr create \
  --repo h0witzer/clocksmith \
  --title "Add IBuzzer interface for chiming clocks" \
  --body "Adds IBuzzer to allow ClockLogic to trigger audio feedback on the hour."
```

### Checking Copilot agent session logs (advanced)

If you are using GitHub Copilot coding agents via the web interface and want to inspect what they committed:

```sh
# View the latest commits on the current branch
gh api repos/:owner/:repo/commits?sha=BRANCH_NAME --jq '.[].commit.message'

# Or just use git:
git log --oneline -10
```

---

## Common mistakes

### "gh: command not found" after installing on macOS

Your shell config file may not have been reloaded. Run:

```sh
source ~/.zshrc   # or ~/.bashrc
```

### "You are not logged in to any GitHub hosts"

Run `gh auth login` again. Your authentication token may have expired.

### "Repository not found" when cloning a private repo

Make sure you authenticated with `gh auth login` and that your GitHub account has access to the repository.

---

## Going further

The `gh` CLI has subcommands for almost everything on GitHub:

```sh
gh issue list            # list open issues
gh issue create          # file a new issue
gh release list          # list releases
gh release create v1.0.0 # tag and create a release
gh workflow run          # trigger a GitHub Actions workflow manually
gh ssh-key add           # add your SSH public key to GitHub
```

Run `gh help` or `gh <command> --help` for full documentation on any command.

---

## What to read next

- [docs/using-copilot-to-scaffold-a-project.md](../docs/using-copilot-to-scaffold-a-project.md) — applies `gh`, `pio`, and Copilot together to create a full sculpture project and work across both repos simultaneously
- [docs/using-as-a-library.md](../docs/using-as-a-library.md) — the clocksmith library dependency model
