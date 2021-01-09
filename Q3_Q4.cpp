#include <iostream>
#include <chrono> 
#include <algorithm>    // std::sort


// Information about processor
int NWINDOWS;
int num_reg_per_window;

// Information about executing the software on RISC (in this case compute_pascal())
int num_procedure_calls;
int current_window_depth;
int max_window_depth;
int num_overflows;
int num_underflows;

// For determining how many empty windows must be available for an overflow to occur,
// this value would be 1 or 0 for the two parts of Q3
int num_empty_windows_for_overflow;

struct psw {
	int cwp;
	int swp;
	int WUSED;
};

struct psw * create_psw() {
	psw * pointer = (psw*) malloc(sizeof(psw));
	(*pointer).cwp = 0;
	// (*pointer).swp = 0;
	(*pointer).swp = 1;
	// (*pointer).WUSED = 0;
	(*pointer).WUSED = 2;
	return pointer;
}

int mod(int a, int b) {
	return (a % b + b) % b;
}

void modify_stack() {
	// Turns out this wasn't necessary, since the program runs much slower
	// for a lower number of register windows anyway.

	// this function is used to artifically inflate the duration
	// of the program, simulating the process of pushing or popping
	// all registers on the stack when an overflow or underflow occurs

	//for (int i = 0; i < num_reg_per_window; i++) {}
	return;
}

void update_depth(int value) {
	current_window_depth += value;
	if (current_window_depth > max_window_depth) max_window_depth = current_window_depth;
}

void call_function_risc(psw * psw) {
	num_procedure_calls++;
	update_depth(1);

	if ((*psw).WUSED == NWINDOWS - num_empty_windows_for_overflow) {
		modify_stack();		// push oldest reigster window to stack
		(*psw).swp = mod((*psw).swp + 1, NWINDOWS);	// SWP++ (mod NWINDOWS)
		num_overflows++;
	}
	else (*psw).WUSED++;			// WUSED++

	(*psw).cwp = mod((*psw).cwp + 1, NWINDOWS);		// CWP++ (mod NWINDOWS)
	return;
}

void return_function_risc(psw * psw) {
	update_depth(-1);

	if ((*psw).WUSED == 2) {
		(*psw).swp = mod((*psw).swp - 1,NWINDOWS);	// SWP--
		modify_stack();				// pop register window from stack
		num_underflows++;
	}
	else (*psw).WUSED--;

	(*psw).cwp = mod((*psw).cwp-1, NWINDOWS);		// CWP-- (mod NWINDOWS)
	return;
}

int compute_pascal_C(int row, int position) {
	if (position == 1) return 1;
	else if (position == row) return 1;
	else return compute_pascal_C(row - 1, position) + compute_pascal_C(row - 1, position - 1);
}

int compute_pascal_RISC(int row, int position, psw * ourPSW) {
	call_function_risc(ourPSW);		// for modifying CWP/SWP when we call a function in RISC
	if (position == 1) {
		return_function_risc(ourPSW);	// for recognising that we have returned from this function in RISC
		return 1;
	}
	else if (position == row) {
		return_function_risc(ourPSW);   
		return 1;
	}
	else {
		// the following is equivalent to the line
		// else return compute_pascal(row - 1, position) + compute_pascal(row - 1, position - 1)
		// we need to split these up as they are both recursive calls, so that we know when to 
		// recognise exactly when this instance of a function call should be recognised as returning
		// for the sake of the CWP/SWP in return_function_risc() below.
		int a = compute_pascal_RISC(row - 1, position, ourPSW);				
		int b = compute_pascal_RISC(row - 1, position - 1, ourPSW);

		return_function_risc(ourPSW);
		return a+b;
	}
}

int start_risc_processor(int row, int position, int num_windows, int overflow_allowance) {
	NWINDOWS = num_windows;
	num_reg_per_window = 32;	// R10-R31 registers get pushed to stack
	num_procedure_calls = 0;
	current_window_depth = 0;
	max_window_depth = 0;
	num_overflows = 0;
	num_underflows = 0;
	num_empty_windows_for_overflow = overflow_allowance;
	psw * ourPSW = create_psw();
	printf("BEFORE CALL -  CWP = %d, SWP = %d, NUSED = %d\n", (*ourPSW).cwp, (*ourPSW).swp, (*ourPSW).WUSED);
	int result = compute_pascal_RISC(row, position, ourPSW);
	printf("AFTER CALL - CWP = %d, SWP = %d, NUSED = %d\n\n", (*ourPSW).cwp, (*ourPSW).swp, (*ourPSW).WUSED);
	return result;
}

void calc_and_print_results_RISC(int row, int position, int num_windows) {
	start_risc_processor(row, position, num_windows, 0);
	printf("FOR %d REGISTER WINDOWS (ALL WINDOWS USED FOR OVERFLOW):\n", num_windows);
	printf("PROCEDURE CALLS = %d\nMAX WINDOW DEPTH = %d\nNUM OVERFLOWS = %d\nNUM UNDERFLOWS = %d\n\n", 
		num_procedure_calls, max_window_depth, num_overflows, num_underflows);

	start_risc_processor(row, position, num_windows, 1);
	printf("FOR %d REGISTER WINDOWS (1 EMPTY WINDOW FOR OVERFLOW):\n", num_windows);
	printf("PROCEDURE CALLS = %d\nMAX WINDOW DEPTH = %d\nNUM OVERFLOWS = %d\nNUM UNDERFLOWS = %d\n\n",
		num_procedure_calls, max_window_depth, num_overflows, num_underflows);
}

int main()
{
	int row = 30;
	int position = 20;

	// to show RISC-emulating version is producing the correct result
	int num_windows = 16;		
	printf("C++ Answer  = %d\n", compute_pascal_C(row, position));
	printf("RISC Answer = %d\n\n", start_risc_processor(row, position, num_windows, 0));

	num_windows = 6;
	calc_and_print_results_RISC(row, position, num_windows);

	num_windows = 8;
	calc_and_print_results_RISC(row, position, num_windows);

	num_windows = 16;
	calc_and_print_results_RISC(row, position, num_windows);

	// for calculating time of unimplemented version 
	
	double sum_time = 0;
	double each_time[11] = { };
	std::chrono::time_point<std::chrono::system_clock> start, end;

	for (int i = 0; i < 11; i++) {
		start = std::chrono::system_clock::now();
		compute_pascal_C(row, position);
		end = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - start;
		sum_time += elapsed_seconds.count();
		each_time[i] = elapsed_seconds.count();
	}

	sum_time /= 11;
	std::sort(each_time, each_time + 11);
	printf("DURATION OF UNIMPLEMENTED VERSION:\n\tAVERAGE = %lf\n\tMEDIAN = %lf\n", sum_time, each_time[5]);

	return 0;
}
