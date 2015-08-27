#define F (1 << 14)			// fixed point 1 (==16384)
#define INT_MAX ((1 << 31) - 1)
#define INT_MIN (-1(1 << 31))

#define ONLY_POINT	2147500031	// 10000000000000000011111111111111
#define ONLY_INT	4294950912

// x and y denote fixed_point numbers in 17.14 format
// n is an integer

int int_to_fp (int n);			// Convert integer to fixed point
int fp_to_int_round (int x);		// Convert FP to int (round)
int fp_to_int (int x);			// Convert FP to int (throw)
int add_fp (int x, int y);		// Add of fp
int add_mixed (int x, int n);		// Add fp with int
int sub_fp (int x, int y);		// Subtraction of fp
int sub_mixed (int x, int n);		// Subtract fp with int (x-n)
int mult_fp (int x, int y);		// Multiply of fp
int mult_mixed (int x, int n);		// Multiply fp with int
int div_fp (int x, int y);		// Division of fp
int div_mixed (int x, int n);		// Division fp with int (x/n)

int int_to_fp (int n)
{
	return F*n;
}
int fp_to_int_round (int x)
{
	if (0 <= x)
	{
		return (x + (F/2))/F;
	}
	else
	{
		return (x - (F/2))/F;
	}
}
int fp_to_int (int x)
{
	return x/F;
}
int add_fp (int x, int y)
{
	return x + y;
}
int add_mixed (int x, int n)
{
	return x + (n*F);
}
int sub_fp (int x, int y)
{
	return x - y;
}
int sub_mixed (int x, int n)
{
	return x - (n*F);
}
int mult_fp (int x, int y)
{
	return ((int64_t)x)*y/F;
}
int mult_mixed (int x, int n)
{
	return x * n;
}
int div_fp (int x, int y)
{
	return ((int64_t)x)*F/y;
}
int div_mixed (int x, int n)
{
	return x / n;
}
