.PHONY: proftest
proftest:
	gcc -Werror proftest.c -lpthread -o proftest
