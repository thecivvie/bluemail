%.txt: %.[1,5]
	@echo "  GROFF " $@
	@GROFF_NO_SGR=1 groff -Tascii -mandoc $< | col -b | recode latin1..ibmpc > $@

txt: bmail.txt bmailrc.txt bmuncoll.txt

clean:
	$(RM) bmail.txt
	$(RM) bmailrc.txt
	$(RM) bmuncoll.txt

.PHONY: txt clean
