# ×Ü¿Ø makefile

.PHONY:init clean

init:
	for i in `ls -F | grep '/$$'`;\
	do make -C $$i;\
	done

clean:
	for i in `ls -F | grep '/$$'`;\
	do make clean -C $$i;\
	done
	rm -f ts

