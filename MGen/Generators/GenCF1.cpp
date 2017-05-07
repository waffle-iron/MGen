#include "../stdafx.h"
#include "GenCF1.h"

#ifdef _DEBUG
#define new DEBUG_NEW 
#endif

// Report violation
#define FLAG(id, i) { if ((skip_flags) && (accept[id] == 0)) goto skip; if (accept[id] > -1) { flags[0] = 0; flags[id] = 1; nflags[i][nflagsc[i]] = id; ++nflagsc[i]; } }
#define FLAG2(id, i) { if ((skip_flags) && (accept[id] == 0)) return 1; if (accept[id] > -1) { flags[0] = 0; flags[id] = 1; nflags[i][nflagsc[i]] = id; ++nflagsc[i]; } }

// Convert chromatic to diatonic
#define CC_C(note) (chrom_to_dia[(note + 12 - tonic_cur) % 12] + ((note + 12 - tonic_cur) / 12 - 1) * 7)

// Convert diatonic to chromatic
#define C_CC(note) (dia_to_chrom[note % 7] + (note / 7) * 12 + tonic_cur)

const CString FlagName[MAX_FLAGS] = {
	"Strict", // 0
	"Minor seventh", // 1
	"Tritone resolved", // 2 
	"Many leaps", // 3 
	"Long smooth", // 4 
	"Long line", // 5 
	"Two 3rds", // 6 
	"Late <6th resolution", // 7 
	"Leap back <7th", // 8 
	"Close repeat", // 9 
	"Stagnation", // 10 
	"Early-late filled >4th", // 11 
	"Multiple culminations", // 12 
	"2nd to last not GBD", // 13
	"3rd to last is CEG", // 14
	"3 letters in a row [C]", // 15
	"4 letters in a row [C]", // 16
	">4 letters in a row [C]", // 17
	"4 step miss [V]", // 18
	"5 step miss [V]", // 19
	">5 step miss [V]", // 20
	"Late culmination", // 21
	"Leap back >6th", // 22
	"Last leap", // 23
	"Unfilled leap >4th", // 24
	"Many leaps+", // 25
	"Leap unresolved", // 26
	"Leap chain", // 27
	"Two 3rds after 6/8", // 28
	"Late >5th resolution", // 29
	"Preleaped unresolved 3rd", // 30
	"Tritone unresolved", // 31
	"Tritone culmination", // 32
	"Leap to leap resolution", // 33
	"3rd to last is leading", // 34
	"Preleaped unfilled 3rd", // 35
	"Outstanding repeat", // 36
	"Too wide range", // 37
	"Too tight range", // 38
	"Major seventh", // 39
	"First steps without C", // 40
	"First steps without E", // 41
	"3 letters in a row [V]", // 42
	"4 letters in a row [V]", // 43
	">4 letters in a row [V]", // 44
	"4 step miss [C]", // 45
	"5 step miss [C]", // 46
	">5 step miss [C]", // 47
	"G-C before cadence", // 48
	"First not C", // 49
	"Last not C", // 50
	"2nd to last is G", // 51
	"Start tonic unprepared", // 52
	"Prefilled unresolved 3rd", // 53
	"Unresolved 3rd", // 54
	"Preleaped unresolved 4th", // 55
	"Prefilled unresolved 4th", // 56
	"Unresolved 4th", // 57
	"Leap back overflow", // 58
	"Preleaped unresolved >4th", // 59
	"Prefilled unresolved >4th", // 60
	"Prefilled unfilled 3rd", // 61
	"Prefilled unfilled 4th", // 62
	"Prefilled unfilled >4th", // 63
	"Preleaped unfilled 4th", // 64
	"Preleaped unfilled >4th", // 65
	"Early-late filled 3rd", // 66
	"Early-late filled 4th", // 67
	"Unfilled 3rd", // 68
	"Unfilled 4th", // 69
	"Consecutive leaps", // 70
	"Consecutive leaps+", // 71
};

const Color FlagColor[] = {
	Color(0, 100, 100, 100), // 0 S
};

// Unskippable rules:
// Note repeats note of previous measure

CGenCF1::CGenCF1()
{
	//midifile_tpq_mul = 8;
	accept.resize(MAX_FLAGS);
	flag_to_sev.resize(MAX_FLAGS);
	flag_color.resize(MAX_FLAGS);
	// Start severity
	sev_to_flag[0] = 0;
	cur_severity = 1;
}

CGenCF1::~CGenCF1()
{
}

void CGenCF1::LoadConfigLine(CString* sN, CString* sV, int idata, float fdata)
{
	CheckVar(sN, sV, "min_interval", &min_interval);
	CheckVar(sN, sV, "max_interval", &max_interval);
	CheckVar(sN, sV, "c_len", &c_len);
	CheckVar(sN, sV, "s_len", &s_len);
	LoadNote(sN, sV, "first_note", &first_note);
	LoadNote(sN, sV, "last_note", &last_note);
	CheckVar(sN, sV, "fill_steps_mul", &fill_steps_mul);
	CheckVar(sN, sV, "max_repeat_mul", &max_repeat_mul);
	CheckVar(sN, sV, "max_smooth_direct", &max_smooth_direct);
	CheckVar(sN, sV, "max_smooth", &max_smooth);
	CheckVar(sN, sV, "max_leaps", &max_leaps);
	CheckVar(sN, sV, "max_leaps2", &max_leaps2);
	CheckVar(sN, sV, "cse_leaps", &cse_leaps);
	CheckVar(sN, sV, "cse_leaps2", &cse_leaps2);
	CheckVar(sN, sV, "max_leap_steps", &max_leap_steps);
	CheckVar(sN, sV, "stag_notes", &stag_notes);
	CheckVar(sN, sV, "stag_note_steps", &stag_note_steps);
	LoadRange(sN, sV, "tempo", &min_tempo, &max_tempo);
	CheckVar(sN, sV, "random_choose", &random_choose);
	CheckVar(sN, sV, "random_key", &random_key);
	CheckVar(sN, sV, "random_seed", &random_seed);
	CheckVar(sN, sV, "repeat_steps", &repeat_steps);
	CheckVar(sN, sV, "shuffle", &shuffle);
	CheckVar(sN, sV, "first_steps_tonic", &first_steps_tonic);
	CheckVar(sN, sV, "show_severity", &show_severity);
	CheckVar(sN, sV, "calculate_correlation", &calculate_correlation);
	CheckVar(sN, sV, "calculate_stat", &calculate_stat);
	CheckVar(sN, sV, "calculate_blocking", &calculate_blocking);
	CheckVar(sN, sV, "late_require", &late_require);
	// Load variants of possible harmonic meaning
	if (*sN == "harm_var") {
		++parameter_found;
		int pos = 0;
		CString st;
		for (int i = 0; i<1000; i++) {
			if (pos == -1) break;
			st = sV->Tokenize(",", pos);
			st.Trim();
			if (st.Find("D") >= 0) hvd.push_back(i);
			if (st.Find("S") >= 0) hvs.push_back(i);
			if (st.Find("T") >= 0) hvt.push_back(i);
		}
	}
	// Load constant harmonic meaning
	if (*sN == "harm_const") {
		++parameter_found;
		int pos = 0;
		CString st;
		for (int i = 0; i<1000; i++) {
			if (pos == -1) break;
			st = sV->Tokenize(",", pos);
			st.Trim();
			if (st.Find("D") >= 0) hcd.push_back(i);
			if (st.Find("S") >= 0) hcs.push_back(i);
			if (st.Find("T") >= 0) hct.push_back(i);
		}
	}
	// Load tonic
	if (*sN == "key") {
		++parameter_found;
		if (sV->Right(1) == "m") {
			*sV = sV->Left(sV->GetLength() - 1);
			//minor = 1;
		}
		tonic_cur = GetPC(*sV);
	}
	// Load accept
	CString st;
	for (int i = 0; i < MAX_FLAGS; ++i) {
		st = FlagName[i];
		st.MakeLower();
		if (*sN == st) {
			++parameter_found;
			accept[i] = atoi(*sV);
			if (*sV == "X") accept[i] = -1;
			// Check if not Strict flag
			if (i) {
				if (cur_severity == MAX_FLAGS) {
					CString* est = new CString;
					est->Format("Warning: more flags in config than in algorithm. Possibly duplicate flags in config. Please correct config %s", m_config);
					WriteLog(1, est);
				}
				else {
					// Check if flag already has severity
					if (flag_to_sev[i]) {
						CString* est = new CString;
						est->Format("Warning: detected duplicate flag %s. Please correct config %s", FlagName[i], m_config);
						WriteLog(1, est);
					}
					else {
						// Load severity based on position in file
						sev_to_flag[cur_severity] = i;
						flag_to_sev[i] = cur_severity;
						// Log
						//CString* est = new CString;
						//est->Format("Flag '%s' gets severity %d", FlagName[i], cur_severity);
						//WriteLog(1, est);
						++cur_severity;
					}
				}
			}
			// Do not load flag multiple times
			break;
		}
	}
}

void CGenCF1::LogCantus(vector<int> &c)
{
	CString st, st2;
	for (int i = 0; i < c.size(); ++i) {
		st.Format("%d ", c[i]);
		st2 += st;
	}
	CGLib::AppendLineToFile("temp.log", st2 + " \n");
}

void CGenCF1::FillCantus(vector<int>& c, int step1, int step2, int value)
{
	// Step2 must be exclusive
	for (int i = step1; i < step2; ++i) {
		c[i] = value;
	}
}

void CGenCF1::FillCantusMap(vector<int>& c, vector<int>& smap, int step1, int step2, vector<int>& value)
{
	// Step2 must be exclusive
	for (int i = step1; i < step2; ++i) {
		c[smap[i]] = value[smap[i]];
	}
}

// Detect repeating notes
int CGenCF1::FailNoteRepeat(vector<int> &c, int step1, int step2) {
	for (int i = step1; i < step2; ++i) {
		if (c[i] == c[i + 1]) return 1;
	}
	return 0;
}

// Detect prohibited note sequences
int CGenCF1::FailNoteSeq(vector<int> &pc, int step1, int step2, vector<int> &flags, vector<vector<int>> &nflags, vector<int> &nflagsc) {
	for (int i = step1; i < step2-2; ++i) {
		if (pc[i] == 4 && pc[i + 1] == 0) FLAG2(48, i)
	}
	return 0;
}

// Count limits
void CGenCF1::GetMelodyInterval(vector<int> &c, int step1, int step2, int &nmin, int &nmax) {
	nmin = MAX_NOTE;
	nmax = 0;
	for (int i = step1; i < step2; ++i) {
		if (c[i] < nmin) nmin = c[i];
		if (c[i] > nmax) nmax = c[i];
	}
}

// Clear flags
void CGenCF1::ClearFlags(vector<int> &flags, vector<int> &nflagsc, int step1, int step2) {
	if (!skip_flags) fill(flags.begin(), flags.end(), 0);
	flags[0] = 1;
	for (int i = step1; i < step2; ++i) {
		nflagsc[i] = 0;
	}
}

int CGenCF1::FailRange(int nmin, int nmax, vector<int> &flags, vector<vector<int>> &nflags, vector<int> &nflagsc) {
	if (nmax - nmin > max_interval) FLAG2(37, 0);
	if (nmax - nmin < min_interval) FLAG2(38, 0);
	return 0;
}

// Calculate pitch class
void CGenCF1::GetPitchClass(vector<int> &c, vector<int> &pc, int step1, int step2) {
	for (int i = step1; i < step2; ++i) {
		pc[i] = c[i] % 7;
	}
}

int CGenCF1::FailMelodyHarmSeqStep(vector<int> &pc, int i, int &count, int &wcount, vector<int> &hv, vector<int> &hc, vector<int> &flags, vector<vector<int>> &nflags, vector<int> &nflagsc) {
	if (find(hv.begin(), hv.end(), pc[i]) != hv.end()) {
		if (wcount == 4) FLAG2(18, i - 1);
		if (wcount == 5) FLAG2(19, i - 1);
		if (wcount > 5) FLAG2(20, i - 1);
		wcount = 0;
	}
	else {
		++wcount;
	}
	if (find(hc.begin(), hc.end(), pc[i]) != hc.end()) {
		++count;
	}
	else {
		if (count == 3) FLAG2(15, i - 1);
		if (count == 4) FLAG2(16, i - 1);
		if (count > 4) FLAG2(17, i - 1);
		count = 0;
	}
	return 0;
}

int CGenCF1::FailMelodyHarmSeq(vector<int> &pc, int ep1, int ep2, vector<int> &flags, vector<vector<int>> &nflags, vector<int> &nflagsc) {
	int dcount = 0;
	int scount = 0;
	int tcount = 0;
	int wdcount = 0;
	int wscount = 0;
	int wtcount = 0;
	for (int i = 0; i < ep2; ++i) {
		// Count same and missing letters in a row
		if (FailMelodyHarmSeqStep(pc, i, tcount, wtcount, hvt, hct, flags, nflags, nflagsc)) return 1;
		if (FailMelodyHarmSeqStep(pc, i, dcount, wdcount, hvd, hcd, flags, nflags, nflagsc)) return 1;
		if (FailMelodyHarmSeqStep(pc, i, scount, wscount, hvs, hcs, flags, nflags, nflagsc)) return 1;
	}
	// Check same letters
	if ((tcount == 3) || (dcount == 3) || (scount == 3)) FLAG2(15, ep2 - 1);
	if ((tcount == 4) || (dcount == 4) || (scount == 4)) FLAG2(16, ep2 - 1);
	if ((tcount > 4) || (dcount > 4) || (scount > 4)) FLAG2(17, ep2 - 1);
	// Check missing letters
	if ((wtcount == 4) || (wdcount == 4) || (wscount == 4)) FLAG2(18, ep2 - 1);
	if ((wtcount == 5) || (wdcount == 5) || (wscount == 5)) FLAG2(19, ep2 - 1);
	if ((wtcount > 5) || (wdcount > 5) || (wscount > 5)) FLAG2(20, ep2 - 1);
	return 0;
}

int CGenCF1::FailMelodyHarmSeqStep2(vector<int> &pc, int i, int &count, int &wcount, vector<int> &hc, vector<int> &hv, vector<int> &flags, vector<vector<int>> &nflags, vector<int> &nflagsc) {
	if (find(hv.begin(), hv.end(), pc[i]) != hv.end()) {
		if (wcount == 4) FLAG2(45, i - 1);
		if (wcount == 5) FLAG2(46, i - 1);
		if (wcount > 5) FLAG2(47, i - 1);
		wcount = 0;
	}
	else {
		++wcount;
	}
	if (find(hc.begin(), hc.end(), pc[i]) != hc.end()) {
		++count;
	}
	else {
		if (count == 3) FLAG2(42, i - 1);
		if (count == 4) FLAG2(43, i - 1);
		if (count > 4) FLAG2(44, i - 1);
		count = 0;
	}
	return 0;
}

int CGenCF1::FailMelodyHarmSeq2(vector<int> &pc, int ep1, int ep2, vector<int> &flags, vector<vector<int>> &nflags, vector<int> &nflagsc) {
	int dcount = 0;
	int scount = 0;
	int tcount = 0;
	int wdcount = 0;
	int wscount = 0;
	int wtcount = 0;
	for (int i = 0; i < ep2; ++i) {
		// Count same and missing letters in a row
		if (FailMelodyHarmSeqStep2(pc, i, tcount, wtcount, hvt, hct, flags, nflags, nflagsc)) return 1;
		if (FailMelodyHarmSeqStep2(pc, i, dcount, wdcount, hvd, hcd, flags, nflags, nflagsc)) return 1;
		if (FailMelodyHarmSeqStep2(pc, i, scount, wscount, hvs, hcs, flags, nflags, nflagsc)) return 1;
	}
	// Check same letters
	if ((tcount == 3) || (dcount == 3) || (scount == 3)) FLAG2(42, ep2 - 1);
	if ((tcount == 4) || (dcount == 4) || (scount == 4)) FLAG2(43, ep2 - 1);
	if ((tcount > 4) || (dcount > 4) || (scount > 4)) FLAG2(44, ep2 - 1);
	// Check missing letters
	if ((wtcount == 4) || (wdcount == 4) || (wscount == 4)) FLAG2(45, ep2 - 1);
	if ((wtcount == 5) || (wdcount == 5) || (wscount == 5)) FLAG2(46, ep2 - 1);
	if ((wtcount > 5) || (wdcount > 5) || (wscount > 5)) FLAG2(47, ep2 - 1);
	return 0;
}

// Calculate chromatic positions
void CGenCF1::GetChromatic(vector<int> &c, vector<int> &cc, int step1, int step2) {
	for (int i = step1; i < step2; ++i) {
		cc[i] = C_CC(c[i]);
	}
}

// Search for outstanding repeats
int CGenCF1::FailOutstandingLeap(vector<int> &c, vector<int> &leap, int ep2, vector<int> &flags, vector<vector<int>> &nflags, vector<int> &nflagsc) {
	if (ep2 > 6) for (int i = 0; i < ep2 - 6; ++i) {
		// Check if note changes direction or is a leap
		if ((i == 0) || (leap[i - 1]) || ((c[i] - c[i - 1])*(c[i + 1] - c[i]) < 0)) {
			// Search for repeat of note at same beat until last three notes
			int finish = i + repeat_steps;
			if (finish > ep2 - 2) finish = ep2 - 2;
			for (int x = i + 2; x < finish; x += 2) {
				// Check if same note with direction change or leap
				if ((c[x] == c[i]) && ((leap[x - 1]) || ((c[x] - c[x - 1])*(c[x + 1] - c[x]) < 0))) {
					// Check that two more notes repeat
					if ((c[x + 1] == c[i + 1]) && (c[x + 2] == c[i + 2])) {
						FLAG2(36, i);
					}
				}
			}
		}
	}
	return 0;
}

// Check if too many leaps
int CGenCF1::FailLeapSmooth(int ep2, vector<int> &leap, vector<int> &smooth, vector<int> &flags, vector<vector<int>> &nflags, vector<int> &nflagsc) {
	// Clear variables
	int leap_sum = 0;
	int leap_sum2 = 0;
	int max_leap_sum = 0;
	int max_leap_sum2 = 0;
	int smooth_sum = 0;
	int smooth_sum2 = 0;
	int leap_sum_i = 0;
	int leap_sum_i2 = 0;
	for (int i = 0; i < ep2 - 1; ++i) {
		// Find all leaps
		leap[i] = 0;
		smooth[i] = 0;
		if (c[i + 1] - c[i] > 1) leap[i] = 1;
		else if (c[i + 1] - c[i] < -1) leap[i] = -1;
		// Find all smooth
		else if (c[i + 1] - c[i] == 1) smooth[i] = 1;
		else if (c[i + 1] - c[i] == -1) smooth[i] = -1;
		// Add new leap
		if (leap[i] != 0) {
			++leap_sum;
			++leap_sum2;
		}
		else {
			leap_sum2 = 0;
		}
		// Subtract old leap
		if ((i >= max_leap_steps) && (leap[i - max_leap_steps] != 0)) leap_sum--;
		// Get maximum leap_sum
		if (leap_sum > max_leap_sum) {
			max_leap_sum = leap_sum;
			leap_sum_i = i;
		}
		if (leap_sum2 > max_leap_sum2) {
			max_leap_sum2 = leap_sum2;
			leap_sum_i2 = i;
		}
		// Prohibit long smooth movement
		if (smooth[i] != 0) ++smooth_sum;
		else smooth_sum = 0;
		if (smooth_sum >= max_smooth) FLAG2(4, i);
		if (i < ep2 - 2) {
			// Prohibit long smooth movement in one direction
			if (smooth[i] == smooth[i + 1]) ++smooth_sum2;
			else smooth_sum2 = 0;
			if (smooth_sum2 >= max_smooth_direct - 1) FLAG2(5, i);
			// Check if two notes repeat
			if ((i > 0) && (c[i] == c[i + 2]) && (c[i - 1] == c[i + 1])) FLAG2(9, i);
		}
	}
	if (max_leap_sum > max_leaps) {
		if (max_leap_sum > max_leaps2) FLAG2(25, leap_sum_i)
		else FLAG2(3, leap_sum_i);
	}
	if (max_leap_sum2 > cse_leaps) {
		if (max_leap_sum2 > cse_leaps2) FLAG2(71, leap_sum_i2)
		else FLAG2(70, leap_sum_i2);
	}
	return 0;
}

int CGenCF1::FailStagnation(vector<int> &c, vector<int> &nstat, int nmin, int nmax, int ep2, vector<int> &flags, vector<vector<int>> &nflags, vector<int> &nflagsc) {
	// Clear nstat
	for (int i = nmin; i <= nmax; ++i) {
		nstat[i] = 0;
	}
	// Prohibit stagnation
	for (int i = 0; i < ep2; ++i) {
		// Add new note to stagnation array
		++nstat[c[i]];
		// Subtract old note
		if ((i >= stag_note_steps)) nstat[c[i - stag_note_steps]]--;
		// Check if too many repeating notes
		if (nstat[c[i]] > stag_notes) FLAG2(10, i);
	}
	return 0;
}

// Prohibit multiple culminations
int CGenCF1::FailMultiCulm(vector<int> &c, int ep2, int nmax, vector<int> &flags, vector<vector<int>> &nflags, vector<int> &nflagsc) {
	int culm_sum = 0, culm_step;
	for (int i = 0; i < ep2; ++i) {
		if (c[i] == nmax) {
			++culm_sum;
			culm_step = i;
			if (culm_sum > 1) FLAG2(12, i);
		}
	}
	// Prohibit culminations at last steps
	if (culm_step > c_len - 4) FLAG2(21, culm_step);
	return 0;
}

int CGenCF1::FailFirstNotes(vector<int> &pc, int ep2, vector<int> &flags, vector<vector<int>> &nflags, vector<int> &nflagsc) {
	// Prohibit first note not tonic
	if (pc[0] != 0) {
		FLAG2(49, 0);
		// Calculate steps to search for tonic
		int fst = first_steps_tonic;
		if (c_len > 10) fst = 4;
		// Prohibit tonic miss at start
		int c_pos = -1;
		int e_pos = -1;
		for (int i = 0; i < fst; ++i) {
			// Detect C note
			if (pc[i] == 0) c_pos = i;
			// Detect E note
			if (pc[i] == 2) e_pos = i;
		}
		int ok = 0;
		// No C ?
		if (c_pos == -1) FLAG2(40, 0)
		else {
			// If C found, check previous note
			if (c_pos > 0) {
				if (pc[c_pos - 1] != 6 || pc[c_pos - 1] != 1 || pc[c_pos - 1] == 4) ok = 1;
			}
			// If C is first note, it does not need to be prepared (actually, this cannot happen because of flag 49)
			else ok = 1;
		}
		// No E ?
		if (e_pos == -1) FLAG2(41, 0)
		else {
			// If E found, check previous note
			if (e_pos > 0) {
				if (pc[e_pos - 1] == 1 || pc[e_pos - 1] == 4) ok = 1;
			}
			// If E is first note, it does not need to be prepared
			else ok = 1;
		}
		// Is E or C prepared?
		if (!ok) FLAG2(52, 0);
	}
	return 0;
}

int CGenCF1::FailLastNotes(vector<int> &pc, int ep2, vector<int> &flags, vector<vector<int>> &nflags, vector<int> &nflagsc) {
	// Prohibit first note not tonic
	if (ep2 > c_len - 1)
		if (pc[c_len - 1] != 0) FLAG2(50, c_len - 1); 
	// Wrong second to last note
	if (ep2 > c_len - 2) {
		if ((pc[c_len - 2] == 0) || (pc[c_len - 2] == 2) || (pc[c_len - 2] == 3) || (pc[c_len - 2] == 5)) FLAG2(13, c_len - 2);
		if (pc[c_len - 2] == 4) FLAG2(51, c_len - 2);
	}
	// Wrong third to last note
	if (ep2 > c_len - 3) {
		if ((pc[c_len - 3] == 0) || (pc[c_len - 3] == 2) || (pc[c_len - 3] == 4)) FLAG2(14, c_len - 3);
		// Leading third to last note
		if (pc[c_len - 3] == 6) FLAG2(34, c_len - 3);
	}
	return 0;
}

void CGenCF1::CountFill(int i, int pos1, int pos2, int leap_size, int leap_start, vector<int> &nstat2, vector<int> &nstat3, int &skips, int &skips2)
{
	if (pos2 < pos1) pos2 = pos1;
	// Clear stat
	int c1 = min(c[leap_start], c[i + 1]);
	int c2 = max(c[leap_start], c[i + 1]);
	for (int x = c1 + 1; x < c2; ++x) {
		nstat3[x] = 0;
	}
	// Fill all notes (even those outside pos1-pos2 window)
	for (int x = pos1; x <= pos2; ++x) {
		++nstat3[c[x]];
	}
	// Local fill
	skips = 0; 
	// Add allowed skips
	if (leap_size > 3) --skips;
	if (leap_size > 6) --skips;
	// Global fill
	skips2 = skips;
	for (int x = c1 + 1; x < c2; ++x) if (!nstat3[x]) {
		++skips;
		if (!nstat2[x]) ++skips2;
	}
}

int CGenCF1::FailLeap(int ep2, vector<int> &leap, vector<int> &smooth, vector<int> &nstat2, vector<int> &nstat3, vector<int> &flags, vector<vector<int>> &nflags, vector<int> &nflagsc)
{
	int preleap, leap_size, leap_start, leap_next, leap_prev, unresolved, prefilled, skips, skips2, pos;
	for (int i = 0; i < ep2 - 1; ++i) {
		if (leap[i] != 0) {
			// Check if this leap is 3rd
			leap_size = abs(c[i + 1] - c[i]);
			leap_start = i;
			preleap = 0;
			prefilled = 0;
			leap_next = 0;
			leap_prev = 0;
			// Prev is leap?
			if (i > 0) leap_prev = leap[i] * leap[i - 1];
			// Check preleap (current leap does not exceed previous close leap)
			if ((i > 0) && ((c[i - 1] - c[i + 1])*leap[i] > 0)) preleap = 1;
			else if ((i > 1) && ((c[i - 2] - c[i + 1])*leap[i] > 0)) preleap = 1;
			else if ((i > 2) && ((c[i - 3] - c[i + 1])*leap[i] > 0)) preleap = 1;
			// Check if leap is third
			if (leap_size == 2) {
				// Check if leap is second third
				if (i > 0 && abs(c[i + 1] - c[i - 1]) == 4) {
					// Set leap start to first note of first third
					leap_start = i - 1;
					// Set leap size to be compound
					leap_size = 4;
					// If 6/8 goes before 2 thirds
					if ((i > 1) && ((leap[i] * (c[i - 1] - c[i - 2]) == -5) || (leap[i] * (c[i - 1] - c[i - 2]) == -7))) FLAG2(28, i)
						// Else mark simple 2x3rds
					else FLAG2(6, i);
				}
			}
			// Check if we have a greater neighbouring leap
			if ((i < ep2 - 2 && abs(c[i + 2] - c[i + 1]) > leap_size) ||
				(leap_start > 0 && abs(c[leap_start] - c[leap_start - 1]) > leap_size)) {
				// Set that we are preleaped (even if we are postleaped)
				preleap = 1;
			}
			if (i > 0) {
				// Check if  leap is prefilled
				pos = i - 2 - (leap_size - 1) * fill_steps_mul;
				if (pos < 0) pos = 0;
				CountFill(i, pos, i - 1, leap_size, leap_start, nstat2, nstat3, skips, skips2);
				if (skips <= 0) prefilled = 1;
			}
			if (i < ep2 - 2) {
				// Check if  leap is filled
				pos = i + 2 + (leap_size - 1) * fill_steps_mul;
				// Do not check fill if search window is cut by end of current not-last scan window
				if ((pos < ep2) || (c_len == ep2)) {
					if (pos > ep2 - 1) pos = ep2 - 1;
					CountFill(i, i + 2, pos, leap_size, leap_start, nstat2, nstat3, skips, skips2);
					// Local not filled?
					if (skips > 0) {
						// Local not filled. Prefilled?
						if (prefilled) {
							if (leap_size == 2) FLAG2(61, i)
							else if (leap_size == 3) FLAG2(62, i)
							else FLAG2(63, i);
						}
						// Local not filled. Not prefilled. Preleaped?
						else if (preleap) {
							if (leap_size == 2) FLAG2(35, i)
							else if (leap_size == 3) FLAG2(64, i)
							else FLAG2(65, i);
						}
						// Local not filled. Not prefilled. Not preleaped. Global filled?
						else if (skips2 <= 0) {
							if (leap_size == 2) FLAG2(66, i)
							else if (leap_size == 3) FLAG2(67, i)
							else FLAG2(11, i);
						}
						// Local not filled. Not prefilled. Not preleaped. Global not filled.
						else {
							if (leap_size == 2) FLAG2(68, i)
							else if (leap_size == 3) FLAG2(69, i)
							else FLAG2(24, i);
						}
					}
				}
			}
			// Check leap resolution if it is not last note
			if (i < ep2 - 2) {
				leap_next = leap[i] * leap[i + 1];
				// Next leap in same direction
				if (leap_next > 0) {
					// Flag if greater than two thirds
					if (abs(c[i + 2] - c[i]) > 4) FLAG2(27, i + 2);
					// Allow if both thirds, without flags (will process next cycle)
				}
				// Next leap back
				else if (leap_next < 0) {
					// Flag if back leap greater than 6th
					int leap_size2 = abs(c[i + 2] - c[i + 1]);
					if (leap_size2 > 5) FLAG2(22, i + 1)
						// Flag if back leap equal or smaller than 6th
					else FLAG2(8, i + 1);
					if (leap_size2 > leap_size) FLAG2(58, i + 1);
				}
				// Next linear in same direction
				else if (leap[i] * (c[i + 2] - c[i + 1]) > 0) {
					// Flag if 2nd after next back
					unresolved = 0;
					if (i < ep2 - 3) {
						// Check if melody direction does not change second note after leap
						if (leap[i] * (c[i + 3] - c[i + 2]) > 0) unresolved = 1;
						// If direction changes second note after leap
						else {
							// Check leap size
							if (leap_size > 4) FLAG2(29, i)
							else FLAG2(7, i);
						}
					}
					else {
						// Mark leap unresolved if this is end of cantus
						if (c_len == ep2) unresolved = 1;
					}
					if (unresolved) {
						if (leap_size == 2) {
							// Flag if third preleaped unresolved
							if (preleap) FLAG2(30, i)
								// Flag if third prefilled unresolved
							else if (prefilled) FLAG2(53, i)
								// Flag if third unresolved
							else FLAG2(54, i);
						}
						else if (leap_size == 3) {
							// Flag if 4th preleaped unresolved
							if (preleap) FLAG2(55, i)
								// Flag if 4th prefilled unresolved
							else if (prefilled) FLAG2(56, i)
								// Flag if 4th unresolved
							else FLAG2(57, i);
						}
						else {
							// Flag if >4th preleaped unresolved
							if (preleap) FLAG2(59, i)
								// Flag if >4th prefilled unresolved
							else if (prefilled) FLAG2(60, i)
								// Flag if >4th unresolved
							else FLAG2(26, i);
						}
					} // if unresolved
				}
				// Next linear back - no flag
			}
		}
	}
	// Prohibit last leap
	if (ep2 == c_len)
		if (leap[c_len - 2]) FLAG2(23, c_len - 1);
	return 0;
}

int CGenCF1::FailIntervals(int ep2, int nmax, vector<int> &pc, vector<int> &flags, vector<vector<int>> &nflags, vector<int> &nflagsc)
{
	int leap_start;
	int found;
	for (int i = 0; i < ep2 - 1; ++i) {
		// Tritone prohibit
		leap_start = i;
		found = 0;
		// Check consecutive tritone
		if ((pc[i + 1] == 6 && pc[i] == 3) || (pc[i + 1] == 3 && pc[i] == 6)) found = 1;
		// Check tritone with additional note inside
		if (i > 0) {
			if ((pc[i + 1] == 6 && pc[i - 1] == 3) || (pc[i + 1] == 3 && pc[i - 1] == 6)) {
				found = 1;
				leap_start = i - 1;
			}
		}
		if (found) {
			// Check if tritone is highest leap if this is last window
			if (ep2 == c_len)
				if ((c[leap_start] == nmax) || (c[i + 1] == nmax)) FLAG2(32, i)
					// Check if tritone is last step
					if (i > c_len - 3) FLAG2(31, i)
					// Check if resolution is correct
					else if (i < ep2 - 2) {
						if (pc[i + 1] == 3) FLAG2(31, i)
						else if (pc[i + 2] != 0) FLAG2(31, i)
						else if (i > 0 && pc[leap_start - 1] != 2) FLAG2(31, i)
							// Record resolved tritone
						else FLAG2(2, i);
					}
			// Do not check tritone if it is at the end of not-last window (after ep2 - 2)
		}
		// Sept prohibit
		if (abs(cc[i + 1] - cc[i]) == 10) FLAG2(1, i)
		else if (abs(cc[i + 1] - cc[i]) == 11) FLAG2(39, i);
	}
	return 0;
}

// Calculate global fill
void CGenCF1::GlobalFill(int ep2, vector<int> &nstat2)
{
	for (int x = 0; x < ep2; ++x) nstat2[x] = 0;
	for (int x = 0; x < ep2; ++x) ++nstat2[c[x]];
}

void CGenCF1::ScanCantus(vector<int> *pcantus, int use_matrix, int v) {
	// Get cantus size
	if (pcantus) c_len = pcantus->size();
	// Resize global vectors
	c.resize(c_len); // cantus (diatonic)
	cc.resize(c_len); // cantus (chromatic)
	nflags.resize(c_len, vector<int>(MAX_FLAGS)); // Flags for each note
	nflagsc.resize(c_len); // number of flags for each note
	// Local variables
	CString st, st2;
	int wid; // Window id
	int seed_cycle = 0; // Number of cycles in case of random_seed
	vector<int> c2(c_len); // Cantus diatonic saved for SWA
	vector<int> pc(c_len); // pitch class
	vector<int> leap(c_len);
	vector<int> smooth(c_len);
	vector<int> wpos1(c_len / s_len + 1);
	vector<int> wpos2(c_len / s_len + 1);
	vector<int> nstat(MAX_NOTE);
	vector<int> nstat2(MAX_NOTE);
	vector<int> nstat3(MAX_NOTE);
	vector<long long> wscans(MAX_WIND); // number of full scans per window
	vector<long long> accepted4(MAX_WIND); // number of accepted canti per window
	vector<long long> accepted5(MAX_WIND); // number of canti with neede flags per window
	vector<long long> fstat(MAX_FLAGS); // number of canti with each flag
	vector<vector<vector<long>>> fblock; // number of canti rejected with foreign flags
	vector<int>  flags(MAX_FLAGS); // Flags for whole cantus
	vector<vector<long long>> fcor(MAX_FLAGS, vector<long long>(MAX_FLAGS)); // Flags correlation matrix
	vector <int> smap; // Map of links from matrix local IDs to cantus step IDs
	skip_flags = !calculate_blocking && !calculate_correlation && !calculate_stat;
	long long cycle = 0;
	long long accepted2 = 0, accepted3 = 0;
	int first_note_dia, first_note_oct;
	int finished = 0;
	int nmin, nmax, culm_sum, culm_step, smooth_sum, smooth_sum2, pos, ok, ok2;
	int dcount, scount, tcount, wdcount, wscount, wtcount;
	int sp1, sp2, ep1, ep2, p, pp;
	int wcount = 1; // Number of windows created
	vector<int> min_c(c_len);
	vector<int> max_c(c_len);
	accepted = 0;
	// Initialize fblock if calculation is needed
	if (calculate_blocking) {
		fblock = vector<vector<vector<long>>> (MAX_WIND, vector<vector<long>>(MAX_FLAGS, vector<long>(MAX_FLAGS)));
	}
	// Analyze single cantus
	if (pcantus) {
		// Copy cantus
		cc = *pcantus;
		// Get diatonic steps from chromatic
		first_note = cc[0];
		last_note = cc[c_len - 1];
		first_note_dia = chrom_to_dia[(first_note % 12 + 12 - tonic_cur) % 12];
		first_note_oct = first_note / 12;
		for (int i = 0; i < c_len; ++i) {
			c[i] = CC_C(cc[i]);
			// Save value for future use;
			c2[i] = c[i];
			// Check duplicate
			if (i > 0 && c[i] == c[i - 1]) return;
			// Set pitch limits
			min_c[i] = c[i] - correct_range;
			max_c[i] = c[i] + correct_range;
		}
		sp1 = 1;
		sp2 = c_len - 1;
		ep1 = sp1;
		ep2 = c_len;
		// Clear flags
		++accepted3;
		fill(flags.begin(), flags.end(), 0);
		flags[0] = 1;
		for (int i = 0; i < ep2; ++i) {
			nflagsc[i] = 0;
		}
		// Matrix scan
		if (use_matrix) {
			// Exit if no violations
			if (!smatrixc) return;
			// Create map
			smap.resize(smatrixc);
			int map_id = 0;
			for (int i = 0; i < c_len; ++i) if (smatrix[i]) {
				smap[map_id] = i;
				++map_id;
			}
			sp1 = 0;
			sp2 = sp1 + s_len; // End of search window
			if (sp2 > smatrixc) sp2 = smatrixc;
			// Record window
			wid = 0;
			wpos1[wid] = sp1;
			wpos2[wid] = sp2;
			// Add last note if this is last window
			// End of evaluation window
			ep1 = smap[sp1];
			if (use_matrix == 1) {
				ep2 = smap[sp2 - 1] + 1;
				if (sp2 == smatrixc) ep2 = c_len;
				// Clear scan steps
				FillCantusMap(c, smap, 0, smatrixc, min_c);
				// Can skip flags - full scan must remove all flags
			}
			// For sliding windows algorithm evaluate whole melody
			if (use_matrix == 2) {
				ep2 = c_len;
				// Cannot skip flags - need them for penalty if cannot remove all flags
				skip_flags = 0;
				// Clear scan steps of current window
				FillCantusMap(c, smap, sp1, sp2, min_c);
			}
			// Minimal position in array to cycle
			pp = sp2 - 1;
			p = smap[pp]; 
		}
		else {
			// For single cantus scan - cannot skip flags - must show all
			skip_flags = 0;
		}
	}
	// Full scan canti
	else {
		// Check that at least one rule accepted
		for (int i = 0; i < MAX_FLAGS; ++i) {
			if (accept[i]) break;
			if (i == MAX_FLAGS - 1) WriteLog(1, "Warning: all rules are rejected (0) in configuration file");
		}
		first_note_dia = chrom_to_dia[(first_note % 12 + 12 - tonic_cur) % 12];
		first_note_oct = first_note / 12;
		// Set first and last notes
		c[0] = CC_C(first_note);
		c[c_len - 1] = CC_C(last_note);
		// Set pitch limits
		for (int i = 0; i < c_len; ++i) {
			min_c[i] = c[0] - max_interval;
			max_c[i] = c[0] + max_interval;
		}
		// Set middle notes to minimum
		FillCantus(c, 1, c_len-1, min_c[0]);
		if (random_seed)
			for (int i = 1; i < c_len - 1; ++i) c[i] = -randbw(min_c[0], max_c[0]);
		sp1 = 1; // Start of search window
		sp2 = sp1 + s_len; // End of search window
		if (sp2 > c_len - 1) sp2 = c_len - 1;
		// Record window
		wid = 0;
		wpos1[wid] = sp1;
		wpos2[wid] = sp2;
		// Add last note if this is last window
		ep1 = sp1;
		ep2 = sp2; // End of evaluation window
		if (ep2 == c_len - 1) ep2 = c_len;
		p = sp2 - 1; // Minimal position in array to cycle
	}
	// Check if too many windows
	if (((c_len - 2) / (float)s_len > MAX_WIND && !pcantus) || (pcantus && use_matrix == 1 && smatrixc/s_len > MAX_WIND)) {
		CString* est = new CString;
		est->Format("Error: generating %d notes with search window %d requires more than %d windows. Change MAX_WIND to allow more.",
			c_len, s_len, MAX_WIND);
		WriteLog(1, est);
		return;
	}
	// Analyze combination
check:
	while (true) {
		//LogCantus(c);
		if (FailNoteRepeat(c, ep1-1, ep2-1)) goto skip;
		if ((need_exit) && (!pcantus || use_matrix)) break;
		GetMelodyInterval(c, 0, ep2, nmin, nmax);
		++accepted3;
		// Limit melody interval
		if (pcantus) {
			ClearFlags(flags, nflagsc, 0, ep2);
			if (nmax - nmin > max_interval) FLAG(37, 0);
			if (nmax - nmin < min_interval) FLAG(38, 0);
		}
		else {
			if (nmax - nmin > max_interval) goto skip;
			if (nmax - nmin < min_interval) goto skip;
			ClearFlags(flags, nflagsc, 0, ep2);
		}
		GetPitchClass(c, pc, 0, ep2);
		if (FailLastNotes(pc, ep2, flags, nflags, nflagsc)) goto skip;
		if (FailMelodyHarmSeq(pc, 0, ep2, flags, nflags, nflagsc)) goto skip;
		if (FailMelodyHarmSeq2(pc, 0, ep2, flags, nflags, nflagsc)) goto skip;
		if (FailNoteSeq(pc, 0, ep2, flags, nflags, nflagsc)) goto skip;
		GetChromatic(c, cc, 0, ep2);
		if (FailIntervals(ep2, nmax, pc, flags, nflags, nflagsc)) goto skip;
		if (FailLeapSmooth(ep2, leap, smooth, flags, nflags, nflagsc)) goto skip;
		if (FailOutstandingLeap(c, leap, ep2, flags, nflags, nflagsc)) goto skip;
		GlobalFill(ep2, nstat2);
		if (FailLeap(ep2, leap, smooth, nstat2, nstat3, flags, nflags, nflagsc)) goto skip;
		if (FailStagnation(c, nstat, nmin, nmax, ep2, flags, nflags, nflagsc)) goto skip;
		if (FailMultiCulm(c, ep2, nmax, flags, nflags, nflagsc)) goto skip;
		if (FailFirstNotes(pc, ep2, flags, nflags, nflagsc)) goto skip;

		if ((!pcantus) || (use_matrix == 1)) {
			++accepted2;
			// Calculate flag statistics
			if (calculate_stat || calculate_correlation) {
				if (ep2 == c_len) for (int i = 0; i < MAX_FLAGS; ++i) {
					if (flags[i]) {
						++fstat[i];
						// Calculate correlation
						if (calculate_correlation) for (int z = 0; z < MAX_FLAGS; ++z) {
							if (flags[z]) ++fcor[i][z];
						}
					}
				}
			}
			// Calculate flag blocking
			if (calculate_blocking) {
				int flags_found = 0;
				int flags_found2 = 0;
				int flags_conflict = 0;
				// Find if any of accepted flags set
				for (int i = 0; i < MAX_FLAGS; ++i) {
					if ((flags[i]) && (accept[i])) ++flags_found;
					if ((flags[i]) && (!accept[i])) ++flags_conflict;
					if ((flags[i]) && (accept[i] == 2)) ++flags_found2;
				}
				// Skip only if flags required
				if ((!late_require) || (ep2 == c_len)) {
					// Check if no needed flags set
					if (flags_found == 0) goto skip;
					// Check if not enough 2 flags set
					if (flags_found2 < flags_need2) goto skip;
				}
				++accepted5[wid];
				// Find flags that are blocking
				for (int i = 0; i < MAX_FLAGS; ++i) {
					if ((flags[i]) && (!accept[i]))
						++fblock[wid][flags_conflict][i];
				}
			}
			// Check if flags are accepted
			for (int i = 0; i < MAX_FLAGS; ++i) {
				if ((flags[i]) && (!accept[i])) goto skip;
				if ((!late_require) || (ep2 == c_len))
					if ((!flags[i]) && (accept[i] == 2)) goto skip;
			}
			++accepted4[wid];
			// If this is not last window, go to next window
			if (ep2 < c_len) {
				if (use_matrix) {
					sp1 = sp2;
					sp2 = sp1 + s_len; // End of search window
					if (sp2 > smatrixc) sp2 = smatrixc;
					// Reserve last window with maximum length
					if ((smatrixc - sp1 < s_len * 2) && (smatrixc - sp1 > s_len)) sp2 = (smatrixc + sp1) / 2;
					// Record window
					++wid;
					wpos1[wid] = sp1;
					wpos2[wid] = sp2;
					++wscans[wid];
					// Add last note if this is last window
					// End of evaluation window
					ep1 = smap[sp1];
					ep2 = smap[sp2 - 1] + 1;
					if (sp2 == smatrixc) ep2 = c_len;
					// Minimal position in array to cycle
					pp = sp2 - 1;
					p = smap[pp];
				}
				else {
					sp1 = sp2;
					sp2 = sp1 + s_len; // End of search window
					if (sp2 > c_len - 1) sp2 = c_len - 1;
					// Reserve last window with maximum length
					if ((c_len - sp1 - 1 < s_len * 2) && (c_len - sp1 - 1 > s_len)) sp2 = (c_len + sp1) / 2;
					// Record window
					++wid;
					wpos1[wid] = sp1;
					wpos2[wid] = sp2;
					++wscans[wid];
					// End of evaluation window
					ep1 = sp1;
					ep2 = sp2;
					// Add last note if this is last window
					if (ep2 == c_len - 1) ep2 = c_len;
					// Go to rightmost element
					p = sp2 - 1;
				}
				if (wcount < wid + 1) {
					wcount = wid + 1;
					if (ep2 == c_len) {
						// Show window statistics
						CString* est = new CString;
						CString st, st2;
						for (int i = 0; i < wcount; ++i) {
							if (i > 0) st2 += ", ";
							st.Format("%d-%d", wpos1[i], wpos2[i]);
							st2 += st;
						}
						est->Format("Algorithm created %d windows: %s", wcount, st2);
						WriteLog(3, est);
					}
				}
				// Clear minimax so that it is recalculated
				nmin = 0;
				nmax = 0;
				goto skip;
			}
			// Check random_choose
			if (random_choose < 100) if (rand2() >= (float)RAND_MAX*random_choose / 100.0) goto skip;
		}
		// Calculate rules penalty if we analyze cantus without full scan
		if (pcantus && (use_matrix == 2 || !use_matrix)) {
			rpenalty_cur = 0;
			for (int x = 0; x < ep2; ++x) {
				if (nflagsc[x] > 0) for (int i = 0; i < nflagsc[x]; ++i) if (!accept[nflags[x][i]]) {
					rpenalty_cur += flag_to_sev[nflags[x][i]];
				}
			}
		}
		// Accept cantus
		++accepted;
		if (use_matrix == 1) {
			//LogCantus(c);
			SaveCantus();
		}
		else if (use_matrix == 2) {
			// Is penalty not greater than minimum of all previous?
			if (rpenalty_cur <= rpenalty_min) {
				// If rpenalty 0, we can skip_flags (if allowed)
				if (!skip_flags && rpenalty_cur == 0) 
					skip_flags = !calculate_blocking && !calculate_correlation && !calculate_stat;
				SaveCantus();
			}
		}
		else {
			SendCantus(v, pcantus);
			if ((pcantus) && (!use_matrix)) return;
		}
	skip:
		while (true) {
			if (c[p] < max_c[p]) break;
			// If current element is max, make it minimum
			c[p] = min_c[p];
			// Move left one element
			if (use_matrix) {
				if (pp == sp1) {
					finished = 1;
					break;
				}
				pp--;
				p = smap[pp];
			}
			else {
				if (p == sp1) {
					finished = 1;
					break;
				}
				p--;
			}
		} // while (true)
		if (finished) {
			// Sliding Windows Approximation
			if (use_matrix == 2) {
				// If we slided to the end, break
				if (sp2 == smatrixc) break;
				// Slide window further
				++sp1;
				++sp2;
				ep1 = smap[sp1];
				// Minimal position in array to cycle
				pp = sp2 - 1;
				p = smap[pp];
				// Restore previous step after sliding window
				c[smap[sp1 - 1]] = c2[smap[sp1 - 1]];
				// Clear scan steps of current window
				FillCantusMap(c, smap, sp1, sp2, min_c);
			}
			// Finish if this is last variant in first window and not SWA
			else if ((p == 1) || (wid == 0)) {
				// If we started from random seed, allow one more full cycle
				if (random_seed) {
					if (seed_cycle) break;
					WriteLog(3, "Random seed allows one more full cycle: restarting");
					++seed_cycle;
				}
				else break;
			}
			if (use_matrix == 1) {
				// Clear current window
				FillCantusMap(c, smap, sp1, sp2, min_c);
				// If this is not first window, go to previous window
				if (wid > 0) wid--;
				sp1 = wpos1[wid];
				sp2 = wpos2[wid];
				// End of evaluation window
				ep1 = smap[sp1];
				ep2 = smap[sp2 - 1] + 1;
				if (sp2 == smatrixc) ep2 = c_len;
				// Minimal position in array to cycle
				pp = sp2 - 1;
				p = smap[pp];
			}
			// Normal full scan
			else if (!use_matrix) {
				// Clear current window
				FillCantus(c, sp1, sp2, min_c[0]);
				// If this is not first window, go to previous window
				if (wid > 0) wid--;
				sp1 = wpos1[wid];
				sp2 = wpos2[wid];
				// End of evaluation window
				ep1 = sp1;
				ep2 = sp2;
				// Add last note if this is last window
				if (ep2 == c_len - 1) ep2 = c_len;
				// Go to rightmost element
				p = sp2 - 1;
			}
			// Clear flag to prevent coming here again
			finished = 0;
			// Clear minimax so that it is recalculated
			nmin = 0;
			nmax = 0;
			// Goto next variant calculation
			goto skip;
		} // if (finished)
		// Increase rightmost element, which was not reset to minimum
		++c[p];
		// Go to rightmost element
		if (use_matrix) {
			pp = sp2 - 1;
			p = smap[pp];
		}
		else {
			p = sp2 - 1;
		}
		++cycle;
	}
	// Write flag correlation
	if (calculate_correlation) {
		DeleteFile("cf1-cor.csv");
		CString st, st2, st3;
		st3 = "Flag; Total; ";
		for (int i = 0; i < MAX_FLAGS; ++i) {
			int f1 = sev_to_flag[i];
			st2.Format("%s; %d; ", FlagName[f1], fcor[f1][f1]);
			st3 += FlagName[f1] + "; ";
			for (int z = 0; z < MAX_FLAGS; ++z) {
				int f2 = sev_to_flag[z];
				st.Format("%lld; ", fcor[f1][f2]);
				st2 += st;
			}
			CGLib::AppendLineToFile("cf1-cor.csv", st2 + "\n");
		}
		CGLib::AppendLineToFile("cf1-cor.csv", st3 + "\n");
	}
	// Show flag statistics
	if (calculate_stat) {
		CString* est = new CString;
		for (int i = 0; i < MAX_FLAGS; ++i) {
			int f1 = sev_to_flag[i];
			st.Format("\n%lld %s ", fstat[f1], FlagName[f1]);
			st2 += st;
		}
		est->Format("%d/%d: Accepted %lld/%lld/%lld/%lld variants of %lld: %s",
			c_len, max_interval, accepted4[wcount - 1], accepted, accepted2,
			accepted3, cycle, st2);
		WriteLog(3, est);
	}
	// Show blocking statistics
	if (calculate_blocking) {
		for (int w = 0; w < wcount; ++w) {
			int lines = 0;
			CString* est = new CString;
			st2 = "";
			for (int d = 1; d < MAX_FLAGS; ++d) {
				if (lines > 100) break;
				int flagc = 0;
				for (int x = 0; x < MAX_FLAGS; ++x) {
					if (fblock[w][d][x] > 0) ++flagc;
				}
				if (!flagc) continue;
				int max_flag = 0;
				long max_value = -1;
				st.Format("\nTIER %d: ", d);
				st2 += st;
				for (int x = 0; x < MAX_FLAGS; ++x) {
					max_value = -1;
					// Find biggest value
					for (int i = 0; i < MAX_FLAGS; ++i) {
						if (fblock[w][d][i] > max_value) {
							max_value = fblock[w][d][i];
							max_flag = i;
						}
					}
					if (max_value < 1) break;
					st.Format("\n%ld %s, ", max_value, FlagName[max_flag]);
					st2 += st;
					++lines;
					// Clear biggest value to search for next
					fblock[w][d][max_flag] = -1;
				}
			}
			est->Format("Window %d: %lld scans, %lld of %lld variants blocked: %s", w, wscans[w], accepted5[w] - accepted4[w], accepted5[w], st2);
			WriteLog(3, est);
		}
	}
}

void CGenCF1::SaveCantus() {
	clib.push_back(cc);
	rpenalty.push_back(rpenalty_cur);
	rpenalty_min = rpenalty_cur;
}

void CGenCF1::SendCantus(int v, vector<int> *pcantus) {
	CString st;
	Sleep(sleep_ms);
	// Copy cantus to output
	int pos = step;
	if (step + real_len >= t_allocated) ResizeVectors(t_allocated * 2);
	for (int x = 0; x < c_len; ++x) {
		for (int i = 0; i < cc_len[x]; ++i) {
			// Set flag color
			color[pos+i][v] = FlagColor[0];
			int current_severity = -1;
			// Set nflag color
			note[pos + i][v] = cc[x];
			tonic[pos + i][v] = tonic_cur;
			if (nflagsc[x] > 0) for (int f = 0; f < nflagsc[x]; ++f) {
				if (!i) {
					comment[pos + i][v] += FlagName[nflags[x][f]];
					st.Format(" [%d]", flag_to_sev[nflags[x][f]]);
					if (show_severity) comment[pos + i][v] += st;
					comment[pos + i][v] += ". ";
				}
				// Set note color if this is maximum flag severity
				if (flag_to_sev[nflags[x][f]] > current_severity) {
					current_severity = flag_to_sev[nflags[x][f]];
					color[pos + i][v] = flag_color[nflags[x][f]];
				}
			}
			len[pos + i][v] = cc_len[x];
			pause[pos + i][v] = 0;
			coff[pos + i][v] = i;
			if (x < real_len / 2)	dyn[pos + i][v] = 60 + 40 * (pos + i - step) / real_len + 20 * rand2() / RAND_MAX;
			else dyn[pos + i][v] = 60 + 40 * (real_len - pos - i + step) / real_len + 20 * rand2() / RAND_MAX;
			// Assign source tempo if exists
			if (cc_tempo[x]) {
				tempo[pos + i] = cc_tempo[x];
			}
			// Generate tempo if no source
			else {
				if (pos + i == 0) {
					tempo[pos + i] = min_tempo + (float)(max_tempo - min_tempo) * (float)rand2() / (float)RAND_MAX;
				}
				else {
					tempo[pos + i] = tempo[pos + i - 1] + randbw(-1, 1);
					if (tempo[pos + i] > max_tempo) tempo[pos + i] = 2 * max_tempo - tempo[pos + i];
					if (tempo[pos + i] < min_tempo) tempo[pos + i] = 2 * min_tempo - tempo[pos + i];
				}
			}
		}
		pos += cc_len[x];
	}
	// Create pause
	step = pos;
	note[step][v] = 0;
	len[step][v] = 1;
	pause[step][v] = 1;
	dyn[step][v] = 0;
	tempo[step] = tempo[step - 1];
	coff[step][v] = 0;
	++step;
	// Count additional variables
	CountOff(step - real_len - 1, step - 1);
	CountTime(step - real_len - 1, step - 1);
	UpdateNoteMinMax(step - real_len - 1, step - 1);
	UpdateTempoMinMax(step - real_len - 1, step - 1);
	if (!pcantus) {
		if (!shuffle) {
			Adapt(step - real_len - 1, step - 1);
		}
	}
	// Send
	t_generated = step;
	if (!pcantus) {
		if (!shuffle) {
			t_sent = t_generated;
		}
	}
}

void CGenCF1::InitCantus()
{
	// Check all flags severity loaded
	if (cur_severity < MAX_FLAGS) {
		for (int i = 1; i < MAX_FLAGS; ++i) {
			if (!flag_to_sev[i]) {
				if (cur_severity == MAX_FLAGS) {
					CString* est = new CString;
					est->Format("Warning: more flags in config than in algorithm. Possibly duplicate flags inc config. Please correct config %s", m_config);
					WriteLog(1, est);
				}
				else {
					sev_to_flag[cur_severity] = i;
					flag_to_sev[i] = cur_severity;
					// Log
					CString* est = new CString;
					est->Format("Warning: flag '%s' not found in config %s. Assigning severity %d to flag. Please add flag to file", FlagName[i], m_config, cur_severity);
					WriteLog(1, est);
					++cur_severity;
				}
			}
		}
	}
	// Global step
	step = 0;
	// Calculate second level flags count
	for (int i = 0; i < MAX_FLAGS; ++i) {
		if (accept[i] == 2) ++flags_need2;
	}
	// Set priority
	for (int i = 0; i < MAX_FLAGS; ++i) {
		flag_to_sev[sev_to_flag[i]] = i;
		flag_color[sev_to_flag[i]] = Color(0, 255.0 / MAX_FLAGS*i, 255 - 255.0 / MAX_FLAGS*i, 0);
	}
}

void CGenCF1::Generate()
{
	// Voice
	int v = 0;
	InitCantus();
	// Set uniform length
	cc_len.resize(c_len);
	cc_tempo.resize(c_len);
	real_len = c_len;
	for (int i = 0; i < c_len; ++i) cc_len[i] = 1;
	ScanCantus(0, 0, 0);
	// Random shuffle
	if (shuffle) {
		vector<unsigned short> ci(accepted); // cantus indexes
		vector<unsigned char> note2(t_generated);
		vector<CString> comment2(t_generated);
		vector<Color> color2(t_generated);
		for (int i = 0; i < accepted; ++i) ci[i] = i;
		// Shuffled indexes
		unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
		::shuffle(ci.begin(), ci.end(), default_random_engine(seed));
		// Swap
		int i1, i2;
		for (int i = 0; i < accepted; ++i) {
			for (int x = 0; x < c_len; ++x) {
				i1 = i*(c_len + 1) + x;
				i2 = ci[i]*(c_len + 1) + x;
				note2[i1] = note[i2][v];
				comment2[i1] = comment[i2][v];
				color2[i1] = color[i2][v];
			}
		}
		// Replace
		for (int i = 0; i < accepted; ++i) {
			for (int x = 0; x < c_len; ++x) {
				i1 = i*(c_len + 1) + x;
				note[i1][v] = note2[i1];
				comment[i1][v] = comment2[i1];
				color[i1][v] = color2[i1];
			}
		}
		// Adapt
		Adapt(0, t_generated-1);
		// Send
		t_sent = t_generated;
		::PostMessage(m_hWnd, WM_GEN_FINISH, 2, 0);
		CString* est = new CString;
		est->Format("Shuffle of %lld melodies finished", accepted);
		WriteLog(3, est);
	}
}