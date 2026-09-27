---
name: review-user-changes
description: Read and integrate user's manual code edits. Invoked when the user says they modified code after Claude wrote it. Understands intent even from pseudo-code or non-compiling drafts.
user-invocable: false
---

# Review User Changes

The user has manually edited code that was previously written by Claude. Their edits express an intent or a simplification — follow it through, but validate correctness first.

## Steps

1. **Read the simplification patterns log first.**
   Read `.claude/code-simplification-patterns.md`. Internalize any recorded patterns before proceeding.

2. **Diff the modified files.**
   Run `!git diff` (and `!git diff --cached` if anything is staged) to see exactly what changed.
   If the system-reminder already shows the diff inline, use that directly.

3. **Understand the intent.**
   The user's code may be pseudo-code, may not compile, or may be incomplete. Focus on *what they are trying to express*, not whether it compiles as-is.

   Ask yourself:
   - What design decision does this change reflect?
   - Does it simplify something that was previously more complex?
   - Does it change a data structure, control flow, or API surface?

4. **Validate the approach.**
   Before writing a single line of code, assess whether the user's direction is technically sound:
   - Does it introduce a logical error, race condition, memory safety issue, or API misuse?
   - Does it conflict with invariants that the rest of the codebase relies on?
   - Is there a subtle flaw that makes the approach unworkable as stated?

   **If you spot a problem:**
   - Do NOT silently "fix" it by deviating from their direction.
   - Report the issue clearly: what the problem is, why it matters, and (if you have one) a suggested correction. Wait for the user to confirm before proceeding.

5. **Decide: understandable or not?**

   **If you can understand the intent AND the approach is sound:**
   - Complete the implementation following the user's approach.
   - Fix compile errors introduced by their draft, but stay true to their direction.
   - Do NOT add new classes, functions, or abstractions unless the user explicitly asked for them, or unless the missing piece is a trivially named helper that any reader would expect.
   - If something is ambiguous but small, make the most conservative choice and note it briefly.

   **If you cannot understand the intent, OR completing it requires inventing significant new design:**
   - Do NOT modify any code.
   - Report to the user: state what you understood, what is unclear, and what new design you would need to add. Ask for confirmation before proceeding.

6. **Build and test.**
   After completing the implementation, build and run the relevant tests.

   **Determine which tests to run:**
   - First check conversation context: if a specific test target was mentioned or used recently, use that.
   - If context is unclear, apply these defaults:
     - Changes under `bot_core/`, `bot_framework/`, or other non-game modules → build and run `test_bot`
     - Changes under `games/<module>/` → build and run `test_<module>` and `run_<module>` (if they exist)
   - Build command: `cmake --build /Users/liuchang/Repository/lgtbot/src_1/build --target <target> -j$(sysctl -n hw.ncpu)`
   - Run the test binary from the build directory.

   If build or tests fail, diagnose and fix before reporting success.

7. **Check for simplification.**
   If the user's edit made existing code noticeably simpler (fewer lines, fewer indirections, cleaner pattern), record the insight in `.claude/code-simplification-patterns.md` following this format:

   ```
   ## <short title>

   **Before:** <what the old approach was>
   **After:** <what the simpler approach is>
   **Lesson:** <generalizable rule for future code>
   ```

   Only add entries that are genuinely generalizable — skip one-off fixes.

8. **Report what you did.**
   One or two sentences: what the user's change expressed, what you completed, whether build and tests passed, and (if any) what you recorded in the patterns log.
