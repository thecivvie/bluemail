CXXFLAGS = $(OPTS) $(MOPTS)

OBJS = area.o bmail.o letter.o reader.o address.o driverl.o service.o \
       filelist.o filestat.o resource.o \
       driver/hmb.o driver/qwk.o driver/bbbs.o driver/demo.o driver/mbox.o \
       driver/omen.o driver/soup.o driver/basic.o driver/bwave.o \
       driver/hippo.o

include depend

%.o: %.cc
	@echo "  C++ " $<
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

dep:
	$(CXX) -MM *.cc > depend
	$(CXX) -MM driver/*.cc >> depend

clean:
	$(RM) *.o
	cd driver $(SEP) $(RM) *.o $(SEP) cd ..
	$(RM) bmail.a

bmail.a: $(OBJS)
	@echo "  AR  " $@
	@ar -r bmail.a $(OBJS) $(ARQUIET)
	@$(RANLIB) bmail.a

.PHONY: dep clean
