subdirs  = debugging
subdirs += rawIO14
subdirs += showBlock14
subdirs += sofs14
subdirs += mkfs14
subdirs += testifuncs14
subdirs += syscalls14
subdirs += mount14

.PHONY: $(subdirs)

all:	$(subdirs)

$(subdirs):
	@echo -e '\e[34;1;40;1mMaking $@...\e[m'
	@make -C $@

clean:
	@echo -e '\e[31;1;40;1mCleaning...\e[m'
	@for dir in $(subdirs) ; do make -C $$dir clean; done

