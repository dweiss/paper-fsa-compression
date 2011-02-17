# Makefile for building a final state automaton
# Copyright (c) Jan Daciuk <jandac@pg.gda.pl>, 1996, 1997, 1998, 1999
#
# The most difficult parts written by Dominique Petitpierre

# These define i/o behaviour of programs
TEXT_IO = one_word_io.o	# texts as input, grep -like output
WORD_IO = one_word_io.o	# one word per line input


# Installation program
INSTALL = cp -i

# C++ compiler
CXX=g++

# Compile options (see the file INSTALL for detail)
# A_TERGO	- include code to build an index a tergo (recognizing word
#		  categories)
# CASECONV	- the first letter in spellchecking may be uppercase - check
#		  both upper & lower
# CHCLASS	- checks if a string is replaced with another string that
#		  sounds similar; in the present form, this checks one-letter
#		  strings against two-letter strings, and vice versa
# DEBUG		- produces huge amounts of useless data
# DESCENDING	- produces a bit smaller, but much slower automata
# DUMP_ALL	- does not print the leading space in fsa_prefix
# FLEXIBLE	- arc size should be adapted to automaton size; better
#		  compression, (slightly) less speed, architecture independence
# GENERALIZE	- used with A_TERGO to reduce the size of the guessing
#		  automaton, and to increase recall
# GUESS_LEXEMES	- tries to guess not only tags, but lexemes as well
#		  in fsa_guess
# GUESS_MMORPH	- makes it possible to use -m option in fsa_guess to predict
#		  morphological descriptions of lexemes corresponding to
#		  unknown inflected words; the descriptions are in the format
#		  of mmorph - MULTEXT morphology tool developed at ISSCO.
# GUESS_PREFIX	- tries to include information about prefixes to disambiguate
#		  morphological parses in fsa_guess
# JOIN_PAIRS	- used to prune the automaton (arcs share memory) with -X
#		  option in fsa_build
# LARGE_DICTIONARIES
#		- to build big but a little bit faster automata (do not use it)
# LOOSING_RPM	- to work around a bug in rpm libstdc++ library
# MORE_COMPR	- to built smaller automata more slowly
# MORPH_INFIX	- makes it possible to use -I and -P options in fsa_morph
#		  for recognition of coded prefixes and infixes
# NEXTBIT	- changes the format of the automaton, so that when there are
#		  chains of nodes, one following another, one bit is set
#		  in the goto field to indicate that fact, and only one byte
#		  from the goto field is used; it usually gives smaller
#		  automata
# NUMBERS	- it is possible to use fsa_hash and build dictionaries for
#		  perfect hashing
# POOR_MORPH	- enables -A option in fsa_morph for morphological analysis
#		  giving only categories, and no base forms.
# PROGRESS	- shows how many lines were read, what fsa_build does
# PRUNE_ARCS	- used with A_TERGO to reduce the size of the guessing
#		  automaton, and to increase precision
# RUNON_WORDS	- checks whether inserting a space inside the word results
#		  in two correct words in fsa_spell
# SHOW_FILLERS	- the filler character should be displayed in fsa_prefix
# SORT_ON_FREQ	- arcs should be sorted on frequency (better compression)
# SLOW_SPARSE	- try to fill every hole in sparse matrix representation
# SPARSE	- use sparse matrix representation
# STATISTICS	- shows some statistics after having built an automaton
# STOPBIT	- changes the format of the automaton, so that there are
#		  no counters, but for each arc there is a bit that says
#		  whether it is the last one in the node; this gives smaller
#		  automata
# TAILS		- changes the format of the automaton, allowing for more
#		  arc sharing, so more compression at the cost of construction
#		  time
# WEIGHTED	- introduces weights on arcs for guessing automata.
#
# PRUNE_ARCS works only with A_TERGO
# GUESS_LEXEMES works only with A_TERGO
# LARGE_DICTIONARIES and FLEXIBLE cannot be specified together
# NUMBERS works only with FLEXIBLE
# STOPBIT works only with FLEXIBLE
# (use FLEXIBLE)
#
# See INSTALL file for info on compile options.
#
# Some versions of g++ (or stdlibc++) are broken - if so, don't use -O2!
# !!! If you change these, please do make clean first before each make
CPPFLAGS=-O2 --pedantic -Wall \
  -DFLEXIBLE  \
  -DA_TERGO \
  -DGENERALIZE \
  -DSORT_ON_FREQ \
  -DSHOW_FILLERS  \
  -DSTOPBIT \
  -DNEXTBIT \
  -DMORE_COMPR \
  -DCASECONV \
  -DRUNON_WORDS \
  -DMORPH_INFIX \
  -DPOOR_MORPH \
  -DCHCLASS \
  -DGUESS_LEXEMES -DGUESS_PREFIX \
  -DGUESS_MMORPH \
  -DDUMP_ALL \
  -DPROGRESS \
  -DLOOSING_RPM #-DDMALLOC




# -pg

#  -DTAILS \
#  -DJOIN_PAIRS \
#  -DPRUNE_ARCS \
#  -DPROGRESS \
#  -DWEIGHTED \
#  -DSTATISTICS \
#  -DSPARSE \
#  -DUTF8 \

# Normally empty
#LDFLAGS=-L/usr/local/lib -ldmallocxx
LDFLAGS=

# Install directories
PREFIXDIR = /usr/local

# this is where fsa_build, fsa_spell, etc. should go
BINDIR = ${PREFIXDIR}/bin
# this is where the manuals should be kept
MANDIR = ${PREFIXDIR}/man
# this is where the dictionaries should go; also accent and language files
DICTDIR = ${PREFIXDIR}/lib
# this is where emacs lisp files go
LISPDIR = /usr/lib/emacs/site-lisp
# this is where tcl scripts go (also perl scripts used in tclmacq)
TCLMACQBINDIR = ${BINDIR}
# this is where tclmacq support files (help, language) go
TCLMACQDIR = ${PREFIXDIR}/lib
# The following should be empty if man fconfigure shows -encoding option,
# and set to \# otherwise. In other words, if your Tcl version is 8.0,
# you should set it to \#, and if it is 8.2 or higher -- leave it empty.
PREP_FCONF = \#
#PREP_FCONF
# to which man section man pages for fsa belong
MANSECT1 = 1
MANSECT5 = 5

########################################################################

# Objects that make particular programs
SPELL_OBJECTS = common.o spell.o nstr.o ${TEXT_IO} spell_main.o
ACCENT_OBJECTS = common.o nstr.o ${TEXT_IO} accent_main.o accent.o
FSA_B_OBJECTS = build_fsa.o nnode.o nindex.o nstr.o
FSA_S_OBJECTS = builds_fsa.o snode.o
FSA_U_OBJECTS = buildu_fsa.o unode.o
PREFIX_OBJECTS = common.o nstr.o one_word_io.o prefix.o prefix_main.o
GUESS_OBJECTS = common.o nstr.o ${TEXT_IO} guess.o guess_main.o
HASH_OBJECTS =  common.o nstr.o ${TEXT_IO} hash.o hash_main.o
MORPH_OBJECTS = common.o nstr.o ${TEXT_IO} morph.o morph_main.o
SYNTH_OBJECTS = common.o nstr.o ${TEXT_IO} synth.o synth_main.o
VISUAL_OBJECTS = common.o nstr.o ${TEXT_IO} visualize.o visual_main.o
ALL_PROGS = fsa_spell fsa_build fsa_accent fsa_prefix fsa_guess fsa_hash \
 fsa_morph fsa_ubuild fsa_visual fsa_synth
SKL_SCRIPTS = jspell jaccent jmorph jguess
TCL_SCRIPTS = tclmacq.tcl filesel.tcl
ALL_SCRIPTS = ${SKL_SCRIPTS} chkmorph.pl deguess.pl demorph.pl \
 find_irregular.pl gendata.pl mmorph23c.pl morph_data.pl morph_infix.pl \
 morph_prefix.pl prep_atg.pl prep_ati.pl prep_atl.pl prep_atp.pl \
 putinplace.pl simplify.pl sortatt.pl sortondesc.pl tclmacq.tcl filesel.tcl
# Note that awk scripts are not portable
AWK_SCRIPTS = de_morph_data.awk de_morph_infix.awk deguess.awk demorph.awk \
 find_irregular.awk mmorph23c.awk morph_data.awk morph_infix.awk \
 morph_prefix.awk prep_atg.awk prep_ati.awk prep_atl.awk prep_atp.awk
TCL_SUPP_FILES = tclmacq-help.txt tclmacq-lang.txt

ALL_OBJ = common.o spell.o nstr.o spell_main.o \
 accent_main.o accent.o build_fsa.o nnode.o nindex.o prefix.o prefix_main.o \
 guess.o guess_main.o hash.o hash_main.o morph.o morph_main.o builds_fsa.o \
 buildu_fsa.o unode.o snode.o visualize.o visual_main.o synth.o synth_main.o


all: ${ALL_PROGS}


fsa_spell: ${SPELL_OBJECTS}
	${CXX} ${CPPFLAGS} ${SPELL_OBJECTS} ${LDFLAGS} -o fsa_spell

fsa_accent: ${ACCENT_OBJECTS}
	${CXX} ${CPPFLAGS} ${ACCENT_OBJECTS} ${LDFLAGS} -o fsa_accent

fsa_build: ${FSA_B_OBJECTS} ${FSA_S_OBJECTS}
	${CXX} ${CPPFLAGS} ${FSA_B_OBJECTS} ${FSA_S_OBJECTS} ${LDFLAGS} -o fsa_build

fsa_ubuild: ${FSA_B_OBJECTS} ${FSA_U_OBJECTS}
	${CXX} ${CPPFLAGS} ${FSA_B_OBJECTS} ${FSA_U_OBJECTS} ${LDFLAGS} -o fsa_ubuild


fsa_prefix: ${PREFIX_OBJECTS}
	${CXX} ${CPPFLAGS} ${PREFIX_OBJECTS} ${LDFLAGS} -o fsa_prefix

fsa_guess: ${GUESS_OBJECTS}
	${CXX} ${CPPFLAGS} ${GUESS_OBJECTS} ${LDFLAGS} -o fsa_guess

fsa_hash: ${HASH_OBJECTS}
	${CXX} ${CPPFLAGS} ${HASH_OBJECTS} ${LDFLAGS} -o fsa_hash

fsa_morph: ${MORPH_OBJECTS}
	${CXX} ${CPPFLAGS} ${MORPH_OBJECTS} ${LDFLAGS} -o fsa_morph

fsa_synth: ${SYNTH_OBJECTS}
	${CXX} ${CPPFLAGS} ${SYNTH_OBJECTS} ${LDFLAGS} -o fsa_synth

fsa_visual: ${VISUAL_OBJECTS}
	${CXX} ${CPPFLAGS} ${VISUAL_OBJECTS} ${LDFLAGS} -o fsa_visual

fsa_dump: dump.cc
	${CXX} ${CPPFLAGS} dump.cc ${LDFLAGS} -o fsa_dump

common.o: common.cc fsa.h nstr.h common.h
	${CXX} ${CPPFLAGS} -c common.cc

spell.o: spell.cc fsa.h nstr.h spell.h common.h
	${CXX} ${CPPFLAGS} -c spell.cc

nstr.o:	nstr.cc nstr.h
	${CXX} ${CPPFLAGS} -c nstr.cc

build_fsa.o: build_fsa.cc nnode.h nindex.h nstr.h fsa.h fsa_version.h mkindex.cc
	${CXX} ${CPPFLAGS} -c build_fsa.cc

builds_fsa.o: builds_fsa.cc nnode.h nindex.h nstr.h fsa.h fsa_version.h mkindex.cc compile_options.h
	${CXX} ${CPPFLAGS} -c builds_fsa.cc

buildu_fsa.o: buildu_fsa.cc nnode.h unode.h nindex.h nstr.h fsa.h fsa_version.h mkindex.cc compile_options.h
	${CXX} ${CPPFLAGS} -c buildu_fsa.cc

nnode.o: nnode.cc nnode.h nstr.h fsa.h nindex.h
	${CXX} ${CPPFLAGS} -c nnode.cc

unode.o: unode.cc unode.h nnode.h nstr.h fsa.h nindex.h
	${CXX} ${CPPFLAGS} -c unode.cc

snode.o: snode.cc nnode.h nstr.h fsa.h nindex.h
	${CXX} ${CPPFLAGS} -c snode.cc

nindex.o: nindex.cc nindex.h nnode.h
	${CXX} ${CPPFLAGS} -c nindex.cc

one_word_io.o: one_word_io.cc fsa.h common.h
	${CXX} ${CPPFLAGS} -c one_word_io.cc

text_io.o: text_io.cc common.h fsa.h
	${CXX} ${CPPFLAGS} -c text_io.cc

spell_main.o: spell_main.cc common.h spell.h fsa_version.h compile_options.h
	${CXX} ${CPPFLAGS} -c spell_main.cc

accent_main.o: accent_main.cc common.h fsa_version.h accent.h compile_options.h
	${CXX} ${CPPFLAGS} -c accent_main.cc

prefix_main.o: prefix_main.cc common.h fsa_version.h prefix.h compile_options.h
	${CXX} ${CPPFLAGS} -c prefix_main.cc


accent.o: accent.cc fsa.h nstr.h common.h accent.h
	${CXX} ${CPPFLAGS} -c accent.cc

prefix.o: prefix.cc fsa.h nstr.h common.h prefix.h
	${CXX} ${CPPFLAGS} -c prefix.cc

guess.o: guess.cc guess.h fsa.h common.h nstr.h
	${CXX} ${CPPFLAGS} -c guess.cc

guess_main.o: guess_main.cc guess.h common.h fsa_version.h compile_options.h
	${CXX} ${CPPFLAGS} -c guess_main.cc

hash.o: hash.cc hash.h  fsa.h common.h nstr.h
	${CXX} ${CPPFLAGS} -c hash.cc

hash_main.o: hash_main.cc hash.h common.h fsa_version.h compile_options.h
	${CXX} ${CPPFLAGS} -c hash_main.cc

morph.o: morph.cc morph.h fsa.h common.h nstr.h
	${CXX} ${CPPFLAGS} -c morph.cc

morph_main.o: morph_main.cc morph.h common.h fsa_version.h compile_options.h
	${CXX} ${CPPFLAGS} -c morph_main.cc

synth.o: synth.cc synth.h fsa.h common.h nstr.h
	${CXX} ${CPPFLAGS} -c synth.cc

synth_main.o: synth_main.cc synth.h common.h fsa_version.h compile_options.h
	${CXX} ${CPPFLAGS} -c synth_main.cc

visualize.o: visualize.cc visualize.h fsa.h common.h nstr.h
	${CXX} ${CPPFLAGS} -c visualize.cc

visual_main.o: visual_main.cc visualize.h common.h fsa_version.h compile_options.h
	${CXX} ${CPPFLAGS} -c visual_main.cc

clean:
	rm -f ${ALL_OBJ}; \
	if [ -f ${TEXT_IO} ] ; then rm ${TEXT_IO} ; fi; \
	if [ -f ${WORD_IO} ] ; then rm ${WORD_IO} ; fi

realclean:
	rm -f ${ALL_OBJ} ${ALL_PROGS} ${SKL_SCRIPTS}; \
	if [ -f ${TEXT_IO} ] ; then rm ${TEXT_IO} ; fi; \
	if [ -f ${WORD_IO} ] ; then rm ${WORD_IO} ; fi

cleanuse:
	rm -f ${SPELL_OBJECTS} ${ACCENT_OBJECTS} ${PREFIX_OBJECTS}

cleanbuild:
	rm -f ${FSA_B_OBJECTS}

install: installbin installman installscripts installlisp installdicts

installman:
	if [ -d ${MANDIR}/man${MANSECT1} ] ; then \
		for manpage in *.1 ;\
		do newname=`basename $$manpage .1`.${MANSECT1} ;\
		   ${INSTALL} $$manpage ${MANDIR}/man${MANSECT1}/$$newname ;\
		done ;\
	else echo "Directory ${MANDIR}/man${MANSECT1} does not exist!" ; fi
	if [ -d ${MANDIR}/man${MANSECT5} ] ; then \
		for manpage in *.5 ;\
		do newname=`basename $$manpage .5`.${MANSECT5} ;\
		   ${INSTALL} $$manpage ${MANDIR}/man${MANSECT5}/$$newname ;\
		done ;\
	else echo "Directory ${MANDIR}/man${MANSECT5} does not exist!" ; fi

installbin: ${ALL_PROGS}
	${INSTALL} ${ALL_PROGS} ${BINDIR}

installbuild: fsa_build
	${INSTALL} fsa_build ${BINDIR}

installscripts:
	if [ -d ${BINDIR} ] ; then \
	for s in ${SKL_SCRIPTS} ; \
	do sed -e "s|XXX_DICT_DIR|${DICTDIR}|g" \
		-e "s|XXX_EXEC_DIR|${BINDIR}|g" < $${s}-skeleton > $${s} ; \
	done ; \
	for s in ${TCL_SCRIPTS} ; \
	do sed -e "s|@TCLMACQDIR@|${TCLMACQDIR}|g" \
		-e "s|@TCLMACQBINDIR@|${TCLMACQBINDIR}|g" \
		-e "s|@Q@|${PREP_FCONF}|g" < $${s}.in > $${s} ; \
	done; \
        chmod 0755 ${ALL_SCRIPTS} ; \
        ${INSTALL} ${ALL_SCRIPTS} ${BINDIR} ;\
	${INSTALL} ${TCL_SUPP_FILES} ${TCLMACQDIR} ; \
	else echo "Directory ${BINDIR} does not exist!" ; fi

installlisp:
	if [ -d ${LISPDIR} ] ; then ${INSTALL} jspell.el ${LISPDIR} ;\
	else echo "Directory ${LISPDIR} does not exist!" ; fi

installdicts:
	if [ -d ${DICTDIR} ] ; then \
	if ls *.fsa > /dev/null 2>/dev/null ; \
		then ${INSTALL} *.fsa ${DICTDIR} ; fi ; \
	if ls *.fsm > /dev/null 2>/dev/null ; \
		then ${INSTALL} *.fsm ${DICTDIR} ; fi ; \
	if ls *.atg > /dev/null 2>/dev/null ; \
		then ${INSTALL} *.atg ${DICTDIR} ; fi ; \
	if ls *.atl > /dev/null 2>/dev/null ; \
		then ${INSTALL} *.atl ${DICTDIR} ; fi ; \
	if ls *.atp > /dev/null 2>/dev/null ; \
		then ${INSTALL} *.atp ${DICTDIR} ; fi ; \
	if ls *.acc > /dev/null 2>/dev/null ; \
		then ${INSTALL} *.acc ${DICTDIR} ; fi ; \
	if ls *.lang > /dev/null 2>/dev/null ; \
		then ${INSTALL} *.lang ${DICTDIR} ; fi ; \
	if ls *.chcl > /dev/null 2>/dev/null ; \
		then ${INSTALL} *.chcl ${DICTDIR} ; fi ; \
	else echo "Directory ${DICTDIR} does not exist!" ; fi
