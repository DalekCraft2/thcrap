/**
  * Touhou Community Reliant Automatic Patcher
  * Main DLL
  *
  * ----
  *
  * Breakpoint entry point. Written for i686-w64-mingw32-as.
  */

	.intel_syntax noprefix
	.global	_bp_entry, _bp_entry_indexptr, _bp_entry_localptr, _bp_entry_callptr, _bp_entry_end

	.macro pushDW value
	.byte 0x68
	.int \value
	.endm

_bp_entry:
	pusha
	pushf
	cld
_bp_entry_indexptr:
	pushDW	0x00000000
_bp_entry_localptr:
	pushDW	0x00000000
_bp_entry_callptr:
	call	_breakpoint_process
	lea		esp, [esp+eax+0x8]
	popf
	popa
	ret
	.balign 16, 0xCC
_bp_entry_end:
