CXXFLAGS = $(OPTS) $(MOPTS) -I$(CURS_INCDIR) -DCURS_HEADER=$(CURS_HEADER)

OBJS = auxil.o error.o mysystem.o

include depend

%.o: %.cc
	@echo "  C++ " $<
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

dep:
	$(CXX) -MM *.cc > depend

clean:
	$(RM) *.o
	$(RM) common.a

common.a: $(OBJS)
	@echo "  AR  " $@
	@ar -r common.a $(OBJS) $(ARQUIET)
	@$(RANLIB) common.a

.PHONY: dep clean
