.text
.global _arm9_start

_arm9_start:
	bl arm9_main
forever:
	b forever
