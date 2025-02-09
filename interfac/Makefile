CXXFLAGS = $(OPTS) $(MOPTS) -I$(CURS_INCDIR) -DCURS_HEADER=$(CURS_HEADER)

OBJS = main.o help.o color.o files.o window.o filedbs.o isoconv.o letterl.o \
       letterw.o offline.o tagline.o wincurs.o addrbook.o ansiview.o \
       arealist.o bulletin.o interfac.o replymgr.o services.o

include depend

%.o: %.cc
	@echo "  C++ " $<
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

dep:
	$(CXX) -MM *.cc > depend

clean:
	$(RM) *.o
	$(RM) interfac.a

interfac.a: $(OBJS)
	@echo "  AR  " $@
	@ar -r interfac.a $(OBJS) $(ARQUIET)
	@$(RANLIB) interfac.a

.PHONY: dep clean
