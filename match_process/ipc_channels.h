#pragma once

// IPC between bot_core (parent) and match_process runners (child):
//
//   Protocol: length-prefixed frames on stdin (parent -> child) and stdout (child -> parent).
//   stderr is inherited from the parent for human-readable logs (glog, fprintf, ...).
//
// Child: ReadFrame(stdin, ...), WriteFrame(stdout, ...).
// Parent: Subprocess::Write / Subprocess::Read (reproc_write / reproc_read OUT).
