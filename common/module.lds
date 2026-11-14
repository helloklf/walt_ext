/* Static arm64 module linker script for hongkong, hand-derived from the
 * kernel module.lds.S + asm/module.lds.h with the device config:
 *   CONFIG_UNWIND_TABLES=y  -> .init.eh_frame (not discarded)
 *   CONFIG_KVM=y            -> .hyp.* sections
 *   no CONFIG_LTO_CLANG     -> no section merging needed
 *   no CONFIG_ARCH_USES_CFI_TRAPS / CRYPTO_FIPS140_MOD
 * The .plt / .init.plt collection is mandatory for arm64 CALL26 relocs. */
SECTIONS {
	/DISCARD/ : {
		*(.discard)
		*(.discard.*)
		*(.export_symbol)
	}

	__ksymtab		0 : { *(SORT(___ksymtab+*)) }
	__ksymtab_gpl		0 : { *(SORT(___ksymtab_gpl+*)) }
	__kcrctab		0 : { *(SORT(___kcrctab+*)) }
	__kcrctab_gpl		0 : { *(SORT(___kcrctab_gpl+*)) }

	.ctors			0 : ALIGN(8) { *(SORT(.ctors.*)) *(.ctors) }
	.init_array		0 : ALIGN(8) { *(SORT(.init_array.*)) *(.init_array) }

	.altinstructions	0 : ALIGN(8) { KEEP(*(.altinstructions)) }
	__bug_table		0 : ALIGN(8) { KEEP(*(__bug_table)) }
	__jump_table		0 : ALIGN(8) { KEEP(*(__jump_table)) }

	__patchable_function_entries : { *(__patchable_function_entries) }

	__param 0 : ALIGN(8) { *(__param) }

	.plt 0 : { *(.plt) }
	.init.plt 0 : { *(.init.plt) }
	.text.ftrace_trampoline 0 : { BYTE(0) }

	.init.eh_frame : { *(.eh_frame) }

	.hyp.text : ALIGN(4096) { *(.hyp.text) . = ALIGN(4096); }
	.hyp.bss : ALIGN(4096) { *(.hyp.bss) . = ALIGN(4096); }
	.hyp.rodata : ALIGN(4096) { *(.hyp.rodata) . = ALIGN(4096); }
	.hyp.data : ALIGN(4096) { *(.hyp.data) . = ALIGN(4096); }
	.hyp.reloc : ALIGN(4) { *(.hyp.reloc) }
}