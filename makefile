.PHONY: test clean

program=edf

test: $(program)
	cat PS.CKGWC_SC.U_DI.A_GP.SIES3-F16-R99990-B9999090-APGA_AR.GLOBAL_DD.20130301_TP.000001-235959_DF.EDR | ./$<

$(program): $(program).cpp
	clang++ -std=c++14 -Wall -Wextra -o$@ $<

clean:
	rm -f $(program)
	rm -f *.o
