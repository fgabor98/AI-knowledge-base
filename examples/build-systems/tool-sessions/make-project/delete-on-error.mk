# A generator that fails after writing half of its output.
table.h:
	printf '#define TABLE_SIZE 16\n' > $@
	false
