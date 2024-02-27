	.text
	.global _start

_start:
	.byte	0xED, 0xA5, 0x8D, 0xBF
	.byte	0x20, 0x43, 0x68, 0x69, 0x73, 0x68, 0x6D, 0x00
	.byte	0x01, 0x09, 0x00, 0x09

driver_name:
	.ascii	"Twice DLDI"
	.zero	(48+driver_name-.)

	.word	0x00000000
	.word	0x00000200
	.zero	24
	.ascii	"TWCE"
	.word	0x00000023

	.word	startup
	.word	is_inserted
	.word	read_sectors
	.word	write_sectors
	.word	clear_status
	.word	shutdown
