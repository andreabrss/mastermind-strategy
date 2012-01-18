/**
 * @file Main.cpp
 * Entry point of the program.
 *
 * \mainpage %Mastermind
 *
 * This project explores various strategies to solve the %Mastermind game 
 * and its variations. The final objective will be a program that solves the 
 * game both in real-time and offline.
 *
 * \section intro Introduction
 *
 * %Mastermind is a game played by two players, the <i>code maker</i> and
 * the <i>code breaker</i>. At the start of the game, the code maker thinks 
 * of a <i>secret</i> <i>codeword</i> with <code>p</code> pegs and 
 * <code>c</code> colors. Then the code breaker makes <i>guesses</i>.
 * For each guess, the code maker responds with a 
 * <i>feedback</i> in the form of <code>xAyB</code>, where <code>x</code>
 * is the number of correct colors in the correct pegs, and <code>y</code>
 * is the number of correct colors in the wrong pegs. The game continues
 * until the code breaker hits the secret.
 * 
 * There are two variations of the %Mastermind game. The traditional 
 * game is played on a board with four pegs and six color.
 * A codeword can contain the same color more than once. A variation of
 * the game is called GuessNumber, also known as Bulls-n-Cows. In this variation,
 * the codeword is not allowed to contain repetitive colors.
 *
 * This program plays the role of the code breaker. The goal is to find out
 * the secret within as few guesses as possible. The program supports both
 * the %Mastermind rule and the GuessNumber rule, subject to the limits:
 *    - Maximum number of colors (defined in <code>MM_MAX_COLORS</code>): 10
 *    - Maximum number of pegs (defined in <code>MM_MAX_PEGS</code>): 6
 *
 * \section strat Strageties of the Code Breaker
 * 
 * There are three types of strategies for the code breaker:
 *
 * <i>Simple strategies</i>. The code breaker just makes a random guess, as
 * long as the guess will bring some information. For example, a simple
 * choice can be the first codeword from the remaining possibilities. 
 * These strategies obviously perform poorly in that they take many
 * steps to find out the secret, but they are very fast and can be used as
 * a benchmark for other more intelligent strategies. The simple strategy 
 * is implemented in <code>Mastermind::SimpleCodeBreaker</code>.
 *
 * <i>Heuristic strategies</i>. The code breaker evaluates each potential 
 * guess according to some scoring criteria, and picks the one that scores 
 * the highest. These strategies are fast enough for real-time games, and
 * performs fairly well. Most research efforts in this field focus on finding
 * a good heuristic criteria, and a number of intuitive and well-performing
 * heuristics have been proposed. See Kooi (2005) for details. The heuristic
 * strategies are implemented in <code>Mastermind::HeuristicCodeBreaker</code>.
 * The supported heuristic criteria are defined in 
 * <code>Mastermind::HeuristicCriteria</code>.
 *
 * <i>Optimal strategy</i>. Since the number of possible secrets
 * and sequences of guesses are finite, the code breaker can employ a 
 * depth-first search to find out the <i>optimal</i> guessing strategy.
 * The optimal strategy minimizes the expected number of guesses, optionally
 * subject to a maximum-rounds constraint. Obviously these strategies are
 * very slow, and therefore they are not suitable for real-time application.
 * The exhaustive search strategy is implemented in 
 * <code>Mastermind::OptimalCodeBreaker</code>.
 *
 * \section optim Program Optimization Techniques
 *
 * While the code-breaking algorithms are quite straightforward, much
 * effort of this project has been put to optimize the performance of a 
 * real-time code breaker. Some of the hot-spots are identified by the
 * profiler. The major points of optimization are described
 * below. Most of the optimized routines are implemented in standalone
 * source files for clarity.
 *
 * <i>Search space pruning</i>. While all codewords are candidates for
 * making a guess, some of them are apparently "equal" in terms of
 * bringing new information. For example, in the first round of a
 * Number Guessing game, any guess works the same. Aware of this, we 
 * implement pruning by classifying digits into three classes: 
 * <i>impossible</i>, <i>unguessed</i>, and the rest. After each round
 * of feedback, we update the list of <i>distinct</i> guesses (in 
 * terms of bringing information) and search within this list only.
 * This reduces the search space significantly.
 *
 * <i>Codeword comparison</i>. This is the most intensive operation
 * in the program, accounting for 40% of all CPU time. The program uses
 * SSE2 instructions (implemented via compiler intrinsics) to compare
 * each pair of codewords in four instructions.
 *
 * <i>Feedback frequency counting</i>. The heuristic code breaker relies
 * heavily on these routines to count statistics on partitions. This 
 * is an intensive operation which accounts for about 20% of all CPU time.
 * The program uses an ASM implementation to maximize performance. See
 * <code>Frequency.cpp</code>.
 *
 * \section reference Reference
 *
 * <a href="http://www.dcc.fc.up.pt/~sssousa/RM09101.pdf">Knuth (1976)</a>
 * published the first paper on Mastermind, where he introduced a 
 * heuristic strategy that aimed at minimizing the worst-case number of
 * remaining possibilities.
 *
 * <a href="http://www.philos.rug.nl/~barteld/master.pdf">Kooi (2005)</a>
 * provided a concise yet thorough overview of the heuristic
 * algorithms to solve the %Mastermind game. He also proposed the %MaxParts
 * heuristic, which proved to work well for a %Mastermind game with 4 pegs
 * and 6 colors, but didn't work well for one with 5 pegs and 8 colors.
 *
 * <a href="http://logica.ugent.be/albrecht/thesis/Logik.pdf">Heeffer (2007)</a>
 * tested various heuristic algorithms on the %Mastermind game with 5 pegs
 * and 8 colors, and found the Entropy method to perform the best.
 *
 * Not much results seen out there for the Number Guesser game.
 */

#include <cstdio>
#include <iostream>

#include "Compare.h"
#include "Codeword.h"
#include "HRTimer.h"
#include "CodeBreaker.h"
#include "Test.h"
#include "Enumerate.h"

using namespace std;
using namespace Mastermind;

#if 0
static int RegressionTestUnit()
{
	CodewordRules rules;
	rules.length = 4;
	rules.ndigits = 10;
	rules.allow_repetition = false;

	// Codeword creation
	Codeword cw;
	if (!cw.IsEmpty()) {
		printf("FAILED: Codeword::IsEmpty()\n");
		return -1;
	}

	// Enumeration
	CodewordList list;
	list = CodewordList::Enumerate(rules);
	if (!(list.GetCount() == 5040)) {
		printf("FAILED: Wrong enumeration count\n");
		return -1;
	}
	if (!(list[357].ToString() == "0741")) {
		printf("FAILED: Wrong enumeration result\n");
		return -1;
	}

	// Comparison
	FeedbackList fbl(list[0], list);
	if (!(fbl.GetCount() == 5040)) {
		printf("FAILED: Codeword::CompareTo() returns wrong length of feedback\n");
		return -1;
	}
	if (!(fbl[3] == Feedback(3, 0))) {
		printf("FAILED: Incorrect comparison results\n");
		return -1;
	}

	// Count frequency
	return 0;
}

static int RegressionTest()
{
	printf("Running regression test...\n");
	if (RegressionTestUnit() == 0) {
		printf("Passed\n");
	} else {
		printf("Failed\n");
	}

	system("PAUSE");
	return 0;
}

static void DumpCodewordList(const CodewordList *list)
{
	for (int i = 0; i < list->GetCount(); i++) {
		cout << (*list)[i].ToString() << " ";
	}
}

static void TestGuessing(CodewordRules rules, CodeBreaker *breakers[], int nb)
{
	CodewordList all = CodewordList::Enumerate(rules);
	Feedback target(rules.length, 0);
	Utilities::HRTimer timer;

	printf("\n");
	printf("Algorithm Descriptions\n");
	printf("------------------------\n");
	for (int i = 0; i < nb; i++) {
		printf("  A%d: %s\n", (i + 1), breakers[i]->GetDescription().c_str());
	}

	printf("\n");
	printf("Frequency Table\n");
	printf("-----------------\n");
	printf("  ##:   Avg    1    2    3    4    5    6    7    8    9   >9     Time(s)\n");

	for (int ib = 0; ib < nb; ib++) {
		CodeBreaker *breaker = breakers[ib];

		// Initialize frequency table
		const int max_freq = 10;
		int freq[max_freq+1];
		for (int i = 1; i <= max_freq; i++) {
			freq[i] = 0;
		}

		int max_round = 0;
		int sum_round = 0;
		int count = all.GetCount();
		int pct = -1;

		// Test each possible secret
		timer.Start();
		for (int i = 0; i < count; i += 1) {
			if (i*100/count > pct) {
				pct = i*100/count;
				printf("\r  A%d: running... %2d%%", ib + 1, pct);
				fflush(stdout);
			}

			Codeword secret = all[i];

			int round = 0;
			breaker->Reset();
			for (round = 1; round < max_freq; round++) {
				Codeword guess = breaker->MakeGuess();
				Feedback fb = secret.CompareTo(guess);
				if (fb == target) {
					break;
				}
				breaker->AddFeedback(guess, fb);
			}

			if (round > max_round)
				max_round = round;
			sum_round += round;
			freq[round]++;
			//printf("%s : %d\n", secret.ToString().c_str(), round);
		}
		double t = timer.Stop();

		// Display statistics
		printf("\r  A%d: %5.3f ", (ib + 1), (double)sum_round / count);
		for (int i = 1; i <= max_freq; i++) {
			if (freq[i] > 0) 
				printf("%4d ", freq[i]);
			else
				printf("   - ");
		}
		printf("%8.2f\n", t);

	}

}

static void TestGuessingByTree(
	CodewordRules rules, 
	CodeBreaker *breakers[], 
	int nb,
	const Codeword& first_guess)
{
	CodewordList all = CodewordList::Enumerate(rules);
	Feedback target(rules.length, 0);
	Utilities::HRTimer timer;

	printf("Game Settings\n");
	printf("---------------\n");
	printf("  Codeword length:     %d\n", rules.length);
	printf("  Number of digits:    %d\n", rules.ndigits);
	printf("  Allow repetition:    %s\n", (rules.allow_repetition? "yes":"no"));
	printf("  Number of codewords: %d\n", all.GetCount());

	//printf("\n");
	//printf("Algorithm Descriptions\n");
	//printf("------------------------\n");
	//for (int i = 0; i < nb; i++) {
	//	printf("  A%d: %s\n", (i + 1), breakers[i]->GetDescription().c_str());
	//}

	printf("\n");
	printf("Frequency Table\n");
	printf("-----------------\n");
	printf("Strategy: Total   Avg    1    2    3    4    5    6    7    8    9   >9   Time\n");

	for (int ib = 0; ib < nb; ib++) {
		CodeBreaker *breaker = breakers[ib];

		// Build a strategy tree of this code breaker
		timer.Start();
		StrategyTree *tree = breaker->BuildStrategyTree(first_guess);
		double t = timer.Stop();

		// Count the steps used to get the answers
		const int max_rounds = 10;
		int freq[max_rounds+1];
		int sum_rounds = tree->GetDepthInfo(freq, max_rounds);
		int count = all.GetCount();

//			if (i*100/count > pct) {
//				pct = i*100/count;
//				printf("\r  A%d: running... %2d%%", ib + 1, pct);
//				fflush(stdout);
//			}

		// Display statistics
		printf("\r%8s:%6d %5.3f ", breaker->GetName(), 
			sum_rounds, (double)sum_rounds / count);
		for (int i = 1; i <= max_rounds; i++) {
			if (freq[i] > 0) 
				printf("%4d ", freq[i]);
			else
				printf("   - ");
		}
		printf("%6.1f\n", t);

		// delete tree;
		// TODO: garbage collection!
	}

}

int TestOutputStrategyTree(CodewordRules rules)
{
	//CodeBreaker *b = new HeuristicCodeBreaker<Heuristics::MinimizeAverage>(rules);
	CodeBreaker *b = new OptimalCodeBreaker(rules);
	Codeword first_guess;
	StrategyTree *tree = b->BuildStrategyTree(first_guess);
	//const char *filename = "E:/good-strat.txt";
	char filename[100];
	sprintf_s(filename, "./strats/mm-%dp%dc-%s-%s.xml", rules.length, rules.ndigits, 
		(rules.allow_repetition? "r":"nr"), b->GetName());
	FILE *fp = fopen(filename, "wt");
	tree->WriteToFile(fp, StrategyTree::XmlFormat, rules);
	fclose(fp);
	delete tree;

	system("PAUSE");
	return 0;
}

// c.f. http://www.javaworld.com.tw/jute/post/view?bid=35&id=138372&sty=1&tpg=1&ppg=1&age=0#138372

static int GetMaxBreakableWithin(
	int nsteps, 
	int f[], 
	CodewordList possibilities, 
	CodewordList all,
	unsigned short unguessed_mask,
	int expand_levels)
{
	assert(nsteps >= 0);
	assert(expand_levels >= 0);

	if (nsteps == 0) {
		return 0;
	} else if (nsteps == 1) {
		return 1;
	} 
	//else if (nsteps == 2) {
	//	int npegs = possibilities.GetRules().length;
	//	return std::min(possibilities.GetCount(), npegs*(npegs+3)/2);
	//}

	unsigned short impossible_mask = ((1<<all.GetRules().ndigits)-1) & ~possibilities.GetDigitMask();
	CodewordList candidates = all.FilterByEquivalence(unguessed_mask, impossible_mask);
	int best = 0;
	for (int i = 0; i < candidates.GetCount(); i++) {
		Codeword guess = candidates[i];
			
		int nguessable = 0;
		if (expand_levels == 0) {
			FeedbackFrequencyTable freq(FeedbackList(guess, possibilities));
			for (int fbv = 0; fbv <= freq.GetMaxFeedbackValue(); fbv++) {
				int partition_size = freq[Feedback(fbv)];
				nguessable += std::min(f[nsteps-1], partition_size);
			}
		} else {
			FeedbackFrequencyTable freq;
			possibilities.Partition(guess, freq);
			int partition_start = 0;
			for (int fbv = 0; fbv <= freq.GetMaxFeedbackValue(); fbv++) {
				int partition_size = freq[Feedback(fbv)];
				if (partition_size == 0)
					continue;

				CodewordList filtered(possibilities, partition_start, partition_size);
				int tmpcount = GetMaxBreakableWithin(nsteps-1, f, filtered, all, 
					unguessed_mask & ~guess.GetDigitMask(),
					expand_levels-1);
				nguessable += std::min(tmpcount, partition_size);
				partition_start += partition_size;
			}
		}

		best = std::max(best, nguessable);
	}
	return best;
}

static int TestBound(CodewordRules rules)
{
	// Find out the maximum size of any partition
	CodewordList all = CodewordList::Enumerate(rules);
	FeedbackFrequencyTable freq(FeedbackList(all[0], all));

	int f[100];
	f[0] = 0;
	f[1] = 1;
	int min_rounds = 0;
	int expand_levels = 1;
	for (int n = 2; n < 100; n++) {
		unsigned short unguessed_mask = ((1<<all.GetRules().ndigits)-1);
		f[n] = GetMaxBreakableWithin(n, f, all.Copy(), all, unguessed_mask, expand_levels);
		if (f[n] >= all.GetCount()) {
			min_rounds = n;
			break;
		}
	}
	for (int n = 1; n <= min_rounds; n++) {
		printf("Maximum number of secrets that can be guessed in %2d rounds: <= %d\n",
			n, f[n]);
	}
	printf("**** Minimum number of rounds necessary to guess all secrets: >= %d\n", 
		min_rounds);

	int min_steps = all.GetCount()*min_rounds;
	for (int n = 1; n < min_rounds; n++) {
		min_steps -= f[n];
	}
	printf("**** Minimum total steps necessary to guess all secrets: >= %d\n",
		min_steps);
	printf("**** Minimum average steps necessary to guess all secrets: >= %5.3f\n",
		(double)min_steps/all.GetCount());

	int *S = Mastermind::Heuristics::MinimizeSteps::partition_score;
	S[0] = 0;
	for (int i = 1; i <= all.GetCount(); i++) {
		int nr = 0;
		for (nr = 1; nr <= 100; nr++) {
			if (f[nr] >= i)
				break;
		}
		int score = i*nr;
		for (int j = 1; j < nr; j++) {
			score -= f[j];
		}
		S[i] = score;
		//if (i % 40==0)
		//printf("S[%d] = %d\n", i, score);
	}

#if 0
	int m=14;

	const int nmax=20;
	int S[nmax+1];
	S[0]=0;
	int n;
	for (n = 1; n <= nmax; n++) {
		if (n <= m) {
			S[n] = 1+(n-1)*2;
		} else {
			S[n]=n;
			int nper = (n-1)/(m-1);
			S[n] += S[nper]*(m-2);
			S[n] += S[(n-1)-(nper*(m-2))];
		}
		printf("S[%4d] = %d\n", n, S[n]);
	}
#endif

	//system("PAUSE");
	return 0;
}
#endif

// Latest profiling results:
// compare_long_codeword_v4():          28%
// FilterByEquivalenceClass_norep_v2(): 22%
// Unknown frames:                      17%
// count_freq_v6:                       16%
// GetSumOfSquares:                      5%
// __VEC_memzero:                        5%

// options
// -p4, --pegs=4       specify the number of pegs
// -c10, --colors=10   specify the number of colors
// -r, --rep           allow repetition
// -nr, --norep        don't allow repetition

// TODO: Test whether ordinal feedback or direct feedback performs better.
// TODO: Improve strategy tree to save memory (pointer) and be thread-safe
//       to prepare for multithreading.
// TODO: Refactor MakeGuess() code to make each call longer and fewer calls
//       to take advantage of OpenMP.
// TODO: Add progress display to OptimalCodeBreaker
// TODO: Output strategy tree after finishing a run
// TODO: Refactor StrategyTree() to speed up Destroy() and clean up memory
int main(int argc, char* argv[])
{
	string s;
	
	CodewordRules rules;
	if (1) {
		rules.length = 4;
		rules.ndigits = 10;
		rules.allow_repetition = false;
	} else if (1) {
		rules.length = 3;
		rules.ndigits = 7;
		rules.allow_repetition = false;
	} else {
		rules.length = 5;
		rules.ndigits = 8;
		rules.allow_repetition = true;
	}

#if 0
	TestBound(rules);
	//return system("PAUSE");

	// There is an interesting bug with the FilterEquivalence_Rep
	// If the first guess is 0011 and feedback is 0A1B,
	// Then the filter will think 1223 and 1233 are equivalent,
	// and keep the first one only.
	// But actually, the second codeword has better score 
	// in the worst-case heuristic criteria.
	// Let's find out why.
	if (0) {
		CodewordList all = CodewordList::Enumerate(rules);
		Codeword guess = Codeword::Parse("0011", rules);
		Feedback fb = Feedback(0, 1);
		CodewordList list = all.FilterByFeedback(guess, fb);

		unsigned char eqclass[16] = {
			0,1,3,4, 5,2,6,7, 8,9,10,11, 12,13,14,15 };
		codeword_t output[1296];
		int count = FilterByEquivalence_Rep(list.GetData(), list.GetCount(), eqclass, output);

		Feedback check(1, 3);
		// Find out the partition of 1223
		if (1) {
			const char *s = "1233";
			guess = Codeword::Parse(s, rules);
			FeedbackList fbl(guess, list);
			FeedbackFrequencyTable freq(fbl);
			//printf("Frequency table %s:\n", s);
			//freq.DebugPrint();
			printf("Guess=0011, Feedback=0A1B\n");
			printf("Guess=%s, Feedback=%s: ", s, check.ToString().c_str());
			list.FilterByFeedback(guess, check).DebugPrint();
			printf("\n");
		}

		// Find out the partition of 1233
		if (1) {
			const char *s = "1223";
			guess = Codeword::Parse(s, rules);
			FeedbackList fbl(guess, list);
			FeedbackFrequencyTable freq(fbl);
			//printf("Frequency table %s:\n", s);
			//freq.DebugPrint();
			printf("Guess=0011, Feedback=0A1B\n");
			printf("Guess=%s, Feedback=%s: ", s, check.ToString().c_str());
			list.FilterByFeedback(guess, check).DebugPrint();
			printf("\n");
		}

		system("PAUSE");
		return 0;
	}

	//return RegressionTest();

	//return TestOutputStrategyTree(rules);

#ifdef NDEBUG
#define LOOP_FLAG 1
#else
#define LOOP_FLAG 0
#endif
	//1829320017
	//return TestCompare(rules, "r_p1a", "r_p1a_omp1", 10000*LOOP_FLAG);
	//return TestCompare(rules, "r_p1a", "r_p1a_omp2", 10000*LOOP_FLAG);

	//return TestCompare(rules, "r_p1a", "r_p8", 10000000*LOOP_FLAG);
	//return TestFrequencyCounting(rules, 250000*LOOP_FLAG);
	//return TestEquivalenceFilter(rules, 10000*LOOP_FLAG);
	//return TestSumOfSquares(rules, "c", "c_p2", 15000000*LOOP_FLAG);
	//return TestSumOfSquares(rules, "c", "c_p2", 300000*LOOP_FLAG);
	//return TestNewScan(rules, 100000*1);
	//return BuildLookupTableForLongComparison();
	//return TestEnumerationDirect(200000*1);
	//return TestScan(rules, 100000*1);
	//return TestEnumeration(rules, 200000*1);

	Codeword first_guess = Codeword::Empty();
	//Codeword first_guess = Codeword::Parse("0011", rules);
	bool posonly = false; // only guess from remaining possibilities
	CodeBreaker* breakers[] = {
		new SimpleCodeBreaker(rules),
		new HeuristicCodeBreaker<Heuristics::MinimizeWorstCase>(rules, posonly),
		new HeuristicCodeBreaker<Heuristics::MinimizeAverage>(rules, posonly),
		new HeuristicCodeBreaker<Heuristics::MaximizeEntropy>(rules, posonly),
		new HeuristicCodeBreaker<Heuristics::MaximizePartitions>(rules, posonly),
		//new HeuristicCodeBreaker<Heuristics::MinimizeSteps>(rules, posonly),
		//new OptimalCodeBreaker(rules),
	};

	// CountFrequenciesImpl->SelectRoutine("c");
	TestGuessingByTree(rules, breakers, sizeof(breakers)/sizeof(breakers[0]), first_guess);
	printf("\n");

	if (0) {
		printf("\nRun again:\n");
		CountFrequenciesImpl->SelectRoutine("c_p8_il_os");
		TestGuessingByTree(rules, breakers, sizeof(breakers)/sizeof(breakers[0]), first_guess);
		printf("\n");
	}

	void PrintFrequencyStatistics();
	//PrintFrequencyStatistics();

	void PrintCompareStatistics();
	//PrintCompareStatistics();

	void PrintMakeGuessStatistics();
	//PrintMakeGuessStatistics();

	void OCB_PrintStatistics();
	OCB_PrintStatistics();

	system("PAUSE");
	return 0;

	printf("Enumerating all codewords...");
	CodewordList all = CodewordList::Enumerate(rules);
	printf(" total %d\n", all.GetCount());
	// DumpCodewordList(list);

	// Generate a secret codeword.
	Codeword secret; // = Codeword::GetRandom(rules);
	printf("Generated secret: %s\n", secret.ToString().c_str());

	// Let's guess!
	Feedback target(rules.length, 0);
	CodewordList list = all;
	for (int k = 1; k <= 9; k++) {
		// Which codeword to guess? The first one.
		Codeword guess = list[0];

		Feedback fb = secret.CompareTo(guess);
		printf("[%d] Guess: %s, feedback: %s... ",
			k, guess.ToString().c_str(), fb.ToString().c_str());
		if (fb == target) {
			printf("That's it!\n");
			break;
		}

		CodewordList possibilities = list.FilterByFeedback(guess, fb);
		printf("possibilities: %d\n", possibilities.GetCount());
		list = possibilities;
	}

	system("PAUSE");
	return 0;

	// Generate a secret codeword.
	// Codeword secret = Codeword::GetRandom(rules);
	cout << "Fyi, the secret is " << secret.ToString() << endl;

	// Let user guess, and provide feedback.
	while (1) {
		cout << "Input guess (q to exit): ";
		cin >> s;
		if (s == "q") {
			break;
		}

		Codeword guess = Codeword::Parse(s.c_str(), rules);
		if (guess.IsEmpty()) {
			cout << "Invalid input!" << endl;
		} else {
			Feedback feedback = secret.CompareTo(guess);
			cout << feedback.ToString() << endl;
		}
	}

#endif

	system("PAUSE");
	return 0;
}
