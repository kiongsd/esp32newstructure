---
name: vision-dev-workflow
description: "Automatically validate requested project changes and create a safe Conventional Commit checkpoint. Use for source-code, test, configuration, or build changes in this repository, especially C# vision projects using VisionMaster, HALCON, or OpenCV; do not use for read-only analysis or when the user explicitly forbids committing."
---

# Vision development workflow

Use this skill for a requested change to the repository. Its outcome is a reviewable, validated Git checkpoint for the completed change, with the commit hash and validation results reported to the user.

## Scope and safety

- This skill runs only after an actual repository change is requested. It does not watch the filesystem and cannot react to edits made outside the current Codex task.
- If the user explicitly says not to commit, skip the checkpoint and report the uncommitted status.
- Never commit secrets, credentials, local settings, generated binaries, large runtime output, or unrelated pre-existing changes. Respect `.gitignore` and repository instructions.
- Do not use `git add .` or `git add -A` blindly. Stage an explicit, reviewed path set.
- Do not create an empty commit. If the requested work produces no diff, report that no checkpoint was needed.
- Do not create checkpoint tags by default. Create one only when the user requests tags; use the next unused `checkpoint-NNN` number and report it.

## Workflow

### 1. Capture the starting state

Before editing, establish the repository root and record the starting state:

```text
git rev-parse --show-toplevel
git status --short
git diff --stat
git diff --cached --stat
```

Treat paths already modified, staged, or untracked at task start as pre-existing. If the user explicitly asks to checkpoint existing work, the requested current diff is the intended scope; otherwise preserve those paths and do not include them in this task's commit.

Read applicable `AGENTS.md` files and project instructions before changing files. Identify the smallest relevant build and test commands from the repository. For a .NET project, prefer the relevant solution or project with `dotnet build` and `dotnet test`; do not invent a command when the repository documents another one. If VisionMaster or HALCON can only be compiled through an installed IDE or vendor environment, state that the check was unavailable instead of pretending it passed.

### 2. Implement and validate

Make the requested change, then run the narrowest meaningful validation:

- Run `git diff --check` to catch whitespace errors.
- Run the relevant build and tests. For C# changes, normally use the affected `.sln`/`.slnx` or `.csproj` and its test projects.
- If validation fails, determine whether the failure is caused by the change. Fix it when practical. Do not call incomplete or knowingly broken work “complete”.
- If a failure is environmental or pre-existing and the requested change is otherwise complete, a checkpoint may still be created, but the failure must be recorded in the final report.

Review both the working diff and the intended file list. Pay special attention to vendor-generated files, user-local configuration, sample images/video, build output, and credentials in vision projects.

### 3. Prepare a scoped checkpoint

After the change is complete, compute the files changed by this task. For a clean starting file, stage it explicitly:

```text
git add -- path/to/file path/to/another-file
git diff --cached --check
git diff --cached
```

When a file was already dirty at task start, inspect its diff and stage only the current task's hunks with `git add -p` or an equivalent patch-based method. If the current and pre-existing edits cannot be separated safely, stop before committing and ask the user how to scope that file. Leave unrelated pre-existing changes untouched.

Never stage a secret or generated artifact merely because it appears in `git status`. If the user asks to add an ignored file, confirm that it is intentional and use an explicit path.

### 4. Create the commit

Generate a concise Conventional Commit subject from the reviewed staged diff:

```text
<type>(<optional-scope>): <imperative summary>
```

Use one of `feat`, `fix`, `refactor`, `perf`, `docs`, `test`, `build`, `ci`, or `chore`. Keep the subject focused and reasonably short. Add a body only when it explains an important design or validation detail. Then commit the staged change without asking for confirmation unless the user explicitly forbade automatic commits.

If `git commit` fails because Git identity, hooks, signing, or permissions are not configured, do not bypass the failure or change global configuration. Report the exact failure and leave the staged diff intact for the user.

### 5. Verify and report

After a successful commit, run:

```text
git log -1 --oneline
git status --short
```

Report:

1. Commit hash and subject.
2. Files included in the checkpoint.
3. Build/test commands and results, including unavailable checks.
4. Any remaining unrelated pre-existing changes or risks.

If tags were requested, create and verify the requested `checkpoint-NNN` tag after the commit, then include it in the report.
