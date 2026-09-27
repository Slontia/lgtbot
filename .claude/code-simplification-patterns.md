# Code Simplification Patterns

Lessons learned from user edits that simplified Claude-written code.
Each entry is a generalizable rule, not a one-off fix.

<!-- New entries go below this line, most recent first. -->

## Return `std::variant<Success, ErrCode>` instead of `std::optional<Success>` with an out-param

**Before:** `std::optional<LobbyGameStartPlan> BeginGameStart(uid, reply, self, ErrCode& err_out)` — callers had to declare an `ErrCode err_out = EC_OK;` local before the call, then decide which of `optional` / `err_out` to inspect. The `self` parameter existed only so the callee could log via the Match, adding coupling.

**After:** `std::variant<LobbyGameStartPlan, ErrCode> BeginGameStart(uid, reply)`. The success path holds `plan_variant` directly; error path is `if (const ErrCode* err = std::get_if<ErrCode>(&plan_variant)) return *err;`. No pre-declared out-param, no coupling to `self` just for logging.

**Lesson:** When a function has exactly one success payload and can fail with an ErrCode, `variant<Success, ErrCode>` is a cleaner return than `optional<Success>` + `ErrCode&`. Every error path is total (no forgotten out-param write), and call sites read like a natural pattern-match.


**Before:** `ReplyRespToHandler(handler, resp)` and `MakeGameOverReply()` were free functions defined once and called once each.

**After:** Their bodies are inlined directly at the call site — one loop and one item construction.

**Lesson:** A function called exactly once is just an indirection. If the body fits in 2–3 lines and has no reuse potential, delete the function and write the code inline.

## Use function overloading to eliminate boilerplate adapter lambdas at call sites

**Before:** `MakeCallback_` was a single static member taking `std::function<Running&()>`. Every call site that had a concrete `Running&` had to wrap it: `MakeCallback_([&running]() -> Running& { return running; })`.

**After:** Two free-function overloads — `MakeCallback(std::function<Running&()>)` for lazy creation (SendStart) and `MakeCallback(Running&)` for the common case. Call sites become `MakeCallback(running)`.

**Lesson:** When a function's parameter is a callable that always reduces to a single object at most call sites, add an overload that takes the object directly. The compiler resolves the right path; call sites lose the wrapper noise.

## Replace (PushHandler, MatchPhaseCommon*) pair with Running& directly

**Before:** `SendXxx` methods took `const PushHandler& on_push` + `const MatchPhaseCommon* phase` as separate parameters. Every call site had to construct a lambda `[this](const PushFrame& f) { ApplyChildPushFrame(f); }` and pass `this` as the phase pointer. `SendRequestAndRead_` took both and threaded them through separately.

**After:** `SendXxx` takes `Running& running` directly. `MakeCallback_` wraps a `std::function<Running&()>` into a `PushHandler` internally, keeping the construction site-local. `SendRequestAndRead_` takes only `ChildMessageHandler& handler` + `const PushHandler& on_push`.

**Lesson:** When two parameters always travel together and one can be derived from the other, merge them into a single typed parameter and move the derivation into the callee.

## Use reference_wrapper<const T> in std::variant instead of wrapper structs

**Before:** `PushFrame` variant held `PostFrame`, `GameOverFrame`, etc. — structs whose only purpose was to wrap a proto message by value (copying it on every push frame). Every push-frame case in `SendRequestAndRead_` had to construct the struct explicitly.

**After:** `PushFrame` uses `std::reference_wrapper<const lgtbot::ipc::XxxResp>` directly. `on_push(resp.post())` passes the proto object by const reference with zero copies; the reference is valid for the synchronous duration of the call.

**Lesson:** When a variant alternative is just a named wrapper around a single value and the value outlives the variant (e.g., is a local on the caller's stack), prefer `reference_wrapper<const T>` over a named struct — it eliminates the copy and the struct definition.

## Collapse message-dispatch into a ChildMessageHandler virtual interface

**Before:** `AppendMsgItem` took `HostMsgSenderBase::MsgSenderGuard&` + `const MatchPhaseCommon*`. Reply delivery and player-ID resolution were conflated in a single function. Every caller had to open a guard and pass the phase pointer even when it just wanted to discard the output.

**After:** `ChildMessageHandler` is a pure virtual interface (`HandleText`, `HandleAtPlayerId`, …). `DoNothingHandler`, `ChildMessageReplyHandler`, and `ExtractTextHandler` implement it. `AppendMsgItem` dispatches to the interface. Callers that don't care pass `DoNothingHandler::Get()`.

**Lesson:** When a function needs different output behavior at different call sites (send to chat / extract text / discard), a small virtual interface is cleaner than threading extra pointers through every layer.
