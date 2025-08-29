#ifndef assert_h_INCLUDED
#define assert_h_INCLUDED

#define DEBUG_ASSERTIONS
#define DEBUG_STRICT_ASSERTIONS

#define panic() do { printf("Panic at %s:%u\n", __FILE__, __LINE__); exit(1); } while(0)

#define assert(assertion) do { if(!(assertion)) { printf("Assertion failed at %s:%u\n", __FILE__, __LINE__); exit(1); } } while(0)

#ifdef DEBUG_ASSERTIONS
	#define debug_assert(assertion) assert(assertion)
#else
	#define debug_assert
#endif

#ifdef STRICT_ASSERTIONS
	#define strict_assert(assertion) assert(assertion)
#else
	#define strict_assert
#endif

#endif // assert_h_INCLUDED
