ENTRY(entry)

MEMORY
{
    RAM    (rwx)  : ORIGIN = 0x00007C00, LENGTH = 128K
}

SECTIONS
{
	.entry				: { __entry_start = . ;			*(.entry)	}
	.text				: { __text_start = . ;			*(.text)	}
	.data				: { __data_start = . ;			*(.data)	}
	.rdata				: { __rdata_start = . ;			*(.rdata)	}
	.rodata				: { __rodata_start = . ;		*(.rodata)	}
	.bss				: { __bss_start = . ;			*(.bss)		}

	__end = .;
}