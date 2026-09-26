# Commit Message Rules

## Title (first line)

If a commit modifies a specific game, the title must start with `<module>: ` where `<module>` matches the `games/<module>/` directory name **exactly** (case-sensitive). Examples: `opencomb: fix bug` for `games/opencomb/`; `OpenComb: fix bug` only if the directory is literally `games/OpenComb/`.

For all other commits (bot_core, CI, game_framework, game_util, etc.), do **not** use a `<prefix>: ` format in the title.

The title must:

- begin with a **lowercase ASCII letter** (`a`–`z`) for non-game commits. Examples: `add grpc server`. Avoid `Add grpc server` or `Fix login`. Game-only commits follow the module name casing; if `games/<module>/` starts with an uppercase letter, the title may too (e.g., `OpenComb: fix scoring` for `games/OpenComb/`).
- **not** end with a period (`.`).

## Line length

- **Title:** at most **50 characters** (recommended). Do not exceed **72 characters**.
- **Body:** wrap each line at **72 characters** or fewer. Separate the title from the body with one blank line.

## Body (optional)

Use the body to explain *why* the change is needed when the title alone is not enough. Still respect the 72-character line wrap.

**Paragraphs** (plain lines, not bullets): each line is a complete sentence — start with an **uppercase** ASCII letter (`A`–`Z`) and end with a **period** (`.`).

**Bullet lists** (lines starting with `- `): within one list (consecutive `-` lines, separated from other blocks by a blank line), every bullet must use the **same** style:

- **phrase style:** lowercase start, **no** trailing period (e.g., `- validate title casing`).
- **sentence style:** uppercase start, **with** trailing period (e.g., `- Validate title casing.`).

Do not mix phrase and sentence styles in the same bullet list. Git trailer lines (e.g., `Signed-off-by: …`) are exempt.

## Git hook

Run `./scripts/setup-git-hooks.sh` once per clone to install the `commit-msg` hook (`core.hooksPath=githooks`). It enforces the rules above on every commit.
