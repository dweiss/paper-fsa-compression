/***	compile_options.h	***/

/*	This file contains code that prints out compile options
	used to compile a particular program.
*/

      // version details
      cout << VERSION << endl;
#ifdef LARGE_DICTIONARIES
      cout << "Compiled with LARGE_DICTIONARIES (why are you using it?)"
      << endl;
#endif
#ifdef FLEXIBLE
      cout << "Compiled with FLEXIBLE (use it!)" << endl;
#ifdef LARGE_DICTIONARIES
      cout << "-- turn LARGE_DICTIONARIES OFF!" << endl;
#endif
#endif
#ifdef STOPBIT
      cout << "Compiled with STOPBIT (better compression at no cost)" << endl;
#endif
#ifdef NEXTBIT
      cout << "Compiled with NEXTBIT (better compression at almost no cost)"
      << endl;
#endif
#ifdef TAILS
      cout << "Compiled with TAILS (sequences of last transitions share space)"
      << endl;
#endif
#ifdef WEIGHTED
      cout << "Compiled with WEIGHTED (arcs with weights for probabilities in fsa_guess)"
      << endl;
#endif
#ifdef MORE_COMPR
      cout << "Compiled with MORE_COMPR (better compression at cost of construction speed)"
      << endl;
#endif
#ifdef NUMBERS
      cout << "Compiled with NUMBERS (perfect hashing possible)" << endl;
#endif
#ifdef DESCENDING
      cout << "Compiled with DESCENDING (arcs sorted in descending order)"
	<< endl;
#endif
#ifdef JOIN_PAIRS
      cout << "Compiled with JOIN_PAIRS (2-arc nodes share space)" << endl;
#endif
#ifdef SORT_ON_FREQ
      cout << "Compiled with SORT_ON_FREQ (arcs sorted on frequency)"
	<< endl;
#endif
#ifdef A_TERGO
      cout << "Compiled with A_TERGO (building guessing automata possible)"
	<< endl;
#endif
#ifdef PROGRESS
      cout << "Compiled with PROGRESS (progress indicator during building)"
	<< endl;
#endif
#ifdef GENERALIZE
      cout << "Compiled with GENERALIZE (more predictions in fsa_guess,"
           << " less accurate)"
	<< endl;
#endif
#ifdef PRUNE_ARCS
      cout << "Compiled with PRUNE_ARCS (less predictions in fsa_guess,"
           << " more accurate)"
	<< endl;
#endif
#ifdef GUESS_LEXEMES
    cout << "Compiled with GUESS_LEXEMES (fsa_guess guesses lexemes)"
      << endl;
#endif
#ifdef GUESS_MMORPH
    cout << "Compiled with GUESS_MMORPH (fsa_guess accepts -m)" << endl;
#endif
#ifdef GUESS_PREFIX
    cout << "Compiled with GUESS_PREFIX (fsa_guess accepts -P)" << endl;
#endif
#ifdef MORPH_INFIX
    cout << "Compiled with MORPH_INFIXES (fsa_guess accepts -I)" << endl;
#endif
#ifdef POOR_MORPH
    cout << "Compiled with POOR_MORPH (fsa_guess accepts -A)" << endl;
#endif
#ifdef CASECONV
    cout << "Compiled with CASECONV (capitalized words checked lowercase)"
      << endl;
#endif
#ifdef CHCLASS
    cout << "Compiled with CHCLASS (diagraph correction in fsa_spell)" << endl;
#endif
#ifdef DEBUG
    cout << "Compiled with DEBUG" << endl;
    cout << "Are you crazy?!!!" << endl;
#endif
#ifdef RUNON_WORDS
    cout << "Compiled with RUNON_WORDS (check run-on words in fsa_spell)"
         << endl;
#endif
#ifdef SHOW_FILLERS
    cout << "Compiled with SHOW_FILLERS (fsa_prefix shows fillers)" << endl;
#endif
#ifdef STATISTICS
    cout << "Compiled with STATISTICS (building statistics)" << endl;
#endif
#ifdef DUMP_ALL
    cout << "Compiled with DUMP_ALL (no leading space in fsa_prefix)" << endl;
#else
    cout << "Compiled without DUMP_ALL (leading space in fsa_prefix)" << endl;
#endif
#ifdef SPARSE
#if defined(FLEXIBLE) && defined(STOPBIT)
    cout << "Compiled with SPARSE (sparse matrix representation used)" << endl;
#else
    cout << "Compiled with SPARSE but without FLEXIBLE or STOPBIT" << endl;
#endif
#ifdef SLOW_SPARSE
    cout << "Compiled with SLOW_SPARSE (checking every hole in sparse matrix)"
         << endl;
#else
    cout << "Compiled without SPARSE (sparse matrix representation not used)"
         << endl;
#endif
#endif
#ifdef UTF8
cout << "Compiled with UTF8 (partial support for UTF8 e.g. in case conversion)"
     << endl;
#else
cout << "Compiled without UTF8 (less support for UTF8)" << endl;
#endif

/***	EOF compile_options.h	***/
