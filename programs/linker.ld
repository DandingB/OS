ENTRY(entry)

SECTIONS
{
	.text				: { __text_start = . ;			*(.text)	}
	.data				: { __data_start = . ;			*(.data)	}
	.rdata				: { __rdata_start = . ;			*(.rdata)	}
	.rodata				: { __rodata_start = . ;		*(.rodata)	}
	.bss				: { __bss_start = . ;			*(.bss)		}

	__end = .;
}